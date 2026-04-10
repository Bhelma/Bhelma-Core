// ============================================================================
// BH_FireSupportHelicopterManager.c
//
// Descripción:
//   Gestiona los tres modos de helicóptero del sistema de Soporte de Fuego:
//     - ATAQUE:     Spawn en aeródromo → volar al objetivo → atacar → despawn
//     - SUMINISTRO: Spawn en aeródromo → volar al LZ → soltar caja + humo morado → despawn
//     - EXTRACCIÓN: Spawn en aeródromo → volar al LZ → esperar jugadores →
//                   esperar orden de partida → volar al destino final → despawn
//
//   Reutiliza BH_AmbientHelicopterControllerComponent como sistema de vuelo.
//   Los waypoints se crean como entidades temporales nombradas en el mundo,
//   ya que el sistema de vuelo los resuelve por nombre con FindEntityByName.
//
// Autor: Bhelma
//
// Dónde se añade:
//   Al GameMode de la misión como ScriptComponent.
//   Solo debe haber UNA instancia en la partida.
//
// Dependencias:
//   - BH_FireSupportComponent.c
//   - BH_AmbientHelicopterControllerComponent.c
//   - BH_FireSupportHUD.c
// ============================================================================

// ---------------------------------------------------------------------------
// Enum: modo de misión activa
// ---------------------------------------------------------------------------
enum BH_EHeliMissionMode
{
	ATTACK   = 0,
	SUPPLY   = 1,
	EXTRACT  = 2
}

// ---------------------------------------------------------------------------
// Datos de una misión de helicóptero activa
// ---------------------------------------------------------------------------
class BH_HeliMissionData
{
	BH_EHeliMissionMode     m_eMode;
	vector                  m_vTargetPos;
	vector                  m_vSpawnPos;
	vector                  m_vDeparturePos;  // para extracción: destino final
	ResourceName            m_sHeliPrefab;
	ResourceName            m_sPilotPrefab;
	ResourceName            m_sCoPilotPrefab;
	ResourceName            m_sSupplyBoxPrefab;
	float                   m_fDespawnDistance;
	BH_FireSupportComponent m_pOwner;
	RplId                   m_RequesterRadioId; // para extracción

	// Entidades spawneadas (para limpieza)
	IEntity                 m_pHeliEntity;
	ref array<IEntity>      m_aWaypointEntities = new array<IEntity>();

	// Estado de extracción
	bool                    m_bExtractionDepartOrdered;
	bool                    m_bSupplyDropped;
}

// ---------------------------------------------------------------------------
// Class descriptor
// ---------------------------------------------------------------------------
[EntityEditorProps(category: "BH/FireSupport/", description: "Helicopter Support Manager", color: "0 180 255 255")]
class BH_FireSupportHelicopterManagerClass : ScriptComponentClass
{
}

// ---------------------------------------------------------------------------
// Manager
// ---------------------------------------------------------------------------
class BH_FireSupportHelicopterManager : ScriptComponent
{
	// -----------------------------------------------------------------------
	// ATRIBUTOS
	// -----------------------------------------------------------------------

	[Attribute(defvalue: "0",
		desc: "Activar logs de debug en consola.",
		category: "BH Debug")]
	bool m_bBH_Debug;

	[Attribute("{8B8AC3AC80C2FF88}Prefabs/Items/Equipment/Supplies/SupplyCrate_US.et",
		UIWidgets.ResourcePickerThumbnail,
		desc: "Prefab de humo morado para marcar el suministro. Se spawnea junto a la caja.",
		params: "et",
		category: "BH Fire Support - Helicopters")]
	protected ResourceName m_sPurpleSmokeGrenPrefab;

	[Attribute("120", UIWidgets.EditBox,
		desc: "Segundos máximos de espera del heli de extracción antes de partir sin los jugadores.",
		params: "30 600",
		category: "BH Fire Support - Helicopters")]
	protected int m_iExtractionWaitTimeout;

	[Attribute("200", UIWidgets.EditBox,
		desc: "Altitud extra sobre el terreno (metros) para el waypoint de despawn alejado.",
		params: "50 500",
		category: "BH Fire Support - Helicopters")]
	protected float m_fDespawnAltitude;

	// -----------------------------------------------------------------------
	// Estado interno
	// -----------------------------------------------------------------------

	protected static BH_FireSupportHelicopterManager s_Instance;

	// Misión activa
	protected ref BH_HeliMissionData m_pActiveMission;

	// Contador para nombres únicos de waypoints temporales
	protected static int s_WPNameCounter = 0;

	// Timer de extracción
	protected float m_fExtractionTimer;
	protected bool  m_bExtractionTimerActive;

	// -----------------------------------------------------------------------
	// LIFECYCLE
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		BH_Log("OnPostInit: BH_FireSupportHelicopterManager listo.");
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		BH_CleanupMission();
		s_Instance = null;
		super.OnDelete(owner);
	}

	// -----------------------------------------------------------------------
	// API PÚBLICA
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	static BH_FireSupportHelicopterManager GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	void BH_LaunchAttack(
		vector targetPos,
		vector spawnPos,
		ResourceName heliPrefab,
		ResourceName pilotPrefab,
		ResourceName coPilotPrefab,
		float despawnDist,
		BH_FireSupportComponent owner)
	{
		BH_Log("BH_LaunchAttack: target=" + targetPos);

		if (RplSession.Mode() == RplMode.Client)
			return;

		BH_HeliMissionData mission = new BH_HeliMissionData();
		mission.m_eMode           = BH_EHeliMissionMode.ATTACK;
		mission.m_vTargetPos      = targetPos;
		mission.m_vSpawnPos       = spawnPos;
		mission.m_sHeliPrefab     = heliPrefab;
		mission.m_sPilotPrefab    = pilotPrefab;
		mission.m_sCoPilotPrefab  = coPilotPrefab;
		mission.m_fDespawnDistance = despawnDist;
		mission.m_pOwner          = owner;

		m_pActiveMission = mission;
		BH_SpawnHelicopter();
	}

	//------------------------------------------------------------------------------------------------
	void BH_LaunchSupply(
		vector targetPos,
		vector spawnPos,
		ResourceName heliPrefab,
		ResourceName pilotPrefab,
		ResourceName coPilotPrefab,
		ResourceName supplyBoxPrefab,
		float despawnDist,
		BH_FireSupportComponent owner)
	{
		BH_Log("BH_LaunchSupply: target=" + targetPos);

		if (RplSession.Mode() == RplMode.Client)
			return;

		BH_HeliMissionData mission = new BH_HeliMissionData();
		mission.m_eMode            = BH_EHeliMissionMode.SUPPLY;
		mission.m_vTargetPos       = targetPos;
		mission.m_vSpawnPos        = spawnPos;
		mission.m_sHeliPrefab      = heliPrefab;
		mission.m_sPilotPrefab     = pilotPrefab;
		mission.m_sCoPilotPrefab   = coPilotPrefab;
		mission.m_sSupplyBoxPrefab = supplyBoxPrefab;
		mission.m_fDespawnDistance  = despawnDist;
		mission.m_pOwner           = owner;

		m_pActiveMission = mission;
		BH_SpawnHelicopter();
	}

	//------------------------------------------------------------------------------------------------
	void BH_LaunchExtraction(
		vector targetPos,
		vector spawnPos,
		ResourceName heliPrefab,
		ResourceName pilotPrefab,
		ResourceName coPilotPrefab,
		float despawnDist,
		RplId requesterRadioId,
		BH_FireSupportComponent owner)
	{
		BH_Log("BH_LaunchExtraction: LZ=" + targetPos);

		if (RplSession.Mode() == RplMode.Client)
			return;

		BH_HeliMissionData mission = new BH_HeliMissionData();
		mission.m_eMode              = BH_EHeliMissionMode.EXTRACT;
		mission.m_vTargetPos         = targetPos;
		mission.m_vSpawnPos          = spawnPos;
		mission.m_sHeliPrefab        = heliPrefab;
		mission.m_sPilotPrefab       = pilotPrefab;
		mission.m_sCoPilotPrefab     = coPilotPrefab;
		mission.m_fDespawnDistance    = despawnDist;
		mission.m_RequesterRadioId   = requesterRadioId;
		mission.m_pOwner             = owner;
		mission.m_bExtractionDepartOrdered = false;

		m_pActiveMission = mission;
		BH_SpawnHelicopter();
	}

	//------------------------------------------------------------------------------------------------
	// Llamado desde BH_FireSupportComponent cuando el jugador marca el destino final
	void BH_SetExtractionDestination(vector destinationPos)
	{
		if (!m_pActiveMission)
			return;

		if (m_pActiveMission.m_eMode != BH_EHeliMissionMode.EXTRACT)
			return;

		BH_Log("BH_SetExtractionDestination: destino=" + destinationPos);

		m_pActiveMission.m_vDeparturePos = destinationPos;
		m_pActiveMission.m_bExtractionDepartOrdered = true;

		// Detener el timer de espera
		m_bExtractionTimerActive = false;
		GetGame().GetCallqueue().Remove(BH_ExtractionTimerTick);

		// Añadir waypoints de salida al heli
		BH_AddExtractionDepartureWaypoints();
	}

	// -----------------------------------------------------------------------
	// SPAWN DEL HELICÓPTERO
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_SpawnHelicopter()
	{
		if (!m_pActiveMission)
			return;

		ResourceName heliPrefab = m_pActiveMission.m_sHeliPrefab;
		if (heliPrefab == ResourceName.Empty)
		{
			BH_Log("BH_SpawnHelicopter: ERROR — prefab de helicóptero no configurado.");
			BH_MissionFailed();
			return;
		}

		Resource res = Resource.Load(heliPrefab);
		if (!res || !res.IsValid())
		{
			BH_Log("BH_SpawnHelicopter: ERROR — no se pudo cargar prefab: " + heliPrefab);
			BH_MissionFailed();
			return;
		}

		// Calcular orientación mirando hacia el objetivo desde el spawn
		vector spawnPos = m_pActiveMission.m_vSpawnPos;
		BaseWorld world = GetGame().GetWorld();
		if (world)
			spawnPos[1] = world.GetSurfaceY(spawnPos[0], spawnPos[2]) + 10;

		vector dir = m_pActiveMission.m_vTargetPos - spawnPos;
		dir[1] = 0;
		dir.Normalize();
		float yaw = Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		vector mat[4];
		Math3D.AnglesToMatrix(Vector(yaw, 0, 0), mat);
		mat[3] = spawnPos;
		spawnParams.Transform = mat;

		IEntity heliEnt = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), spawnParams);
		if (!heliEnt)
		{
			BH_Log("BH_SpawnHelicopter: ERROR — spawn fallido.");
			BH_MissionFailed();
			return;
		}

		m_pActiveMission.m_pHeliEntity = heliEnt;
		BH_Log("BH_SpawnHelicopter: helicóptero spawneado en " + spawnPos);

		// Configurar el controlador de vuelo tras un pequeño delay (igual que SlotBase)
		GetGame().GetCallqueue().CallLater(BH_InitHelicopterController, 500, false);
	}

	// -----------------------------------------------------------------------
	// CONFIGURACIÓN DEL CONTROLADOR DE VUELO
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_InitHelicopterController()
	{
		if (!m_pActiveMission || !m_pActiveMission.m_pHeliEntity)
			return;

		BH_AmbientHelicopterControllerComponent ctrl = BH_AmbientHelicopterControllerComponent.Cast(
			m_pActiveMission.m_pHeliEntity.FindComponent(BH_AmbientHelicopterControllerComponent));

		if (!ctrl)
		{
			BH_Log("BH_InitHelicopterController: ERROR — BH_AmbientHelicopterControllerComponent no encontrado en el prefab.");
			BH_MissionFailed();
			return;
		}

		// Configurar tripulación
		ctrl.BH_UpdateCrewSettings(
			m_pActiveMission.m_sPilotPrefab,
			m_pActiveMission.m_sCoPilotPrefab,
			true,   // tripulación invulnerable
			false,  // sin artilleros para utilidad/extracción
			0,
			ResourceName.Empty
		);

		// Para ataque: activar modo de ataque a jugadores
		bool attackPlayers = false;
		if (m_pActiveMission.m_eMode == BH_EHeliMissionMode.ATTACK)
			attackPlayers = true;

		// Configurar parámetros de vuelo
		ctrl.BH_UpdateSimSettings(
			100,    // velocidad máxima
			attackPlayers,
			120,    // timeout de ataque
			35,     // altura mínima sobre terreno
			40,     // distancia mínima a waypoint
			2.0,    // rotación base
			15.0,   // divisor rotación
			0.5     // reducción de daño
		);

		ctrl.BH_UpdateLightSettings(true, true, false, false);
		ctrl.BH_UpdateLockSetting(true); // bloqueado para jugadores salvo extracción
		ctrl.BH_SetDebugMode(m_bBH_Debug);

		// Crear y añadir waypoints según el modo
		BH_BuildWaypoints(ctrl);
	}

	// -----------------------------------------------------------------------
	// CONSTRUCCIÓN DE WAYPOINTS
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Crea entidades de posición temporales nombradas en el mundo y las
	// registra como waypoints en el controlador de vuelo.
	protected void BH_BuildWaypoints(BH_AmbientHelicopterControllerComponent ctrl)
	{
		if (!m_pActiveMission)
			return;

		if (m_pActiveMission.m_eMode == BH_EHeliMissionMode.ATTACK)
		{
			BH_BuildAttackWaypoints(ctrl);
		}
		else if (m_pActiveMission.m_eMode == BH_EHeliMissionMode.SUPPLY)
		{
			BH_BuildSupplyWaypoints(ctrl);
		}
		else if (m_pActiveMission.m_eMode == BH_EHeliMissionMode.EXTRACT)
		{
			BH_BuildExtractionWaypoints(ctrl);
		}
	}

	//------------------------------------------------------------------------------------------------
	// ATAQUE: spawn → objetivo (con ataque activo) → alejarse → despawn
	protected void BH_BuildAttackWaypoints(BH_AmbientHelicopterControllerComponent ctrl)
	{
		// WP0: objetivo (ataque)
		BH_AddWPToController(ctrl, m_pActiveMission.m_vTargetPos, false, false, false, false);

		// WP1: punto de salida (2km en dirección contraria al spawn)
		vector exitDir = m_pActiveMission.m_vSpawnPos - m_pActiveMission.m_vTargetPos;
		exitDir[1] = 0;
		exitDir.Normalize();
		vector exitPos = m_pActiveMission.m_vTargetPos + exitDir * m_pActiveMission.m_fDespawnDistance;
		BH_AddWPToController(ctrl, exitPos, false, false, false, true); // DeleteHelicopter en el último WP
	}

	//------------------------------------------------------------------------------------------------
	// SUMINISTRO: spawn → LZ (aterrizar) → soltar caja → salir → despawn
	protected void BH_BuildSupplyWaypoints(BH_AmbientHelicopterControllerComponent ctrl)
	{
		// WP0: LZ — aterrizar, esperar 10s, soltar carga y salir
		BH_AddWPToController(ctrl, m_pActiveMission.m_vTargetPos, true, false, false, false);

		// WP1: punto de salida → despawn
		vector exitDir = m_pActiveMission.m_vSpawnPos - m_pActiveMission.m_vTargetPos;
		exitDir[1] = 0;
		exitDir.Normalize();
		vector exitPos = m_pActiveMission.m_vTargetPos + exitDir * m_pActiveMission.m_fDespawnDistance;
		BH_AddWPToController(ctrl, exitPos, false, false, false, true);

		// Programar el drop de suministros cuando el heli llegue al LZ
		// (se comprueba por proximidad en el loop de monitorización)
		GetGame().GetCallqueue().CallLater(BH_MonitorSupplyDrop, 2000, true);
	}

	//------------------------------------------------------------------------------------------------
	// EXTRACCIÓN: spawn → LZ (aterrizar, esperar jugadores) → destino final → despawn
	// El destino final se añade después cuando el jugador lo marca en el mapa
	protected void BH_BuildExtractionWaypoints(BH_AmbientHelicopterControllerComponent ctrl)
	{
		// WP0: LZ — aterrizar y esperar embarque
		BH_AddWPToController(ctrl, m_pActiveMission.m_vTargetPos, true, true, false, false);

		// No añadimos WP de salida todavía — se añaden cuando el jugador
		// ordena la partida con BH_SetExtractionDestination()

		// Arrancar el timer de espera máxima
		m_fExtractionTimer = m_iExtractionWaitTimeout;
		m_bExtractionTimerActive = true;
		GetGame().GetCallqueue().CallLater(BH_ExtractionTimerTick, 1000, true);

		// Abrir el heli para que puedan subir
		ctrl.BH_UpdateLockSetting(false);

		// Notificar a clientes
		Rpc(RPC_ClientNotifyExtractionArrived);
	}

	//------------------------------------------------------------------------------------------------
	// Añade un waypoint al controlador creando una entidad de posición temporal
	protected void BH_AddWPToController(
		BH_AmbientHelicopterControllerComponent ctrl,
		vector pos,
		bool isLanding,
		bool waitForGetIn,
		bool waitForGetOut,
		bool deleteHeli)
	{
		if (!ctrl || !m_pActiveMission)
			return;

		// Generar nombre único para esta entidad de waypoint
		s_WPNameCounter++;
		string wpName = "BH_FireSupportWP_" + s_WPNameCounter.ToString();

		// Spawnear entidad de posición temporal en el mundo
		IEntity wpEnt = BH_SpawnPositionEntity(pos, wpName);
		if (!wpEnt)
		{
			BH_Log("BH_AddWPToController: ERROR — no se pudo crear entidad de waypoint '" + wpName + "'");
			return;
		}

		// Registrar para limpieza posterior
		m_pActiveMission.m_aWaypointEntities.Insert(wpEnt);

		// Construir datos del waypoint
		BH_AmbientHelicopterWaypoint wpData = new BH_AmbientHelicopterWaypoint();
		wpData.m_WaypoinName        = wpName;
		wpData.m_IsLandingEvent     = isLanding;
		wpData.m_WaitForPlayersGetIn = waitForGetIn;
		wpData.m_WaitForPlayersGetOut = waitForGetOut;
		wpData.m_DeleteHelicopter   = deleteHeli;
		wpData.m_WaitSeconds        = 10;
		wpData.m_ShutdownEngine     = false;

		ctrl.BH_AddWaypoint(wpData);

		BH_Log("BH_AddWPToController: WP '" + wpName + "' en " + pos + " landing=" + isLanding + " delete=" + deleteHeli);
	}

	//------------------------------------------------------------------------------------------------
	// Crea una entidad GenericEntity como marcador de posición para el waypoint
	protected IEntity BH_SpawnPositionEntity(vector pos, string name)
	{
		// Ajustar Y al terreno
		BaseWorld world = GetGame().GetWorld();
		if (world)
			pos[1] = world.GetSurfaceY(pos[0], pos[2]);

		// Crear una entidad vacía directamente sin prefab
		// Es lo más seguro — no depende de ningún GUID externo
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		mat[3] = pos;
		params.Transform = mat;

		// Usar el prefab de waypoint de IA que siempre existe en Reforger
		Resource res = Resource.Load("{750A8D1695BD6998}AI/Entities/Waypoints/AIWaypoint.et");
		if (!res || !res.IsValid())
		{
			BH_Log("BH_SpawnPositionEntity: ERROR — no se pudo cargar prefab de posición.");
			return null;
		}

		IEntity ent = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (!ent)
			return null;

		ent.SetName(name);
		BH_Log("BH_SpawnPositionEntity: entidad '" + name + "' creada en " + pos);
		return ent;
	}

	// -----------------------------------------------------------------------
	// LÓGICA DE SUMINISTRO — Detectar llegada y soltar carga
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Comprueba cada 2s si el heli de suministro ha llegado al LZ
	protected void BH_MonitorSupplyDrop()
	{
		if (!m_pActiveMission || m_pActiveMission.m_eMode != BH_EHeliMissionMode.SUPPLY)
		{
			GetGame().GetCallqueue().Remove(BH_MonitorSupplyDrop);
			return;
		}

		if (m_pActiveMission.m_bSupplyDropped)
		{
			GetGame().GetCallqueue().Remove(BH_MonitorSupplyDrop);
			return;
		}

		if (!m_pActiveMission.m_pHeliEntity)
			return;

		float dist = vector.DistanceXZ(
			m_pActiveMission.m_pHeliEntity.GetOrigin(),
			m_pActiveMission.m_vTargetPos);

		// Cuando el heli esté a menos de 30m del LZ, soltar la carga
		if (dist < 30)
		{
			m_pActiveMission.m_bSupplyDropped = true;
			GetGame().GetCallqueue().Remove(BH_MonitorSupplyDrop);
			BH_DropSupplies();
		}
	}

	//------------------------------------------------------------------------------------------------
	// Spawnea la caja de suministros y el humo morado en el LZ
	protected void BH_DropSupplies()
	{
		if (!m_pActiveMission)
			return;

		vector dropPos = m_pActiveMission.m_vTargetPos;
		BaseWorld world = GetGame().GetWorld();
		if (world)
			dropPos[1] = world.GetSurfaceY(dropPos[0], dropPos[2]);

		BH_Log("BH_DropSupplies: soltando carga en " + dropPos);

		// Spawnear caja de suministros
		ResourceName supplyPrefab = m_pActiveMission.m_sSupplyBoxPrefab;
		if (supplyPrefab != ResourceName.Empty)
		{
			Resource supplyRes = Resource.Load(supplyPrefab);
			if (supplyRes && supplyRes.IsValid())
			{
				EntitySpawnParams supplyParams = new EntitySpawnParams();
				supplyParams.TransformMode = ETransformMode.WORLD;
				vector supplyMat[4];
				Math3D.MatrixIdentity4(supplyMat);
				supplyMat[3] = dropPos;
				supplyParams.Transform = supplyMat;

				GetGame().SpawnEntityPrefab(supplyRes, GetGame().GetWorld(), supplyParams);
				BH_Log("BH_DropSupplies: caja spawneada.");
			}
		}

		// Spawnear humo morado junto a la caja
		if (m_sPurpleSmokeGrenPrefab != ResourceName.Empty)
		{
			Resource smokeRes = Resource.Load(m_sPurpleSmokeGrenPrefab);
			if (smokeRes && smokeRes.IsValid())
			{
				// Offset de 2m para que sea visible junto a la caja
				vector smokePos = dropPos + Vector(2, 0, 0);

				EntitySpawnParams smokeParams = new EntitySpawnParams();
				smokeParams.TransformMode = ETransformMode.WORLD;
				vector smokeMat[4];
				Math3D.MatrixIdentity4(smokeMat);
				smokeMat[3] = smokePos;
				smokeParams.Transform = smokeMat;

				GetGame().SpawnEntityPrefab(smokeRes, GetGame().GetWorld(), smokeParams);
				BH_Log("BH_DropSupplies: humo morado spawneado.");
			}
		}

		// Notificar a clientes
		Rpc(RPC_ClientNotifySupplyDrop, dropPos);
	}

	// -----------------------------------------------------------------------
	// LÓGICA DE EXTRACCIÓN — Timer y waypoints de salida
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Tick del timer de espera de extracción (cada 1s)
	protected void BH_ExtractionTimerTick()
	{
		if (!m_bExtractionTimerActive)
			return;

		m_fExtractionTimer -= 1;

		BH_Log("BH_ExtractionTimerTick: tiempo restante=" + m_fExtractionTimer + "s");

		if (m_fExtractionTimer <= 0)
		{
			m_bExtractionTimerActive = false;
			GetGame().GetCallqueue().Remove(BH_ExtractionTimerTick);
			BH_Log("BH_ExtractionTimerTick: tiempo agotado, heli partiendo sin jugadores.");
			BH_ExtractionTimeout();
		}
	}

	//------------------------------------------------------------------------------------------------
	// El tiempo de espera se agotó — el heli parte hacia un punto seguro y despawnea
	protected void BH_ExtractionTimeout()
	{
		if (!m_pActiveMission)
			return;

		// Notificar a clientes
		Rpc(RPC_ClientNotifyExtractionTimeout);

		// Añadir waypoint de salida simple (volver hacia el spawn y despawnear)
		BH_AmbientHelicopterControllerComponent ctrl = BH_GetActiveController();
		if (!ctrl)
		{
			BH_MissionFailed();
			return;
		}

		vector exitPos = m_pActiveMission.m_vSpawnPos;
		BH_AddWPToController(ctrl, exitPos, false, false, false, true);
		BH_MissionComplete();
	}

	//------------------------------------------------------------------------------------------------
	// Añade los waypoints de salida cuando el jugador ordena la partida
	protected void BH_AddExtractionDepartureWaypoints()
	{
		if (!m_pActiveMission)
			return;

		BH_AmbientHelicopterControllerComponent ctrl = BH_GetActiveController();
		if (!ctrl)
		{
			BH_MissionFailed();
			return;
		}

		// WP: destino final marcado por el jugador
		BH_AddWPToController(ctrl, m_pActiveMission.m_vDeparturePos, true, false, true, false);

		// WP: punto de salida alejado → despawn
		vector exitDir = m_pActiveMission.m_vSpawnPos - m_pActiveMission.m_vDeparturePos;
		exitDir[1] = 0;
		exitDir.Normalize();
		vector exitPos = m_pActiveMission.m_vDeparturePos + exitDir * m_pActiveMission.m_fDespawnDistance;
		BH_AddWPToController(ctrl, exitPos, false, false, false, true);

		BH_MissionComplete();

		// Notificar a clientes
		Rpc(RPC_ClientNotifyExtractionDeparting);
	}

	// -----------------------------------------------------------------------
	// FINALIZACIÓN DE MISIÓN
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// La misión concluyó correctamente — liberar bloqueo global
	protected void BH_MissionComplete()
	{
		BH_Log("BH_MissionComplete.");

		if (m_pActiveMission && m_pActiveMission.m_pOwner)
			m_pActiveMission.m_pOwner.BH_ReleaseGlobalLock();

		// NO limpiamos m_pActiveMission aquí porque el heli sigue volando
		// y usando las entidades de waypoint. La limpieza ocurre cuando el
		// sistema de vuelo destruye el heli (DeleteHelicopter en el último WP).
		// Programamos una limpieza diferida de las entidades de waypoint.
		GetGame().GetCallqueue().CallLater(BH_DeferredCleanup, 120000, false); // 2 min
	}

	//------------------------------------------------------------------------------------------------
	// La misión falló — liberar bloqueo y limpiar
	protected void BH_MissionFailed()
	{
		BH_Log("BH_MissionFailed.");

		if (m_pActiveMission && m_pActiveMission.m_pOwner)
			m_pActiveMission.m_pOwner.BH_ReleaseGlobalLock();

		BH_CleanupMission();
	}

	//------------------------------------------------------------------------------------------------
	// Limpieza diferida — borra las entidades de waypoint temporales
	protected void BH_DeferredCleanup()
	{
		BH_CleanupMission();
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_CleanupMission()
	{
		if (!m_pActiveMission)
			return;

		// Borrar entidades de waypoint temporales
		foreach (IEntity wpEnt : m_pActiveMission.m_aWaypointEntities)
		{
			if (wpEnt)
				SCR_EntityHelper.DeleteEntityAndChildren(wpEnt);
		}

		// Detener timers activos
		m_bExtractionTimerActive = false;
		GetGame().GetCallqueue().Remove(BH_ExtractionTimerTick);
		GetGame().GetCallqueue().Remove(BH_MonitorSupplyDrop);
		GetGame().GetCallqueue().Remove(BH_DeferredCleanup);

		m_pActiveMission = null;
		BH_Log("BH_CleanupMission: limpieza completada.");
	}

	// -----------------------------------------------------------------------
	// UTILIDADES
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Obtiene el controlador de vuelo del heli activo
	protected BH_AmbientHelicopterControllerComponent BH_GetActiveController()
	{
		if (!m_pActiveMission || !m_pActiveMission.m_pHeliEntity)
			return null;

		return BH_AmbientHelicopterControllerComponent.Cast(
			m_pActiveMission.m_pHeliEntity.FindComponent(BH_AmbientHelicopterControllerComponent));
	}

	// -----------------------------------------------------------------------
	// RPC — Servidor → Clientes (notificaciones HUD)
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ClientNotifySupplyDrop(vector pos)
	{
		if (System.IsConsoleApp())
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		BH_FireSupportHUD hud = BH_FireSupportHUD.Cast(pc.FindComponent(BH_FireSupportHUD));
		if (hud)
			hud.BH_ShowExternalNotification("Suministros entregados en el LZ");
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ClientNotifyExtractionArrived()
	{
		if (System.IsConsoleApp())
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		BH_FireSupportHUD hud = BH_FireSupportHUD.Cast(pc.FindComponent(BH_FireSupportHUD));
		if (hud)
			hud.BH_ShowExternalNotification("Helicóptero de extracción en el LZ — ¡Embarca!");
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ClientNotifyExtractionDeparting()
	{
		if (System.IsConsoleApp())
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		BH_FireSupportHUD hud = BH_FireSupportHUD.Cast(pc.FindComponent(BH_FireSupportHUD));
		if (hud)
			hud.BH_ShowExternalNotification("Helicóptero partiendo hacia el destino final");
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ClientNotifyExtractionTimeout()
	{
		if (System.IsConsoleApp())
			return;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		BH_FireSupportHUD hud = BH_FireSupportHUD.Cast(pc.FindComponent(BH_FireSupportHUD));
		if (hud)
			hud.BH_ShowExternalNotification("Tiempo agotado — el helicóptero de extracción se ha marchado");
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_Log(string msg)
	{
		if (!m_bBH_Debug)
			return;

		Print("[BH_FireSupportHelicopterManager] " + msg, LogLevel.NORMAL);
	}
}