//------------------------------------------------------------------------------------------------
//! BH_SupplyDepotTaskComponent.c
//!
//! DESCRIPCION:
//!   Monitoriza los suministros entregados acumulativamente en un punto destino de la red
//!   logistica y ejecuta acciones SF al alcanzar el objetivo.
//!
//!   IMPORTANTE: Colocar este componente en el sub-prefab hijo que tiene el contenedor
//!   real de suministros (el que muestra valor > 0 al recibir entregas), NO en el
//!   prefab distribuidor padre.
//!
//!   Usa el evento GetOnResourcesChanged() del SCR_ResourceContainer — sin polling.
//!   Solo acumula subidas (entregas), ignora bajadas (consumo del juego).
//!
//! CONFIGURACION EN WORLD EDITOR:
//!   1. Selecciona el sub-prefab hijo con el contenedor real de SUPPLIES.
//!   2. Anade el componente: BH_SupplyDepotTaskComponent.
//!   3. Configura "Suministros Requeridos". Cada punto puede tener su propio valor.
//!   4. En "Acciones al Completar" anade las acciones SF deseadas.
//!   5. Activa "Debug" para ver el progreso en consola.
//------------------------------------------------------------------------------------------------

[ComponentEditorProps(category: "BH/Tasks", description: "Monitoriza suministros entregados acumulativamente en un contenedor de suministros y ejecuta acciones SF al alcanzar el objetivo.")]
class BH_SupplyDepotTaskComponentClass : ScriptComponentClass {}

//------------------------------------------------------------------------------------------------
class BH_SupplyDepotTaskComponent : ScriptComponent
{
	[Attribute(
		defvalue: "500",
		uiwidget: UIWidgets.SpinBox,
		params: "0 999999 1",
		desc: "Cantidad total de suministros que deben entregarse (acumulados) para completar el objetivo. Las bajadas por consumo no restan.",
		category: "BH Supply Task"
	)]
	protected float m_fBH_RequiredSupplies;

	[Attribute(
		desc: "Acciones del Scenario Framework que se ejecutan UNA SOLA VEZ al alcanzar el objetivo.",
		uiwidget: UIWidgets.Object,
		category: "BH Supply Task"
	)]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aBH_OnObjectiveReached;

	[Attribute(
		defvalue: "0",
		uiwidget: UIWidgets.CheckBox,
		desc: "Activa logs de debug en la consola.",
		category: "BH Debug"
	)]
	protected bool m_bBH_Debug;

	// ---- Estado interno ----
	protected bool  m_bBH_Completed           = false;
	protected float m_fBH_AccumulatedSupplies = 0.0;
	protected bool  m_bBH_Initialized         = false;
	protected ref array<SCR_ResourceContainer> m_aBH_TrackedContainers = {};

	//------------------------------------------------------------------------------------------------
	protected void BH_Log(string message, LogLevel level = LogLevel.NORMAL)
	{
		if (!m_bBH_Debug)
			return;
		Print(string.Format("[BH_SupplyDepotTask | %1] %2", GetOwner().GetName(), message), level);
	}

	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		if (!Replication.IsServer() || m_bBH_Initialized)
			return;

		m_bBH_Initialized = true;

		SCR_ResourceComponent resourceComp = SCR_ResourceComponent.Cast(owner.FindComponent(SCR_ResourceComponent));
		if (!resourceComp)
		{
			Print(string.Format("[BH_SupplyDepotTask | %1] ERROR: No se encontro SCR_ResourceComponent.", owner.GetName()), LogLevel.ERROR);
			return;
		}

		// Intentar primero con GetContainer directo
		SCR_ResourceContainer container = resourceComp.GetContainer(EResourceType.SUPPLIES);
		if (container)
		{
			container.GetOnResourcesChanged().Insert(OnSuppliesChanged);
			m_aBH_TrackedContainers.Insert(container);
			BH_Log(string.Format("Suscrito a contenedor SUPPLIES. Valor actual: %1 / %2",
				container.GetResourceValue(), container.GetMaxResourceValue()));
		}

		// Recorrer todos por si hay varios
		array<SCR_ResourceContainer> allContainers = resourceComp.GetContainers();
		if (allContainers)
		{
			foreach (SCR_ResourceContainer c : allContainers)
			{
				if (!c || c.GetResourceType() != EResourceType.SUPPLIES)
					continue;
				if (m_aBH_TrackedContainers.Contains(c))
					continue;

				c.GetOnResourcesChanged().Insert(OnSuppliesChanged);
				m_aBH_TrackedContainers.Insert(c);
				BH_Log(string.Format("Suscrito a contenedor adicional SUPPLIES. Valor: %1 / %2",
					c.GetResourceValue(), c.GetMaxResourceValue()));
			}
		}

		if (m_aBH_TrackedContainers.IsEmpty())
		{
			Print(string.Format("[BH_SupplyDepotTask | %1] ERROR: No se encontro ningun contenedor de SUPPLIES. Asegurate de colocar el componente en el sub-prefab hijo con el contenedor real.", owner.GetName()), LogLevel.ERROR);
			return;
		}

		BH_Log(string.Format("Inicializado. Contenedores: %1 | Objetivo: %2",
			m_aBH_TrackedContainers.Count(), m_fBH_RequiredSupplies));
	}

	//------------------------------------------------------------------------------------------------
	//! Callback del SCR_ResourceContainer — se dispara cada vez que cambia el valor
	protected void OnSuppliesChanged(SCR_ResourceContainer container, float previousValue)
	{
		if (m_bBH_Completed || !container)
			return;

		float currentValue = container.GetResourceValue();
		float delta        = currentValue - previousValue;

		// Solo acumular subidas
		if (delta <= 0)
		{
			if (delta < 0)
				BH_Log(string.Format("Bajada ignorada: %1 consumidos. Actual: %2 | Acumulado: %3 / %4",
					-delta, currentValue, m_fBH_AccumulatedSupplies, m_fBH_RequiredSupplies));
			return;
		}

		m_fBH_AccumulatedSupplies += delta;

		BH_Log(string.Format("Entrega detectada: +%1 (ahora: %2). Acumulado: %3 / %4",
			delta, currentValue, m_fBH_AccumulatedSupplies, m_fBH_RequiredSupplies));

		CheckObjective();
	}

	//------------------------------------------------------------------------------------------------
	protected void CheckObjective()
	{
		if (m_bBH_Completed || m_fBH_AccumulatedSupplies < m_fBH_RequiredSupplies)
			return;

		m_bBH_Completed = true;

		foreach (SCR_ResourceContainer container : m_aBH_TrackedContainers)
		{
			if (container)
				container.GetOnResourcesChanged().Remove(OnSuppliesChanged);
		}

		BH_Log(string.Format("OBJETIVO ALCANZADO: %1 / %2. Ejecutando acciones SF...",
			m_fBH_AccumulatedSupplies, m_fBH_RequiredSupplies), LogLevel.WARNING);

		IEntity owner = GetOwner();
		foreach (SCR_ScenarioFrameworkActionBase action : m_aBH_OnObjectiveReached)
		{
			if (action)
				action.OnActivate(owner);
		}
	}

	//------------------------------------------------------------------------------------------------
	void ResetTask()
	{
		if (!Replication.IsServer())
			return;

		m_bBH_Completed           = false;
		m_fBH_AccumulatedSupplies = 0.0;

		foreach (SCR_ResourceContainer container : m_aBH_TrackedContainers)
		{
			if (!container)
				continue;
			container.GetOnResourcesChanged().Remove(OnSuppliesChanged);
			container.GetOnResourcesChanged().Insert(OnSuppliesChanged);
		}

		BH_Log(string.Format("Tarea reseteada. Acumulado: 0 / %1", m_fBH_RequiredSupplies));
	}

	//------------------------------------------------------------------------------------------------
	float GetAccumulatedSupplies() { return m_fBH_AccumulatedSupplies; }
	float GetRequiredSupplies()    { return m_fBH_RequiredSupplies; }
	bool  IsCompleted()            { return m_bBH_Completed; }

	//------------------------------------------------------------------------------------------------
	override protected void OnDelete(IEntity owner)
	{
		foreach (SCR_ResourceContainer container : m_aBH_TrackedContainers)
		{
			if (container)
				container.GetOnResourcesChanged().Remove(OnSuppliesChanged);
		}

		super.OnDelete(owner);
	}
}