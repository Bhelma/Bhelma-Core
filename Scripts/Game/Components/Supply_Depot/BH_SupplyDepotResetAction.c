//------------------------------------------------------------------------------------------------
//! BH_SupplyDepotResetAction.c
//!
//! DESCRIPCION:
//!   Accion del Scenario Framework que resetea el BH_SupplyDepotTaskComponent de una entidad
//!   objetivo, permitiendo que la tarea de suministros pueda completarse de nuevo.
//!
//!   Util para misiones repetibles donde los jugadores deben llevar suministros al mismo
//!   depot varias veces (ej: primera oleada 500, segunda oleada otros 500, etc.).
//!
//! CONFIGURACION EN WORLD EDITOR:
//!   1. En cualquier array de acciones SF (ej: "Acciones al Completar" de un LayerTask,
//!      o en un BH_ScenarioFrameworkSlotInteractive, etc.) anade esta accion:
//!      BH_SupplyDepotResetAction
//!   2. En "Entidad Depot Objetivo" arrastra el SupplyDepot que quieres resetear.
//!   3. Cuando la accion se active, el depot volvera a monitorizar suministros desde cero.
//!
//! NOTA:
//!   Esta accion NO borra los suministros fisicos del depot. Solo resetea el estado interno
//!   del componente BH_SupplyDepotTaskComponent para que pueda volver a completarse.
//!   Si necesitas vaciar el depot tambien, hazlo con las herramientas vanilla de SF.
//------------------------------------------------------------------------------------------------

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sDescription")]
class BH_SupplyDepotResetAction : SCR_ScenarioFrameworkActionBase
{
	[Attribute(
		desc: "Entidad SupplyDepot que contiene el BH_SupplyDepotTaskComponent a resetear. Si se deja vacio, se intenta usar la entidad sobre la que se activa la accion.",
		uiwidget: UIWidgets.ResourceNamePicker,
		category: "BH Supply Reset"
	)]
	protected string m_sDepotEntityName;

	[Attribute(
		defvalue: "0",
		uiwidget: UIWidgets.CheckBox,
		desc: "Activa logs de debug en la consola.",
		category: "BH Debug"
	)]
	protected bool m_bBH_Debug;

	//------------------------------------------------------------------------------------------------
	protected void BH_Log(string message, LogLevel level = LogLevel.NORMAL)
	{
		if (!m_bBH_Debug)
			return;
		Print(string.Format("[BH_SupplyDepotResetAction] %1", message), level);
	}

	//------------------------------------------------------------------------------------------------
	override void OnActivate(IEntity object)
	{
		if (!Replication.IsServer())
			return;

		IEntity depotEntity;

		// Si se ha especificado un nombre de entidad, buscarla en el mundo
		if (!m_sDepotEntityName.IsEmpty())
		{
			depotEntity = GetGame().GetWorld().FindEntityByName(m_sDepotEntityName);
			if (!depotEntity)
			{
				Print(string.Format("[BH_SupplyDepotResetAction] ERROR: No se encontro ninguna entidad con el nombre '%1' en el mundo.", m_sDepotEntityName), LogLevel.ERROR);
				return;
			}
		}
		else
		{
			// Fallback: usar la entidad sobre la que se activa la accion
			depotEntity = object;
		}

		if (!depotEntity)
		{
			Print("[BH_SupplyDepotResetAction] ERROR: No hay entidad objetivo. Configura 'm_sDepotEntityName' o asegurate de que la accion se activa sobre el depot.", LogLevel.ERROR);
			return;
		}

		BH_SupplyDepotTaskComponent supplyTask = BH_SupplyDepotTaskComponent.Cast(depotEntity.FindComponent(BH_SupplyDepotTaskComponent));
		if (!supplyTask)
		{
			Print(string.Format("[BH_SupplyDepotResetAction] ERROR: La entidad '%1' no tiene BH_SupplyDepotTaskComponent.", depotEntity.GetName()), LogLevel.ERROR);
			return;
		}

		supplyTask.ResetTask();

		BH_Log(string.Format("Tarea reseteada en depot '%1'. Suministros requeridos: %.1f",
			depotEntity.GetName(),
			supplyTask.GetRequiredSupplies()));
	}
}