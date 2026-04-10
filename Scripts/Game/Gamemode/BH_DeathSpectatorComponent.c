// ============================================================================
// BH_DeathSpectatorComponent.c
// Mod: Bhelma - Sistema de Briefing Pre-Partida
// 
// DESCRIPCION:
//   Camara de muerte: cuando un jugador muere se le activa una
//   camara libre (ManualCameraSpectate) tipo Zeus.
//   No puede respawnear (muerte permanente).
//
//   Incluye modded class de SCR_PlayerDeployMenuHandlerComponent
//   para evitar que el deploy menu se reabra automaticamente
//   (CanOpenMenu devuelve true cuando hasDeadEntity, lo cual
//   fuerza la apertura del menu cada frame, cerrando la camara).
//
//   DEBUG: Activar m_bDebugEnabled en el Workbench
// ============================================================================

[ComponentEditorProps(category: "Bhelma/GameMode", description: "Camara libre al morir + muerte permanente.")]
class BH_DeathSpectatorComponentClass : SCR_BaseGameModeComponentClass
{
}

class BH_DeathSpectatorComponent : SCR_BaseGameModeComponent
{
	[Attribute("0", UIWidgets.CheckBox, "Activar mensajes de debug en consola", category: "Bhelma Spectator")]
	protected bool m_bDebugEnabled;
	
	[Attribute("1", UIWidgets.CheckBox, "Activar muerte permanente con camara libre", category: "Bhelma Spectator")]
	protected bool m_bPermaDeathEnabled;
	
	[Attribute("Has caido en combate. Camara libre activada.", UIWidgets.EditBox, "Mensaje al morir", category: "Bhelma Spectator")]
	protected string m_sDeathMessage;
	
	[Attribute("{E1FF38EC8894C5F3}Prefabs/Editor/Camera/ManualCameraSpectate.et", UIWidgets.ResourceNamePicker, "Prefab de la camara de espectador", category: "Bhelma Spectator", params: "et")]
	protected ResourceName m_sCameraPrefab;
	
	protected ref array<int> m_aDeadPlayers = {};
	protected SCR_ManualCamera m_SpectatorCamera;
	protected bool m_bLocalPlayerDead = false;
	
	protected static BH_DeathSpectatorComponent s_Instance;
	static BH_DeathSpectatorComponent GetInstance() { return s_Instance; }
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		BH_Debug("Inicializado. PermaDeath=" + m_bPermaDeathEnabled);
	}
	
	// -------------------------------------------------------
	// OnPlayerKilled - solo SERVER
	// -------------------------------------------------------
	override void OnPlayerKilled(notnull SCR_InstigatorContextData instigatorContextData)
	{
		if (!m_bPermaDeathEnabled)
			return;
		
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (!briefingMgr || !briefingMgr.IsMissionStarted())
			return;
		
		if (!Replication.IsServer())
			return;
		
		int playerId = instigatorContextData.GetVictimPlayerID();
		if (playerId <= 0)
			return;
		
		BH_Debug("[SERVER] Jugador " + playerId + " muerto permanentemente");
		
		if (!m_aDeadPlayers.Contains(playerId))
			m_aDeadPlayers.Insert(playerId);
		
		vector deathPos = vector.Zero;
		IEntity victimEntity = instigatorContextData.GetVictimEntity();
		if (victimEntity)
			deathPos = victimEntity.GetOrigin();
		
		// Notificar al cliente
		Rpc(RpcDo_ActivateSpectator, playerId, deathPos);
		
		// Listen Server: ejecutar local tambien
		PlayerController localPC = GetGame().GetPlayerController();
		if (localPC && localPC.GetPlayerId() == playerId)
			DoActivateSpectator(playerId, deathPos);
	}
	
	// -------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ActivateSpectator(int playerId, vector deathPos)
	{
		PlayerController localPC = GetGame().GetPlayerController();
		if (!localPC || localPC.GetPlayerId() != playerId)
			return;
		
		// Evitar doble ejecucion en Listen Server
		if (m_bLocalPlayerDead)
			return;
		
		DoActivateSpectator(playerId, deathPos);
	}
	
	// -------------------------------------------------------
	protected void DoActivateSpectator(int playerId, vector deathPos)
	{
		if (m_bLocalPlayerDead)
			return;
		m_bLocalPlayerDead = true;
		
		BH_Debug("Activando camara de espectador para jugador " + playerId);
		
		SCR_ChatPanelManager chatManager = SCR_ChatPanelManager.GetInstance();
		if (chatManager)
			chatManager.ShowHelpMessage("[Bhelma] " + m_sDeathMessage);
		
		// Crear camara con retraso para que BI termine de procesar la muerte
		GetGame().GetCallqueue().CallLater(CreateSpectatorCamera, 1500, false, deathPos);
	}
	
	// -------------------------------------------------------
	protected void CreateSpectatorCamera(vector position)
	{
		if (m_SpectatorCamera)
			return;
		
		// Cerrar menus de deploy si estan abiertos
		SCR_DeployMenuMain.CloseDeployMenu();
		SCR_RoleSelectionMenu.CloseRoleSelectionMenu();
		
		Resource cameraResource = Resource.Load(m_sCameraPrefab);
		if (!cameraResource || !cameraResource.IsValid())
		{
			BH_Debug("ERROR: Prefab no encontrado: " + m_sCameraPrefab);
			return;
		}
		
		position[1] = position[1] + 10;
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = position;
		
		IEntity cameraEntity = GetGame().SpawnEntityPrefab(cameraResource, GetGame().GetWorld(), spawnParams);
		if (!cameraEntity)
		{
			BH_Debug("ERROR: No se pudo spawnear la camara");
			return;
		}
		
		m_SpectatorCamera = SCR_ManualCamera.Cast(cameraEntity);
		if (m_SpectatorCamera)
		{
			CameraManager cameraManager = GetGame().GetCameraManager();
			if (cameraManager)
			{
				cameraManager.SetCamera(m_SpectatorCamera);
				BH_Debug("Camara de espectador activada en: " + position.ToString());
			}
		}
	}
	
	// -------------------------------------------------------
	bool IsPlayerPermaDead(int playerId)
	{
		return m_aDeadPlayers.Contains(playerId);
	}
	
	// -------------------------------------------------------
	// Consultar si el jugador LOCAL esta muerto (client-side)
	// Usado por el modded SCR_PlayerDeployMenuHandlerComponent
	// -------------------------------------------------------
	bool IsLocalPlayerDead()
	{
		return m_bLocalPlayerDead;
	}
	
	protected void BH_Debug(string message)
	{
		if (m_bDebugEnabled)
			Print("[BH_Spectator] " + message, LogLevel.NORMAL);
	}
	
	void ~BH_DeathSpectatorComponent()
	{
		if (m_SpectatorCamera)
			delete m_SpectatorCamera;
		if (s_Instance == this)
			s_Instance = null;
	}
}

// ============================================================================
// MODDED: Evitar que el deploy menu se reabra para jugadores muertos
//
// El problema: SCR_PlayerDeployMenuHandlerComponent.CanOpenMenu()
// devuelve true cuando hasDeadEntity=true. Esto provoca que Update()
// abra el deploy menu cada frame, cerrando nuestra camara libre.
//
// Solucion: si el jugador tiene muerte permanente de Bhelma,
// CanOpenMenu() devuelve false.
// ============================================================================
modded class SCR_PlayerDeployMenuHandlerComponent
{
	override protected bool CanOpenMenu()
	{
		// Si el jugador tiene muerte permanente, NO abrir deploy menu
		BH_DeathSpectatorComponent deathComp = BH_DeathSpectatorComponent.GetInstance();
		if (deathComp && deathComp.IsLocalPlayerDead())
			return false;
		
		return super.CanOpenMenu();
	}
	
	override protected bool CanOpenWelcomeScreen()
	{
		BH_DeathSpectatorComponent deathComp = BH_DeathSpectatorComponent.GetInstance();
		if (deathComp && deathComp.IsLocalPlayerDead())
			return false;
		
		return super.CanOpenWelcomeScreen();
	}
}