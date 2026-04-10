// ============================================================
//  BH_JammerToggleAction.c
//  Autor: BH Mod
//  Descripción: Dos acciones separadas para activar y
//               desactivar el inhibidor de señal.
//               - BH_JammerActivateAction: solo visible cuando
//                 el jammer está APAGADO
//               - BH_JammerDeactivateAction: solo visible cuando
//                 el jammer está ENCENDIDO
//               Ambas requieren estar dentro del vehículo.
// ============================================================

// ============================================================
//  Acción: ACTIVAR inhibidor
//  Solo aparece cuando el jammer está APAGADO
// ============================================================

class BH_JammerActivateAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		BH_VehicleJammer jammer = BH_GetJammer();
		if (!jammer) return false;

		// Solo mostrar si el jammer está APAGADO
		if (jammer.BH_IsActive()) return false;

		return BH_IsPlayerInVehicle(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_VehicleJammer jammer = BH_GetJammer();
		if (!jammer) return;

		jammer.BH_SetActive(true);
		BH_IED_Utils.BH_Log("[BH_JammerAction] Inhibidor ACTIVADO.");
	}

	protected BH_VehicleJammer BH_GetJammer()
	{
		IEntity owner = GetOwner();
		if (!owner) return null;
		return BH_VehicleJammer.Cast(owner.FindComponent(BH_VehicleJammer));
	}

	protected bool BH_IsPlayerInVehicle(IEntity player)
	{
		if (!player) return false;
		IEntity currentVehicle = CompartmentAccessComponent.GetVehicleIn(player);
		if (!currentVehicle) return false;
		return currentVehicle == GetOwner();
	}
}

// ============================================================
//  Acción: DESACTIVAR inhibidor
//  Solo aparece cuando el jammer está ENCENDIDO
// ============================================================

class BH_JammerDeactivateAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		BH_VehicleJammer jammer = BH_GetJammer();
		if (!jammer) return false;

		// Solo mostrar si el jammer está ENCENDIDO
		if (!jammer.BH_IsActive()) return false;

		return BH_IsPlayerInVehicle(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_VehicleJammer jammer = BH_GetJammer();
		if (!jammer) return;

		jammer.BH_SetActive(false);
		BH_IED_Utils.BH_Log("[BH_JammerAction] Inhibidor DESACTIVADO.");
	}

	protected BH_VehicleJammer BH_GetJammer()
	{
		IEntity owner = GetOwner();
		if (!owner) return null;
		return BH_VehicleJammer.Cast(owner.FindComponent(BH_VehicleJammer));
	}

	protected bool BH_IsPlayerInVehicle(IEntity player)
	{
		if (!player) return false;
		IEntity currentVehicle = CompartmentAccessComponent.GetVehicleIn(player);
		if (!currentVehicle) return false;
		return currentVehicle == GetOwner();
	}
}