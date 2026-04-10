// ============================================================
//  BH_VBIED.c
//  Autor: BH Mod
//  Descripción: Componente que convierte un vehículo en un
//               VBIED (Vehicle-Borne IED / coche bomba).
//
//  Se añade al prefab del vehículo JUNTO con:
//  - BH_IED_Controller (lógica de IED, cables, estados)
//  - BH_IED_DisarmInteraction (acciones de cortar cable)
//  - BH_IED_ProximityTrigger (detección de proximidad)
//
//  Este componente se encarga de:
//  1) Bloquear las puertas del vehículo (nadie puede subirse)
//  2) Al explotar, destruir el vehículo via DamageManager
//     (queda el chasis quemado) en vez de eliminar la entidad
//  3) Usar SCR_VehicleSoundComponent para el sonido de explosión
//
//  CONFIGURACIÓN EN WORKBENCH:
//  1. Hereda de un prefab de vehículo vanilla
//  2. Añade BH_IED_Controller, BH_IED_DisarmInteraction,
//     BH_IED_ProximityTrigger y BH_VBIED
//  3. Añade las acciones de cable en ActionsManagerComponent
//  4. Configura el SoundComponent con el .acp de la IED
//  5. El vehículo aparecerá con puertas bloqueadas
// ============================================================

class BH_VBIEDClass : ScriptComponentClass {}

class BH_VBIED : ScriptComponent
{
	// --------------------------------------------------------
	// Atributos configurables desde Workbench
	// --------------------------------------------------------

	[Attribute("1", UIWidgets.CheckBox, "Bloquear puertas del vehículo (impedir que se suban)")]
	protected bool m_bLockDoors;

	// --------------------------------------------------------
	// Variables internas
	// --------------------------------------------------------

	protected BH_IED_Controller m_Controller;

	// --------------------------------------------------------
	// Init
	// --------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_Controller = BH_IED_Controller.Cast(owner.FindComponent(BH_IED_Controller));
		if (!m_Controller)
		{
			BH_IED_Utils.BH_Log("[BH_VBIED] ERROR: No se encontró BH_IED_Controller en el vehículo.");
			return;
		}

		// Marcar este controller como VBIED para que no destruya la entidad
		m_Controller.BH_SetIsVBIED(true);

		// Bloquear puertas
		if (m_bLockDoors)
			BH_LockAllDoors(owner);

		BH_IED_Utils.BH_Log("[BH_VBIED] VBIED inicializado.");
	}

	// --------------------------------------------------------
	// Bloquear todas las puertas del vehículo
	// --------------------------------------------------------

	protected void BH_LockAllDoors(IEntity owner)
	{
		// Buscar todos los compartimentos y bloquear acceso
		BaseCompartmentManagerComponent compMgr = BaseCompartmentManagerComponent.Cast(
			owner.FindComponent(BaseCompartmentManagerComponent)
		);
		if (!compMgr)
		{
			BH_IED_Utils.BH_Log("[BH_VBIED] AVISO: No se encontró BaseCompartmentManagerComponent.");
			return;
		}

		// Obtener todos los compartimentos
		array<BaseCompartmentSlot> compartments = new array<BaseCompartmentSlot>();
		compMgr.GetCompartments(compartments);

		foreach (BaseCompartmentSlot slot : compartments)
		{
			if (slot)
				slot.SetCompartmentAccessible(false);
		}

		BH_IED_Utils.BH_Log(string.Format("[BH_VBIED] %1 puertas bloqueadas.", compartments.Count()));
	}

	// --------------------------------------------------------
	// Destruir el vehículo via DamageManager
	// Llamado desde BH_IED_Controller.BH_Explode() cuando
	// m_bIsVBIED es true
	// --------------------------------------------------------

	void BH_DestroyVehicle()
	{
		IEntity owner = GetOwner();
		if (!owner) return;

		DamageManagerComponent dmgMgr = DamageManagerComponent.Cast(
			owner.FindComponent(DamageManagerComponent)
		);
		if (dmgMgr)
		{
			// Destruir todas las hitzones para destrucción completa del vehículo
			array<HitZone> hitZones = new array<HitZone>();
			dmgMgr.GetAllHitZones(hitZones);

			foreach (HitZone hz : hitZones)
			{
				if (!hz) continue;

				float maxHP = hz.GetMaxHealth();
				if (maxHP <= 0) continue;

				BaseDamageContext dmgContext = new BaseDamageContext();
				dmgContext.damageValue = maxHP + 1000;
				dmgContext.struckHitZone = hz;
				dmgMgr.HandleDamage(dmgContext);
			}

			BH_IED_Utils.BH_Log(string.Format("[BH_VBIED] Vehículo destruido. %1 hitzones dañadas.", hitZones.Count()));
		}
		else
		{
			BH_IED_Utils.BH_Log("[BH_VBIED] AVISO: Sin DamageManager, eliminando entidad.");
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
		}
	}
}