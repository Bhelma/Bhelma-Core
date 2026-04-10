//------------------------------------------------------------------------------------------------
//! Accion de usuario dinamica que lee la configuracion del componente
//! BH_ScenarioFrameworkSlotInteractive en tiempo de ejecucion.
//!
//! MULTIJUGADOR:
//! - HasLocalEffectOnlyScript() retorna FALSE = PerformAction se ejecuta en el SERVIDOR
//! - Toda la logica se delega al slot, que ejecuta las acciones SF
//! - Las acciones SF gestionan su propia replicacion internamente
//!
//! Solo hay que anadir esta accion al prefab en ActionsManagerComponent > Additional Actions.
//! El slot controla el nombre de la accion, que ocurre al interactuar, etc.
//!
//! COMO ENCUENTRA EL SLOT:
//! El slot se registra en un mapa estatico (entidad -> slot) al spawnear.
//! Esta accion busca su entidad owner en ese mapa. No depende de jerarquia parent-child.
//------------------------------------------------------------------------------------------------
class BH_DynamicInteractiveAction : ScriptedUserAction
{
	protected BH_ScenarioFrameworkSlotInteractive m_BH_SlotComponent;
	protected bool m_bBH_SlotSearchDone = false;

	//------------------------------------------------------------------------------------------------
	protected void BH_Log(string message, LogLevel level = LogLevel.NORMAL)
	{
		BH_ScenarioFrameworkSlotInteractive slot = BH_GetSlot();
		if (!slot || !slot.BH_IsDebugEnabled())
			return;

		Print(string.Format("[BH_DynamicAction] %1", message), level);
	}

	//------------------------------------------------------------------------------------------------
	//! Busca el BH_ScenarioFrameworkSlotInteractive usando el registro estatico del slot.
	protected BH_ScenarioFrameworkSlotInteractive BH_GetSlot()
	{
		if (m_bBH_SlotSearchDone)
			return m_BH_SlotComponent;

		IEntity owner = GetOwner();
		if (!owner)
			return null;

		m_BH_SlotComponent = BH_ScenarioFrameworkSlotInteractive.BH_GetSlotForEntity(owner);

		if (m_BH_SlotComponent)
		{
			m_bBH_SlotSearchDone = true;
			if (m_BH_SlotComponent.BH_IsDebugEnabled())
				Print(string.Format("[BH_DynamicAction] Slot encontrado para entidad %1", owner), LogLevel.NORMAL);
		}

		return m_BH_SlotComponent;
	}

	//------------------------------------------------------------------------------------------------
	//! Se ejecuta en el SERVIDOR (authority) — delega al slot
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_ScenarioFrameworkSlotInteractive slot = BH_GetSlot();
		if (!slot)
		{
			Print(string.Format("[BH_DynamicAction] ERROR: No se encontro BH_ScenarioFrameworkSlotInteractive para la entidad %1!", pOwnerEntity), LogLevel.ERROR);
			return;
		}

		BH_Log(string.Format("PerformAction - Owner: %1, User: %2", pOwnerEntity, pUserEntity));
		slot.BH_OnInteractiveActionPerformed(pOwnerEntity, pUserEntity);
	}

	//------------------------------------------------------------------------------------------------
	//! Se ejecuta en el SERVIDOR — notifica al slot que la accion empezo
	override void OnActionStart(IEntity pUserEntity)
	{
		super.OnActionStart(pUserEntity);

		BH_ScenarioFrameworkSlotInteractive slot = BH_GetSlot();
		if (slot)
		{
			BH_Log(string.Format("OnActionStart - User: %1", pUserEntity));
			slot.BH_OnInteractiveActionStarted(GetOwner(), pUserEntity);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Se ejecuta en el CLIENTE y SERVIDOR — comprueba si la accion ya se realizo
	override bool CanBeShownScript(IEntity user)
	{
		BH_ScenarioFrameworkSlotInteractive slot = BH_GetSlot();
		if (slot && slot.BH_IsOneTimeUse() && slot.BH_HasBeenPerformed())
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Se ejecuta en el CLIENTE y SERVIDOR — comprueba si la accion ya se realizo
	override bool CanBePerformedScript(IEntity user)
	{
		BH_ScenarioFrameworkSlotInteractive slot = BH_GetSlot();
		if (slot && slot.BH_IsOneTimeUse() && slot.BH_HasBeenPerformed())
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Lee el nombre de la accion desde la config del slot
	override bool GetActionNameScript(out string outName)
	{
		BH_ScenarioFrameworkSlotInteractive slot = BH_GetSlot();
		if (slot)
		{
			string name = slot.BH_GetActionName();
			if (!name.IsEmpty())
			{
				outName = name;
				return true;
			}
		}

		outName = "Interactuar";
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Retorna FALSE = PerformAction se ejecuta en el SERVIDOR (authority), no localmente
	override bool HasLocalEffectOnlyScript()
	{
		return false;
	}
}