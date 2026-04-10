// ============================================================
//  BH_EscortWatchdog.c
//  Supervisa la escolta de un NPC rescatado.
//
//  DESACTIVADO POR DEFECTO en el editor.
//  Si se activa, puede interferir con las órdenes del
//  command wheel cuando el NPC está en el slave group.
//
//  MODOS DE OPERACIÓN:
//
//  1) PASIVO (NPC en slave group):
//     Solo teleporta si la distancia supera el máximo
//     configurado en el editor (por defecto 150m).
//     No interfiere con órdenes ni waypoints del grupo.
//
//  2) ACTIVO (NPC sin grupo, solo waypoint Follow):
//     Teleporta si estancado, refresca waypoints.
// ============================================================

class BH_EscortWatchdog
{
	// ----------------------------------------------------------
	//  CONFIGURACIÓN: MODO ACTIVO (sin grupo)
	// ----------------------------------------------------------

	protected static const int   BH_WATCHDOG_INTERVAL_MS     = 3000;
	protected static const float BH_MAX_DISTANCE_ACTIVE      = 50.0;
	protected static const float BH_DESIRED_DISTANCE         = 2.5;
	protected static const float BH_TELEPORT_OFFSET          = 2.0;
	protected static const int   BH_STUCK_THRESHOLD          = 4;
	protected static const float BH_STUCK_MOVE_THRESHOLD     = 0.5;
	protected static const int   BH_WAYPOINT_REFRESH_MS      = 5000;

	// ----------------------------------------------------------
	//  CONFIGURACIÓN: MODO PASIVO (con slave group)
	// ----------------------------------------------------------

	protected static const int BH_WATCHDOG_INTERVAL_PASSIVE_MS = 10000;

	// ----------------------------------------------------------
	//  VARIABLES INTERNAS
	// ----------------------------------------------------------

	protected IEntity m_NPC;
	protected IEntity m_Player;
	protected BH_NPCInteractionComponent m_InteractionComp;

	protected vector m_vLastNPCPosition;
	protected int    m_iStuckCounter = 0;
	protected bool   m_bActive = false;
	protected bool   m_bPassiveMode = false;

	// Distancia máxima configurable desde el editor
	protected float  m_fMaxDistancePassive = 150.0;

	// ----------------------------------------------------------
	//  API PÚBLICA
	// ----------------------------------------------------------

	//! @param maxDistancePassive  Distancia máxima en modo pasivo (configurable desde editor)
	void BH_StartWatching(IEntity npc, IEntity player, BH_NPCInteractionComponent interaction, float maxDistancePassive = 150.0)
	{
		if (m_bActive)
			BH_StopWatching();

		if (!npc || !player || !interaction)
		{
			Print("[BH_Watchdog] ERROR: Parámetros nulos.", LogLevel.ERROR);
			return;
		}

		m_NPC                = npc;
		m_Player             = player;
		m_InteractionComp    = interaction;
		m_vLastNPCPosition   = npc.GetOrigin();
		m_iStuckCounter      = 0;
		m_bActive            = true;
		m_bPassiveMode       = false;
		m_fMaxDistancePassive = maxDistancePassive;

		Print(string.Format("[BH_Watchdog] Iniciando. Distancia máxima pasiva: %1m", m_fMaxDistancePassive), LogLevel.NORMAL);

		GetGame().GetCallqueue().CallLater(BH_WatchdogTick, BH_WATCHDOG_INTERVAL_MS, true);
		GetGame().GetCallqueue().CallLater(BH_WaypointRefreshTick, BH_WAYPOINT_REFRESH_MS, true);
	}

	void BH_StopWatching()
	{
		if (!m_bActive)
			return;

		m_bActive = false;
		m_bPassiveMode = false;

		GetGame().GetCallqueue().Remove(BH_WatchdogTick);
		GetGame().GetCallqueue().Remove(BH_WaypointRefreshTick);

		m_NPC             = null;
		m_Player          = null;
		m_InteractionComp = null;
		m_iStuckCounter   = 0;

		Print("[BH_Watchdog] Supervisión detenida.", LogLevel.NORMAL);
	}

	void BH_SetPassiveMode(bool passive)
	{
		if (m_bPassiveMode == passive)
			return;

		m_bPassiveMode = passive;
		m_iStuckCounter = 0;

		GetGame().GetCallqueue().Remove(BH_WatchdogTick);

		if (passive)
		{
			GetGame().GetCallqueue().CallLater(BH_WatchdogTick, BH_WATCHDOG_INTERVAL_PASSIVE_MS, true);
			GetGame().GetCallqueue().Remove(BH_WaypointRefreshTick);
			Print("[BH_Watchdog] Modo PASIVO.", LogLevel.NORMAL);
		}
		else
		{
			GetGame().GetCallqueue().CallLater(BH_WatchdogTick, BH_WATCHDOG_INTERVAL_MS, true);
			GetGame().GetCallqueue().CallLater(BH_WaypointRefreshTick, BH_WAYPOINT_REFRESH_MS, true);
			Print("[BH_Watchdog] Modo ACTIVO.", LogLevel.NORMAL);
		}
	}

	bool BH_IsActive()      { return m_bActive; }
	bool BH_IsPassiveMode() { return m_bPassiveMode; }

	void BH_SetPlayer(IEntity player)
	{
		m_Player = player;
	}

	// ----------------------------------------------------------
	//  TICK PRINCIPAL
	// ----------------------------------------------------------

	protected void BH_WatchdogTick()
	{
		if (!m_bActive || !m_NPC || !m_Player || !m_InteractionComp)
		{
			BH_StopWatching();
			return;
		}

		CharacterControllerComponent charCtrl = CharacterControllerComponent.Cast(
			m_NPC.FindComponent(CharacterControllerComponent)
		);
		if (charCtrl && charCtrl.IsDead())
		{
			Print("[BH_Watchdog] NPC muerto.", LogLevel.NORMAL);
			BH_StopWatching();
			return;
		}

		if (m_bPassiveMode)
			BH_TickPassive();
		else
			BH_TickActive();
	}

	// ----------------------------------------------------------
	//  MODO PASIVO: solo emergencia con distancia configurable
	// ----------------------------------------------------------

	protected void BH_TickPassive()
	{
		vector npcPos    = m_NPC.GetOrigin();
		vector playerPos = m_Player.GetOrigin();
		float  distance  = vector.Distance(npcPos, playerPos);

		if (distance > m_fMaxDistancePassive)
		{
			Print(string.Format("[BH_Watchdog] PASIVO: NPC perdido (%1m > %2m). Teleport de emergencia.",
				distance, m_fMaxDistancePassive), LogLevel.WARNING);
			BH_TeleportToPlayer();
		}
	}

	// ----------------------------------------------------------
	//  MODO ACTIVO: escolta con waypoint
	// ----------------------------------------------------------

	protected void BH_TickActive()
	{
		vector npcPos    = m_NPC.GetOrigin();
		vector playerPos = m_Player.GetOrigin();
		float  distance  = vector.Distance(npcPos, playerPos);

		float movedSinceLastTick = vector.Distance(npcPos, m_vLastNPCPosition);

		if (movedSinceLastTick < BH_STUCK_MOVE_THRESHOLD && distance > BH_DESIRED_DISTANCE)
			m_iStuckCounter++;
		else
			m_iStuckCounter = 0;

		m_vLastNPCPosition = npcPos;

		bool shouldTeleport = false;

		if (distance > BH_MAX_DISTANCE_ACTIVE)
		{
			Print(string.Format("[BH_Watchdog] ACTIVO: NPC lejos (%1m). Teleportando.", distance), LogLevel.NORMAL);
			shouldTeleport = true;
		}

		if (m_iStuckCounter >= BH_STUCK_THRESHOLD && distance > BH_DESIRED_DISTANCE)
		{
			Print(string.Format("[BH_Watchdog] ACTIVO: NPC estancado (%1 ciclos, %2m). Teleportando.",
				m_iStuckCounter, distance), LogLevel.NORMAL);
			shouldTeleport = true;
		}

		if (shouldTeleport)
		{
			BH_TeleportToPlayer();
			m_iStuckCounter = 0;
		}
	}

	// ----------------------------------------------------------
	//  TELEPORT
	// ----------------------------------------------------------

	protected void BH_TeleportToPlayer()
	{
		if (!m_NPC || !m_Player)
			return;

		vector playerPos = m_Player.GetOrigin();
		vector playerForward = m_Player.GetTransformAxis(2);
		vector spawnPos = playerPos - (playerForward * BH_TELEPORT_OFFSET);
		spawnPos[1] = SCR_TerrainHelper.GetTerrainY(spawnPos);

		if (!BH_IsPositionValid(spawnPos))
		{
			vector playerRight = m_Player.GetTransformAxis(0);
			spawnPos = playerPos + (playerRight * BH_TELEPORT_OFFSET);
			spawnPos[1] = SCR_TerrainHelper.GetTerrainY(spawnPos);
		}

		if (!BH_IsPositionValid(spawnPos))
		{
			spawnPos = playerPos;
			spawnPos[1] = SCR_TerrainHelper.GetTerrainY(spawnPos);
		}

		m_NPC.SetOrigin(spawnPos);

		vector dir = playerPos - spawnPos;
		dir.Normalize();
		if (dir.LengthSq() > 0.01)
		{
			float yaw = Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG;
			m_NPC.SetYawPitchRoll(Vector(yaw, 0, 0));
		}

		Print("[BH_Watchdog] NPC teleportado.", LogLevel.NORMAL);

		if (!m_bPassiveMode && m_InteractionComp)
			m_InteractionComp.BH_RefreshEscortWaypoint();
	}

	// ----------------------------------------------------------
	//  REFRESCO DE WAYPOINT (solo modo activo)
	// ----------------------------------------------------------

	protected void BH_WaypointRefreshTick()
	{
		if (!m_bActive || m_bPassiveMode || !m_InteractionComp || !m_Player)
			return;

		m_InteractionComp.BH_RefreshEscortWaypoint();
	}

	// ----------------------------------------------------------
	//  HELPERS
	// ----------------------------------------------------------

	protected bool BH_IsPositionValid(vector pos)
	{
		vector from = pos;
		from[1] = from[1] + 2.0;
		vector to = pos;
		to[1] = to[1] - 1.0;

		autoptr TraceParam trace = new TraceParam();
		trace.Start  = from;
		trace.End    = to;
		trace.Flags  = TraceFlags.WORLD;
		trace.LayerMask = EPhysicsLayerDefs.Terrain;

		float hitFraction = GetGame().GetWorld().TraceMove(trace, null);
		return (hitFraction < 1.0);
	}
}