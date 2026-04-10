// ============================================================================
// BH_ArtillerySupportManager.c
//
// Descripción:
//   Gestiona las misiones de soporte de artillería del sistema de Soporte de
//   Fuego por Radio. Se añade al GameMode de la misión como componente.
//   Toda la lógica se ejecuta SOLO en el servidor.
//
//   Flujo:
//     1. BH_FireSupportComponent llama a BH_LaunchMission() en el servidor
//     2. Se espera el delay configurado (simula tiempo de recarga y viaje)
//     3. Se lanzan N impactos uno a uno con el intervalo configurado
//     4. Cada impacto: posición aleatoria dentro del radio + spawn del prefab
//        de explosión + RPC a clientes para reproducir sonido de llegada
//     5. Al finalizar la misión se libera el bloqueo global
//
// Autor: Bhelma
//
// Dónde se añade:
//   Al GameMode de la misión, como componente adicional (ScriptComponent).
//   Solo debe haber UNA instancia en la partida.
//
// Dependencias:
//   - BH_FireSupportComponent.c
// ============================================================================

// ---------------------------------------------------------------------------
// Datos internos de una misión de artillería activa
// ---------------------------------------------------------------------------
class BH_ArtilleryMissionData
{
	vector             m_vTargetPos;
	int                m_iRoundsTotal;
	int                m_iRoundsFired;
	float              m_fRadius;
	float              m_fImpactInterval;
	BH_FireSupportComponent m_pOwner;
}

// ---------------------------------------------------------------------------
// Class descriptor
// ---------------------------------------------------------------------------
[EntityEditorProps(category: "BH/FireSupport/", description: "Artillery Support Manager", color: "255 80 0 255")]
class BH_ArtillerySupportManagerClass : ScriptComponentClass
{
}

// ---------------------------------------------------------------------------
// Manager
// ---------------------------------------------------------------------------
class BH_ArtillerySupportManager : ScriptComponent
{
	// -----------------------------------------------------------------------
	// ATRIBUTOS
	// -----------------------------------------------------------------------

	[Attribute(defvalue: "0",
		desc: "Activar logs de debug en consola.",
		category: "BH Debug")]
	bool m_bBH_Debug;

	[Attribute("1.5", UIWidgets.EditBox,
		desc: "Segundos de delay entre el sonido de llegada del proyectil y la explosión.",
		params: "0.1 5",
		category: "BH Fire Support - Artillery")]
	protected float m_fShellIncomingDelay;

	// -----------------------------------------------------------------------
	// Estado interno
	// -----------------------------------------------------------------------

	// Instancia singleton accesible desde BH_FireSupportComponent
	protected static BH_ArtillerySupportManager s_Instance;

	// Misión activa (solo una simultánea por diseño)
	protected ref BH_ArtilleryMissionData m_pActiveMission;

	// Indica si hay un impacto en curso (para no solapar CallLater)
	protected bool m_bFiringActive;

	// -----------------------------------------------------------------------
	// LIFECYCLE
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		BH_Log("OnPostInit: BH_ArtillerySupportManager listo.");
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		BH_StopFiring();
		s_Instance = null;
		super.OnDelete(owner);
	}

	// -----------------------------------------------------------------------
	// API PÚBLICA
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	static BH_ArtillerySupportManager GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	// Lanza una misión de artillería. Llamado desde BH_FireSupportComponent en servidor.
	void BH_LaunchMission(
		vector targetPos,
		int    delay,
		int    rounds,
		float  radius,
		float  impactInterval,
		BH_FireSupportComponent owner)
	{
		BH_Log("BH_LaunchMission: target=" + targetPos + " delay=" + delay + "s rounds=" + rounds);

		// Solo en servidor
		if (RplSession.Mode() == RplMode.Client)
			return;

		// Guardar datos de la misión
		m_pActiveMission = new BH_ArtilleryMissionData();
		m_pActiveMission.m_vTargetPos       = targetPos;
		m_pActiveMission.m_iRoundsTotal     = rounds;
		m_pActiveMission.m_iRoundsFired     = 0;
		m_pActiveMission.m_fRadius          = radius;
		m_pActiveMission.m_fImpactInterval  = impactInterval;
		m_pActiveMission.m_pOwner           = owner;

		m_bFiringActive = true;

		// Notificar a los clientes del delay inminente
		RplComponent rplComp = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (rplComp)
			Rpc(RPC_ClientNotifyIncoming, delay);

		// Esperar el delay antes del primer impacto
		GetGame().GetCallqueue().CallLater(BH_FireNextRound, delay * 1000, false);
	}

	// -----------------------------------------------------------------------
	// LÓGICA DE SERVIDOR — Secuencia de impactos
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Dispara el siguiente proyectil de la misión activa
	protected void BH_FireNextRound()
	{
		if (!m_pActiveMission || !m_bFiringActive)
			return;

		if (m_pActiveMission.m_iRoundsFired >= m_pActiveMission.m_iRoundsTotal)
		{
			BH_MissionComplete();
			return;
		}

		// Calcular posición aleatoria dentro del radio
		float angle  = Math.RandomFloat(0, 360) * Math.DEG2RAD;
		float dist   = Math.RandomFloat(0, m_pActiveMission.m_fRadius);
		float offsetX = Math.Sin(angle) * dist;
		float offsetZ = Math.Cos(angle) * dist;

		vector impactPos;
		impactPos[0] = m_pActiveMission.m_vTargetPos[0] + offsetX;
		impactPos[2] = m_pActiveMission.m_vTargetPos[2] + offsetZ;

		// Ajustar Y al terreno
		BaseWorld world = GetGame().GetWorld();
		if (world)
			impactPos[1] = world.GetSurfaceY(impactPos[0], impactPos[2]);
		else
			impactPos[1] = m_pActiveMission.m_vTargetPos[1];

		// Notificar a clientes para reproducir sonido de llegada ("incoming!")
		// (eliminado — el sonido lo gestiona el prefab de explosión)

		// Tras el delay de llegada, spawnear la explosión
		// Necesitamos pasar la posición; usamos variables de estado ya que
		// Enforce Script no permite closures ni pasar argumentos a CallLater
		m_vPendingImpactPos = impactPos;
		GetGame().GetCallqueue().CallLater(BH_SpawnExplosion, (int)(m_fShellIncomingDelay * 1000), false);

		m_pActiveMission.m_iRoundsFired++;
	}

	// Posición pendiente del próximo impacto (bridge para CallLater sin args)
	protected vector m_vPendingImpactPos;

	//------------------------------------------------------------------------------------------------
	// Spawnea efecto visual y sonido de impacto de artillería
	protected void BH_SpawnExplosion()
	{
		if (!m_pActiveMission || !m_bFiringActive)
			return;

		// --- Partículas visuales --- igual que BH_IED_Controller
		BH_IED_Utils.BH_SpawnParticleEffect(
			"{8105B9A5EA395C54}Particles/Weapon/Explosion_M15_AT_Mine.ptc",
			m_vPendingImpactPos);

		// --- Sonido ---
		// Spawnear una entidad temporal con el SoundComponent del .acp de la mina
		// y reproducir el evento de explosión, igual que hace la IED
		BH_SpawnExplosionSound(m_vPendingImpactPos);

		BH_Log("BH_SpawnExplosion: impacto en " + m_vPendingImpactPos);

		// Programar el siguiente impacto si quedan rondas
		if (m_pActiveMission.m_iRoundsFired < m_pActiveMission.m_iRoundsTotal)
		{
			GetGame().GetCallqueue().CallLater(BH_FireNextRound,
				(int)(m_pActiveMission.m_fImpactInterval * 1000), false);
		}
		else
		{
			GetGame().GetCallqueue().CallLater(BH_MissionComplete, 2000, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Spawnea una entidad temporal, le asigna el .acp de la mina AT,
	// reproduce SOUND_EXPLOSION y la destruye tras 3 segundos
	protected void BH_SpawnExplosionSound(vector pos)
	{
		// Ajustar Y al terreno
		BaseWorld world = GetGame().GetWorld();
		if (world)
			pos[1] = world.GetSurfaceY(pos[0], pos[2]);

		// Spawnear la IED del mod como entidad temporal — tiene el .acp
		// {87E0968F67279DFE} asignado con el evento SOUND_EXPLOSION configurado.
		// La usamos solo para reproducir el sonido y la destruimos inmediatamente.
		Resource res = Resource.Load("{1F641A3DF8723A1A}Prefabs/IED/BH_IED_Base.et");
		if (!res || !res.IsValid())
		{
			BH_Log("BH_SpawnExplosionSound: no se pudo cargar prefab de sonido.");
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		mat[3] = pos;
		params.Transform = mat;

		IEntity soundEnt = GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params);
		if (!soundEnt)
			return;

		// Reproducir SOUND_EXPLOSION desde el SoundComponent del prefab
		SoundComponent soundComp = SoundComponent.Cast(soundEnt.FindComponent(SoundComponent));
		if (soundComp)
			soundComp.SoundEvent("SOUND_EXPLOSION");

		// Destruir la entidad tras 3 segundos (tiempo suficiente para el sonido)
		GetGame().GetCallqueue().CallLater(SCR_EntityHelper.DeleteEntityAndChildren, 3000, false, soundEnt);
	}

	//------------------------------------------------------------------------------------------------
	// Finaliza la misión y libera el bloqueo global
	protected void BH_MissionComplete()
	{
		BH_Log("BH_MissionComplete: misión de artillería completada.");

		if (m_pActiveMission && m_pActiveMission.m_pOwner)
			m_pActiveMission.m_pOwner.BH_ReleaseGlobalLock();

		m_pActiveMission  = null;
		m_bFiringActive   = false;
	}

	//------------------------------------------------------------------------------------------------
	// Cancela la misión activa (limpieza de emergencia)
	protected void BH_StopFiring()
	{
		if (!m_bFiringActive)
			return;

		GetGame().GetCallqueue().Remove(BH_FireNextRound);
		GetGame().GetCallqueue().Remove(BH_SpawnExplosion);
		GetGame().GetCallqueue().Remove(BH_MissionComplete);

		if (m_pActiveMission && m_pActiveMission.m_pOwner)
			m_pActiveMission.m_pOwner.BH_ReleaseGlobalLock();

		m_pActiveMission = null;
		m_bFiringActive  = false;

		BH_Log("BH_StopFiring: misión cancelada.");
	}

	// -----------------------------------------------------------------------
	// RPC — Servidor → Clientes (efectos de sonido)
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Notifica a los clientes que viene artillería (antes del primer impacto)
	// para que puedan reproducir avisos de voz / radio.
	// delay: segundos hasta el primer impacto
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ClientNotifyIncoming(int delay)
	{
		if (System.IsConsoleApp())
			return;

		BH_Log("RPC_ClientNotifyIncoming: artillería en camino, ETA " + delay + "s");

		// Mostrar mensaje en pantalla a todos los clientes
		// Se reutiliza el canal de notificación del HUD si está disponible;
		// si no, se imprime en el chat del juego como fallback.
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;

		BH_FireSupportHUD hudComp = BH_FireSupportHUD.Cast(pc.FindComponent(BH_FireSupportHUD));
		if (hudComp)
		{
			string etaStr = delay.ToString();
			hudComp.BH_ShowExternalNotification("¡ARTILLERÍA ENTRANTE! ETA: " + etaStr + "s");
		}
	}

	// -----------------------------------------------------------------------
	// UTILIDADES
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_Log(string msg)
	{
		if (!m_bBH_Debug)
			return;

		Print("[BH_ArtillerySupportManager] " + msg, LogLevel.NORMAL);
	}
}