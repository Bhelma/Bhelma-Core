//------------------------------------------------------------------------------------------------
//! Accion de usuario autonoma con sus propias acciones del Scenario Framework.
//! Se configura directamente en el prefab, sin necesitar un BH_ScenarioFrameworkSlotInteractive.
//!
//! CASO DE USO:
//! Tienes un prefab de telefono con esta accion configurada. Pones 10 telefonos en el mapa.
//! Cuando cualquier jugador interactua con cualquiera de ellos, se ejecutan las acciones SF
//! configuradas aqui. Todos los telefonos hacen lo mismo sin necesitar un Slot cada uno.
//!
//! MULTIJUGADOR:
//! - HasLocalEffectOnlyScript() retorna FALSE = PerformAction se ejecuta en el SERVIDOR
//! - Las acciones SF gestionan su propia replicacion internamente
//! - Funciona en local, listen server y servidor dedicado
//!
//! CONFIGURACION EN EL PREFAB:
//! 1. El prefab necesita ActionsManagerComponent con un Action Context
//! 2. En Additional Actions, anadir BH_InteractiveSlotUserAction
//! 3. Configurar el nombre, duracion, uso unico
//! 4. Anadir las acciones SF en "Acciones al realizar"
//! 5. Colocar el prefab donde quieras — no necesita Slot ni Layer ni Area
//------------------------------------------------------------------------------------------------
class BH_InteractiveSlotUserAction : ScriptedUserAction
{
	// ---- Debug ----
	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Activa los logs de debug en la consola", category: "BH Debug")]
	protected bool m_bBH_Debug;

	// ---- Configuracion de la accion ----
	[Attribute(defvalue: "Interactuar", uiwidget: UIWidgets.EditBox, desc: "Nombre de la accion que ve el jugador (ej: 'Usar Telefono', 'Pulsar Boton')", category: "BH Accion")]
	protected string m_sBH_ActionName;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Si esta marcado, la accion solo se puede realizar una vez por cada instancia del objeto", category: "BH Accion")]
	protected bool m_bBH_OneTimeUse;

	// ---- Acciones del Scenario Framework ----
	[Attribute(desc: "Acciones del Scenario Framework que se ejecutan cuando el jugador REALIZA la accion. Se ejecutan en el servidor. Para notificaciones, usar SCR_ScenarioFrameworkActionShowPopupNotification.", category: "BH Accion - Acciones SF")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aBH_OnActionPerformed;

	[Attribute(desc: "Acciones del Scenario Framework que se ejecutan cuando el jugador EMPIEZA la accion (solo si la accion tiene duracion > 0)", category: "BH Accion - Acciones SF")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aBH_OnActionStarted;

	[Attribute(desc: "Acciones del Scenario Framework que se ejecutan cuando el jugador CANCELA la accion (solo si la accion tiene duracion > 0)", category: "BH Accion - Acciones SF")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aBH_OnActionCancelled;

	// ---- Estado interno (por instancia, no compartido entre objetos) ----
	protected bool m_bBH_HasBeenPerformed = false;

	//------------------------------------------------------------------------------------------------
	protected void BH_Log(string message, LogLevel level = LogLevel.NORMAL)
	{
		if (!m_bBH_Debug)
			return;

		Print(string.Format("[BH_StandaloneAction] %1", message), level);
	}

	//------------------------------------------------------------------------------------------------
	//! Se ejecuta en el SERVIDOR — ejecuta las acciones SF configuradas en el prefab
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_Log(string.Format("PerformAction - Owner: %1, User: %2", pOwnerEntity, pUserEntity));

		if (m_bBH_OneTimeUse && m_bBH_HasBeenPerformed)
		{
			BH_Log("Ya realizada (UsoUnico), ignorando");
			return;
		}

		m_bBH_HasBeenPerformed = true;

		// Ejecutar acciones SF — tienen su propia replicacion interna
		BH_ExecuteActions(m_aBH_OnActionPerformed, "OnActionPerformed");

		BH_Log(string.Format("'%1' completada!", m_sBH_ActionName));
	}

	//------------------------------------------------------------------------------------------------
	//! Se ejecuta en el SERVIDOR — notifica que la accion empezo (acciones con duracion)
	override void OnActionStart(IEntity pUserEntity)
	{
		super.OnActionStart(pUserEntity);
		BH_Log(string.Format("OnActionStart - User: %1", pUserEntity));
		BH_ExecuteActions(m_aBH_OnActionStarted, "OnActionStarted");
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (m_bBH_OneTimeUse && m_bBH_HasBeenPerformed)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (m_bBH_OneTimeUse && m_bBH_HasBeenPerformed)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool GetActionNameScript(out string outName)
	{
		if (m_sBH_ActionName.IsEmpty())
			return false;

		outName = m_sBH_ActionName;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! FALSE = PerformAction se ejecuta en el SERVIDOR, no localmente
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Ejecutar acciones SF — identico al patron de BH_NPCInteractionComponent
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
}