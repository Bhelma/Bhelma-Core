// ============================================================
//  BH_IED_ProximityTrigger.c
//  Autor: BH Mod
//  Descripción: Detecta jugadores y vehículos en el radio
//               de la IED. Si no hay inhibidor activo,
//               reproduce el ring y detona.
//               Si hay jammer activo en radio, activa el
//               beep de alerta en el vehículo jammer.
//
//  DEDICADO: El beep se gestiona via BH_VehicleJammer
//  que replica el estado a los clientes. Este script
//  solo le dice al jammer "activa/desactiva el beep".
// ============================================================

class BH_IED_ProximityTriggerClass : ScriptComponentClass {}

class BH_IED_ProximityTrigger : ScriptComponent
{
	// --------------------------------------------------------
	// Atributos configurables desde Workbench
	// --------------------------------------------------------

	[Attribute("15", UIWidgets.Slider, "Radio de detección de proximidad (metros)", "1 50 1")]
	protected float m_fDetectionRadius;

	[Attribute("1", UIWidgets.CheckBox, "Detectar infantería")]
	protected bool m_bDetectInfantry;

	[Attribute("1", UIWidgets.CheckBox, "Detectar vehículos")]
	protected bool m_bDetectVehicles;

	[Attribute("1.5", UIWidgets.Slider, "Intervalo de comprobación en segundos", "0.2 5 0.1")]
	protected float m_fCheckInterval;

	[Attribute("30", UIWidgets.Slider, "Radio de inhibición del jammer (metros)", "5 100 1")]
	protected float m_fJammerRadius;

	// --------------------------------------------------------
	// Variables internas
	// --------------------------------------------------------

	protected BH_IED_Controller m_Controller;
	protected bool              m_bTriggered;
	protected bool              m_bJammerFound;
	protected Vehicle           m_ActiveJammerVehicle;
	protected bool              m_bBeepActive;

	// --------------------------------------------------------
	// Init
	// --------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		BH_IED_Utils.BH_Log(string.Format("[BH_IED_Proximity] OnPostInit. IsServer: %1", BH_IED_Utils.BH_IsServer()));

		if (!BH_IED_Utils.BH_IsServer())
			return;

		m_Controller = BH_IED_Controller.Cast(owner.FindComponent(BH_IED_Controller));
		if (!m_Controller)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Proximity] ERROR: No se encontró BH_IED_Controller en la entidad.");
			return;
		}

		m_bTriggered  = false;
		m_bBeepActive = false;

		GetGame().GetCallqueue().CallLater(BH_CheckProximity, m_fCheckInterval * 1000, true);
		BH_IED_Utils.BH_Log("[BH_IED_Proximity] Loop de comprobación iniciado.");
	}

	// --------------------------------------------------------
	// Limpiar CallLater si la entidad se destruye
	// --------------------------------------------------------

	override void OnDelete(IEntity owner)
	{
		// Si el beep estaba activo, pararlo
		if (m_bBeepActive && m_ActiveJammerVehicle)
		{
			BH_VehicleJammer jammer = BH_VehicleJammer.Cast(m_ActiveJammerVehicle.FindComponent(BH_VehicleJammer));
			if (jammer)
				jammer.BH_SetBeeping(false);
		}

		GetGame().GetCallqueue().Remove(BH_CheckProximity);
		super.OnDelete(owner);
	}

	// --------------------------------------------------------
	// Loop de comprobación
	// --------------------------------------------------------

	protected void BH_CheckProximity()
	{
		if (!m_Controller) return;

		BH_EIEDState state = m_Controller.BH_GetState();
		if (state != BH_EIEDState.BH_IED_ARMED)
		{
			// IED ya no está activa — parar beep si estaba activo
			if (m_bBeepActive && m_ActiveJammerVehicle)
			{
				BH_VehicleJammer jammer = BH_VehicleJammer.Cast(m_ActiveJammerVehicle.FindComponent(BH_VehicleJammer));
				if (jammer)
					jammer.BH_SetBeeping(false);
				m_bBeepActive = false;
			}

			GetGame().GetCallqueue().Remove(BH_CheckProximity);
			return;
		}

		IEntity owner = GetOwner();
		if (!owner) return;

		vector iedPos = owner.GetOrigin();

		// Buscar jammer activo en radio de la IED
		m_bJammerFound        = false;
		Vehicle prevJammerVehicle = m_ActiveJammerVehicle;
		m_ActiveJammerVehicle = null;
		GetGame().GetWorld().QueryEntitiesBySphere(iedPos, m_fJammerRadius, BH_JammerCallback, null, EQueryEntitiesFlags.ALL);

		// Gestionar beep
		if (m_bJammerFound && m_ActiveJammerVehicle)
		{
			if (!m_bBeepActive)
			{
				// Activar beep en el vehículo jammer
				BH_VehicleJammer jammer = BH_VehicleJammer.Cast(m_ActiveJammerVehicle.FindComponent(BH_VehicleJammer));
				if (jammer)
					jammer.BH_SetBeeping(true);
				m_bBeepActive = true;
			}
		}
		else
		{
			if (m_bBeepActive)
			{
				// Parar beep en el vehículo que lo tenía
				if (prevJammerVehicle)
				{
					BH_VehicleJammer jammer = BH_VehicleJammer.Cast(prevJammerVehicle.FindComponent(BH_VehicleJammer));
					if (jammer)
						jammer.BH_SetBeeping(false);
				}
				m_bBeepActive = false;
			}
		}

		// Buscar entidades en radio de detección
		GetGame().GetWorld().QueryEntitiesBySphere(iedPos, m_fDetectionRadius, BH_ProximityCallback, null, EQueryEntitiesFlags.ALL);
	}

	// --------------------------------------------------------
	// Callback de detección de entidades
	// --------------------------------------------------------

	protected bool BH_ProximityCallback(IEntity ent)
	{
		if (!ent)         return true;
		if (m_bTriggered) return false;

		// Ignorar la propia entidad (importante para VBIED)
		IEntity owner = GetOwner();
		if (owner && ent == owner) return true;

		// También ignorar por root parent (por si detecta una parte hija del owner)
		IEntity entRoot = ent.GetRootParent();
		if (entRoot && entRoot == owner) return true;
		if (owner)
		{
			IEntity ownerRoot = owner.GetRootParent();
			if (ownerRoot && (ent == ownerRoot || entRoot == ownerRoot)) return true;
		}

		if (m_bDetectInfantry)
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(ent);
			if (character && BH_IsAliveCharacter(character))
			{
				BH_IED_Utils.BH_Log(string.Format("[BH_IED_Proximity] Infantería detectada: %1", ent.GetName()));
				BH_HandleDetection(ent);
				return false;
			}
		}

		if (m_bDetectVehicles)
		{
			Vehicle vehicle = Vehicle.Cast(ent);
			if (vehicle)
			{
				BH_IED_Utils.BH_Log(string.Format("[BH_IED_Proximity] Vehículo detectado: %1", ent.GetName()));
				BH_HandleDetection(ent);
				return false;
			}
		}

		return true;
	}

	// --------------------------------------------------------
	// Gestión de detección — comprueba inhibidor antes de detonar
	// --------------------------------------------------------

	protected void BH_HandleDetection(IEntity ent)
	{
		BH_IED_Utils.BH_Log(string.Format("[BH_IED_Proximity] HandleDetection. JammerFound: %1", m_bJammerFound));

		if (m_bJammerFound)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Proximity] Inhibidor activo en radio — IED neutralizada.");
			return;
		}

		// Comprobar si hay cobertura de señal móvil (antena operativa)
		IEntity owner = GetOwner();
		if (owner)
		{
			bool hasCoverage = BH_SignalTower.BH_HasSignalCoverage(owner.GetOrigin());
			BH_IED_Utils.BH_Log(string.Format("[BH_IED_Proximity] Cobertura de señal IED: %1", hasCoverage));

			if (!hasCoverage)
			{
				BH_IED_Utils.BH_Log("[BH_IED_Proximity] Sin cobertura de señal — IED no puede recibir llamada.");
				return;
			}
		}

		// Comprobar si hay alguna IA con móvil operativa (viva + con cobertura)
		bool hasActiveCaller = BH_IED_Caller.BH_HasActiveCaller();
		BH_IED_Utils.BH_Log(string.Format("[BH_IED_Proximity] Caller activo: %1", hasActiveCaller));

		if (!hasActiveCaller)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Proximity] Ningún caller operativo — nadie puede activar la IED.");
			return;
		}

		BH_IED_Utils.BH_Log("[BH_IED_Proximity] Detonando IED.");
		m_bTriggered = true;

		// Parar beep antes de detonar
		if (m_bBeepActive && m_ActiveJammerVehicle)
		{
			BH_VehicleJammer jammer = BH_VehicleJammer.Cast(m_ActiveJammerVehicle.FindComponent(BH_VehicleJammer));
			if (jammer)
				jammer.BH_SetBeeping(false);
			m_bBeepActive = false;
		}

		m_Controller.BH_TriggerByProximity();
	}

	// --------------------------------------------------------
	// Búsqueda de jammer activo en radio
	// --------------------------------------------------------

	protected bool BH_JammerCallback(IEntity ent)
	{
		if (!ent) return true;

		Vehicle vehicle = Vehicle.Cast(ent);
		if (vehicle && BH_CheckVehicleJammer(vehicle))
		{
			BH_IED_Utils.BH_Log(string.Format("[BH_IED_Proximity] Vehículo jammer activo: %1", vehicle.GetName()));
			m_bJammerFound        = true;
			m_ActiveJammerVehicle = vehicle;
			return false;
		}

		return true;
	}

	protected bool BH_CheckVehicleJammer(Vehicle vehicle)
	{
		BH_VehicleJammer jammer = BH_VehicleJammer.Cast(vehicle.FindComponent(BH_VehicleJammer));
		if (jammer) return jammer.BH_IsActive();
		return false;
	}

	// --------------------------------------------------------
	// Helper — comprueba que el personaje está vivo
	// --------------------------------------------------------

	protected bool BH_IsAliveCharacter(ChimeraCharacter character)
	{
		DamageManagerComponent dmgMgr = DamageManagerComponent.Cast(character.FindComponent(DamageManagerComponent));
		if (!dmgMgr) return true;
		return !dmgMgr.IsDestroyed();
	}

	float BH_GetDetectionRadius() { return m_fDetectionRadius; }
}