//------------------------------------------------------------------------------------------------
//! Slot personalizado del Scenario Framework que spawnea un objeto interactivo.
//! Cuando un jugador interactua con el, se ejecutan las acciones del Scenario Framework configuradas.
//!
//! ARQUITECTURA MULTIJUGADOR:
//! - PerformAction se ejecuta en el SERVIDOR (authority) porque HasLocalEffectOnlyScript retorna false
//! - Las acciones SF tienen su propia replicacion interna — funcionan en local, listen y dedicado
//! - El slot NO tiene RplComponent, por lo que no usamos RplProp ni Rpc directamente
//! - Para notificaciones/hints, usar las acciones SF del desplegable (ej: ShowPopupNotification)
//!
//! CONFIGURACION EN WORLD EDITOR:
//! 1. Arrastra un Slot.et dentro de la jerarquia de tu Layer
//! 2. Reemplaza o anade este componente (BH_ScenarioFrameworkSlotInteractive)
//! 3. Configura "Object To Spawn" con cualquier prefab (ej: interruptor, radio, laptop, etc.)
//! 4. Configura el nombre de la accion (ej: "Encender Luz")
//! 5. Anade acciones del Scenario Framework en el array "On Action Performed"
//! 6. Listo! Funciona en local, listen server y servidor dedicado.
//------------------------------------------------------------------------------------------------

[EntityEditorProps(category: "GameScripted/ScenarioFramework", description: "BH Slot Interactivo - Spawnea un objeto con una accion de usuario configurable que ejecuta acciones del Scenario Framework", color: "0 255 128 255")]
class BH_ScenarioFrameworkSlotInteractiveClass : SCR_ScenarioFrameworkSlotBaseClass
{
}

//------------------------------------------------------------------------------------------------
class BH_ScenarioFrameworkSlotInteractive : SCR_ScenarioFrameworkSlotBase
{
	// ---- Registro estatico: permite que las acciones de usuario encuentren su slot ----
	protected static ref map<IEntity, BH_ScenarioFrameworkSlotInteractive> s_mBH_EntitySlotMap = new map<IEntity, BH_ScenarioFrameworkSlotInteractive>();

	//------------------------------------------------------------------------------------------------
	//! Busca el slot asociado a una entidad spawneada. Usado por BH_DynamicInteractiveAction.
	static BH_ScenarioFrameworkSlotInteractive BH_GetSlotForEntity(IEntity entity)
	{
		if (!entity || !s_mBH_EntitySlotMap)
			return null;

		return s_mBH_EntitySlotMap.Get(entity);
	}

	// ---- Debug ----
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Activa los logs de debug en la consola para este slot", category: "BH Debug")]
	protected bool m_bBH_Debug;

	// ---- Configuracion de la accion ----
	[Attribute(defvalue: "Interactuar", uiwidget: UIWidgets.EditBox, desc: "Nombre de la accion que ve el jugador (ej: 'Encender Luz', 'Activar Radio')", category: "BH Accion Interactiva")]
	protected string m_sBH_ActionName;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Duracion en segundos para mantener la accion (0 = instantanea)", category: "BH Accion Interactiva")]
	protected float m_fBH_ActionDuration;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Si esta marcado, la accion solo se puede realizar una vez", category: "BH Accion Interactiva")]
	protected bool m_bBH_OneTimeUse;

	// ---- Acciones del Scenario Framework ----
	[Attribute(desc: "Acciones del Scenario Framework que se ejecutan cuando el jugador REALIZA la accion. Para notificaciones, usar SCR_ScenarioFrameworkActionShowPopupNotification aqui.", category: "BH Accion Interactiva - Acciones SF")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aBH_OnActionPerformed;

	[Attribute(desc: "Acciones del Scenario Framework que se ejecutan cuando el jugador EMPIEZA la accion (solo si duracion > 0)", category: "BH Accion Interactiva - Acciones SF")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aBH_OnActionStarted;

	[Attribute(desc: "Acciones del Scenario Framework que se ejecutan cuando el jugador CANCELA la accion (solo si duracion > 0)", category: "BH Accion Interactiva - Acciones SF")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aBH_OnActionCancelled;

	// ---- Estado interno ----
	protected bool m_bBH_ActionPerformed = false;
	protected IEntity m_BH_SpawnedEntity;

	//------------------------------------------------------------------------------------------------
	//! Log interno de debug - solo imprime si m_bBH_Debug esta activado
	protected void BH_Log(string message, LogLevel level = LogLevel.NORMAL)
	{
		if (!m_bBH_Debug)
			return;

		Print(string.Format("[BH_SlotInteractive] %1", message), level);
	}

	//------------------------------------------------------------------------------------------------
	//! Se llama despues de que la entidad es spawneada por el Scenario Framework
	override void AfterAllChildrenSpawned(SCR_ScenarioFrameworkLayerBase layer)
	{
		super.AfterAllChildrenSpawned(layer);

		BH_Log("AfterAllChildrenSpawned - INICIO");

		m_BH_SpawnedEntity = m_Entity;
		if (!m_BH_SpawnedEntity)
		{
			BH_Log("ERROR: No se ha spawneado ninguna entidad! Comprueba la configuracion de Object To Spawn.", LogLevel.ERROR);
			return;
		}

		BH_Log(string.Format("Entidad spawneada: %1", m_BH_SpawnedEntity));

		// Registrar esta entidad en el mapa estatico para que la accion de usuario la encuentre
		s_mBH_EntitySlotMap.Set(m_BH_SpawnedEntity, this);
		BH_Log("Entidad registrada en el mapa estatico de slots");

		// Validar configuracion (solo debug)
		if (m_bBH_Debug)
			BH_ValidateUserAction(m_BH_SpawnedEntity);

		BH_Log(string.Format("Configuracion completa. Accion: '%1', Duracion: %2, UsoUnico: %3", m_sBH_ActionName, m_fBH_ActionDuration, m_bBH_OneTimeUse));
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_ValidateUserAction(IEntity entity)
	{
		if (!entity)
			return;

		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent));
		if (!actionsManager)
		{
			BH_Log("AVISO: No tiene ActionsManagerComponent! El prefab necesita uno con BH_DynamicInteractiveAction.", LogLevel.WARNING);
			return;
		}

		BH_Log("ActionsManagerComponent encontrado");

		array<BaseUserAction> actions = {};
		actionsManager.GetActionsList(actions);
		BH_Log(string.Format("La entidad tiene %1 acciones de usuario", actions.Count()));

		bool foundBHAction = false;
		foreach (BaseUserAction action : actions)
		{
			BH_Log(string.Format("  - %1", action.ClassName()));
			if (BH_DynamicInteractiveAction.Cast(action) || BH_InteractiveSlotUserAction.Cast(action))
				foundBHAction = true;
		}

		if (!foundBHAction)
			BH_Log("AVISO: No se encontro ninguna accion BH_! Anade BH_DynamicInteractiveAction a Additional Actions.", LogLevel.WARNING);
		else
			BH_Log("Accion BH encontrada - listo");
	}

	//------------------------------------------------------------------------------------------------
	//! Llamado por las acciones BH cuando el jugador REALIZA la accion.
	//! Se ejecuta en el SERVIDOR porque HasLocalEffectOnlyScript() retorna false.
	void BH_OnInteractiveActionPerformed(IEntity ownerEntity, IEntity userEntity)
	{
		BH_Log(string.Format("ACCION REALIZADA - Owner: %1, User: %2", ownerEntity, userEntity));

		if (m_bBH_OneTimeUse && m_bBH_ActionPerformed)
		{
			BH_Log("Ya realizada (UsoUnico), ignorando");
			return;
		}

		m_bBH_ActionPerformed = true;

		// Ejecutar acciones SF — tienen su propia replicacion interna
		BH_ExecuteActions(m_aBH_OnActionPerformed, "OnActionPerformed");

		BH_Log(string.Format("'%1' completada!", m_sBH_ActionName));
	}

	//------------------------------------------------------------------------------------------------
	//! Llamado cuando el jugador EMPIEZA la accion (duracion > 0)
	void BH_OnInteractiveActionStarted(IEntity ownerEntity, IEntity userEntity)
	{
		BH_Log("ACCION INICIADA");
		BH_ExecuteActions(m_aBH_OnActionStarted, "OnActionStarted");
	}

	//------------------------------------------------------------------------------------------------
	//! Llamado cuando el jugador CANCELA la accion
	void BH_OnInteractiveActionCancelled(IEntity ownerEntity, IEntity userEntity)
	{
		BH_Log("ACCION CANCELADA");
		BH_ExecuteActions(m_aBH_OnActionCancelled, "OnActionCancelled");
	}

	//------------------------------------------------------------------------------------------------
	//! Ejecutar acciones SF — identico al patron usado en BH_NPCInteractionComponent
	//! Las acciones SF gestionan su propia replicacion internamente
	protected void BH_ExecuteActions(array<ref SCR_ScenarioFrameworkActionBase> actions, string arrayName)
	{
		if (!actions || actions.IsEmpty())
		{
			BH_Log(string.Format("ExecuteActions: '%1' vacio o null", arrayName));
			return;
		}

		BH_Log(string.Format("ExecuteActions: '%1' (%2 acciones)", arrayName, actions.Count()));

		foreach (int i, SCR_ScenarioFrameworkActionBase action : actions)
		{
			if (action)
			{
				BH_Log(string.Format("  [%1] Ejecutando %2", i, action.ClassName()));
				action.OnActivate(GetOwner());
			}
			else
			{
				BH_Log(string.Format("  [%1] NULL, saltando", i), LogLevel.WARNING);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// ---- Getters publicos ----
	//------------------------------------------------------------------------------------------------

	bool BH_HasBeenPerformed()		{ return m_bBH_ActionPerformed; }
	string BH_GetActionName()		{ return m_sBH_ActionName; }
	float BH_GetActionDuration()	{ return m_fBH_ActionDuration; }
	bool BH_IsOneTimeUse()			{ return m_bBH_OneTimeUse; }
	bool BH_IsDebugEnabled()		{ return m_bBH_Debug; }

	//------------------------------------------------------------------------------------------------
	override void RestoreToDefault(bool includeChildren = false, bool reinitAfterRestoration = false, bool affectRandomization = true)
	{
		BH_Log("RestoreToDefault - reiniciando");
		m_bBH_ActionPerformed = false;

		if (m_BH_SpawnedEntity)
			s_mBH_EntitySlotMap.Remove(m_BH_SpawnedEntity);

		super.RestoreToDefault(includeChildren, reinitAfterRestoration, affectRandomization);
	}

	//------------------------------------------------------------------------------------------------
	void ~BH_ScenarioFrameworkSlotInteractive()
	{
		if (m_BH_SpawnedEntity && s_mBH_EntitySlotMap)
			s_mBH_EntitySlotMap.Remove(m_BH_SpawnedEntity);
	}
}