// ============================================================================
// BH_BriefingManagerComponent.c
// Mod: Bhelma - Sistema de Briefing Pre-Partida
// 
// DESCRIPCION:
//   Componente principal del sistema de briefing de Bhelma.
//   
//   LANZAMIENTO DE MISION:
//     La tecla se detecta desde el modded SCR_DeployMenuMain.OnMenuUpdate
//     (en BH_DeployButtonLock.c) ya que ahi el input SIEMPRE funciona.
//     El metodo TryStartMission() se llama desde ahi.
//     Tambien se mantiene /startmission como backup para listen server.
//
//   DEBUG: Activar m_bDebugEnabled en el Workbench
// ============================================================================

enum BH_EBriefingState
{
	BRIEFING,
	MISSION
}

[ComponentEditorProps(category: "Bhelma/GameMode", description: "Componente principal del sistema de briefing de Bhelma.")]
class BH_BriefingManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

class BH_BriefingManagerComponent : SCR_BaseGameModeComponent
{
	[Attribute("0", UIWidgets.CheckBox, "Activar mensajes de debug en consola", category: "Bhelma Briefing")]
	protected bool m_bDebugEnabled;
	
	[Attribute("startmission", UIWidgets.EditBox, "Comando de chat para lanzar la mision (sin /)", category: "Bhelma Briefing")]
	protected string m_sChatCommand;
	
	[Attribute("La mision ha comenzado!", UIWidgets.EditBox, "Mensaje al lanzar la mision", category: "Bhelma Briefing")]
	protected string m_sStartMessage;
	
	[Attribute("0", UIWidgets.CheckBox, "Permitir inicio sin ser admin (SOLO PARA TESTING en Workbench). Desactivar en produccion!", category: "Bhelma Briefing")]
	protected bool m_bBypassAdminCheck;
	
	protected BH_EBriefingState m_eState = BH_EBriefingState.BRIEFING;
	protected static BH_BriefingManagerComponent s_Instance;
	protected ref ScriptInvoker m_OnStateChanged = new ScriptInvoker();
	
	static BH_BriefingManagerComponent GetInstance() { return s_Instance; }
	ScriptInvoker GetOnStateChanged() { return m_OnStateChanged; }
	BH_EBriefingState GetBriefingState() { return m_eState; }
	bool IsBriefing() { return m_eState == BH_EBriefingState.BRIEFING; }
	bool IsMissionStarted() { return m_eState == BH_EBriefingState.MISSION; }
	bool IsAdminBypass() { return m_bBypassAdminCheck; }
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		GetGame().GetCallqueue().CallLater(RegisterChatCommand, 1000, false);
		BH_Debug("Inicializado. Estado: BRIEFING");
	}
	
	// Chat command (backup para listen server)
	protected void RegisterChatCommand()
	{
		SCR_ChatPanelManager chatManager = SCR_ChatPanelManager.GetInstance();
		if (!chatManager)
		{
			GetGame().GetCallqueue().CallLater(RegisterChatCommand, 2000, false);
			return;
		}
		ChatCommandInvoker invoker = chatManager.GetCommandInvoker(m_sChatCommand);
		if (invoker)
		{
			invoker.Insert(OnChatCommand);
			BH_Debug("Chat /" + m_sChatCommand + " registrado");
		}
	}
	
	protected void OnChatCommand(SCR_ChatPanel panel, string data)
	{
		TryStartMission();
	}
	
	// -------------------------------------------------------
	// Metodo publico: intentar lanzar la mision
	// Se llama desde el modded SCR_DeployMenuMain (tecla)
	// o desde el chat command
	// -------------------------------------------------------
	// -------------------------------------------------------
	// CLIENTE: intentar lanzar la mision
	// Envia RPC via PlayerController (donde el cliente ES owner)
	// NO directamente por el GameMode (cliente NO es owner -> 
	// RPC ignorado silenciosamente en servidor dedicado)
	// -------------------------------------------------------
	void TryStartMission()
	{
		if (m_eState == BH_EBriefingState.MISSION)
		{
			BH_Debug("Mision ya en curso");
			return;
		}
		
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			BH_Debug("Sin PlayerController");
			return;
		}
		
		int playerId = pc.GetPlayerId();
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		
		bool isAdmin = pm.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		if (m_bBypassAdminCheck)
		{
			BH_Debug("BYPASS ADMIN ACTIVADO (solo testing!)");
			isAdmin = true;
		}
		
		BH_Debug("TryStartMission: jugador=" + pm.GetPlayerName(playerId) + " admin=" + isAdmin);
		
		if (!isAdmin)
		{
			ShowMessageToPlayer("No tienes permisos de administrador.");
			return;
		}
		
		// CLAVE: enviar RPC via PlayerController, NO via GameMode
		// El cliente es OWNER del PlayerController -> RPC funciona en dedicado
		SCR_PlayerController scrPC = SCR_PlayerController.Cast(pc);
		if (scrPC)
		{
			BH_Debug("Enviando RPC via PlayerController...");
			scrPC.BH_RequestStartMission();
		}
	}
	
	// -------------------------------------------------------
	// SERVIDOR: ejecutar inicio de mision
	// Llamado desde modded SCR_PlayerController.BH_RpcAsk_StartMission
	// -------------------------------------------------------
	void ServerStartMission(int playerId)
	{
		BH_Debug("[SERVER] ServerStartMission jugador=" + playerId);
		
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		
		bool isAdmin = pm.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR);
		if (m_bBypassAdminCheck)
			isAdmin = true;
		
		if (!isAdmin)
		{
			BH_Debug("[SERVER] RECHAZADO: no es admin");
			return;
		}
		if (m_eState == BH_EBriefingState.MISSION)
			return;
		
		m_eState = BH_EBriefingState.MISSION;
		BH_Debug("[SERVER] Estado -> MISSION");
		Rpc(RpcDo_MissionStarted);
		DoMissionStarted();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_MissionStarted()
	{
		DoMissionStarted();
	}
	
	protected void DoMissionStarted()
	{
		m_eState = BH_EBriefingState.MISSION;
		m_OnStateChanged.Invoke(m_eState);
		ShowMessageToPlayer(m_sStartMessage);
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(SpawnAllPlayers, 500, false);
	}
	
	protected void SpawnAllPlayers()
	{
		BH_Debug("[SERVER] Spawneando...");
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm) return;
		array<int> playerIds = {};
		pm.GetPlayers(playerIds);
		foreach (int pid : playerIds)
			SpawnPlayerByGroup(pid);
		BH_Debug("[SERVER] Spawn completado: " + playerIds.Count());
	}
	
	protected void SpawnPlayerByGroup(int playerId)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm) return;
		PlayerController pc = pm.GetPlayerController(playerId);
		if (!pc) return;
		SCR_RespawnComponent respawnComp = SCR_RespawnComponent.Cast(pm.GetPlayerRespawnComponent(playerId));
		if (!respawnComp) return;
		SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsMgr) return;
		SCR_AIGroup playerGroup = groupsMgr.GetPlayerGroup(playerId);
		if (!playerGroup)
		{
			BH_Debug("[SERVER] " + pm.GetPlayerName(playerId) + " sin grupo");
			return;
		}
		
		string groupName = playerGroup.GetCustomName();
		BH_Debug("[SERVER] GetCustomName() = '" + groupName + "'");
		if (groupName.IsEmpty())
		{
			string company, platoon, squad, character, format;
			playerGroup.GetCallsigns(company, platoon, squad, character, format);
			BH_Debug("[SERVER] Callsigns -> co='" + company + "' pl='" + platoon + "' sq='" + squad + "'");
			if (!squad.IsEmpty())
				groupName = squad;
			else if (!format.IsEmpty())
				groupName = string.Format(format, company, platoon, squad, character);
		}
		
		BH_Debug("[SERVER] " + pm.GetPlayerName(playerId) + " -> '" + groupName + "'");
		BH_GroupSpawnPoint targetSpawn = BH_GroupSpawnPoint.FindSpawnPointForGroup(groupName);
		if (!targetSpawn)
		{
			BH_Debug("[SERVER] Sin spawn, usando default");
			targetSpawn = BH_GroupSpawnPoint.GetDefaultSpawnPoint();
		}
		if (!targetSpawn) return;
		
		SCR_PlayerLoadoutComponent loadoutComp = SCR_PlayerLoadoutComponent.Cast(pc.FindComponent(SCR_PlayerLoadoutComponent));
		if (!loadoutComp) return;
		SCR_BasePlayerLoadout loadout = loadoutComp.GetLoadout();
		if (!loadout) return;
		
		SCR_SpawnPointSpawnData spawnData = new SCR_SpawnPointSpawnData(loadout.GetLoadoutResource(), targetSpawn.GetRplId());
		respawnComp.RequestSpawn(spawnData);
		BH_Debug("[SERVER] Spawn: " + pm.GetPlayerName(playerId) + " -> '" + targetSpawn.GetGroupName() + "'");
	}
	
	override void OnPlayerConnected(int playerId) {}
	
	override bool RplSave(ScriptBitWriter writer)
	{
		writer.WriteInt(m_eState);
		return true;
	}
	
	override bool RplLoad(ScriptBitReader reader)
	{
		int state;
		reader.ReadInt(state);
		m_eState = state;
		if (m_eState == BH_EBriefingState.MISSION)
			m_OnStateChanged.Invoke(m_eState);
		return true;
	}
	
	protected void ShowMessageToPlayer(string message)
	{
		SCR_ChatPanelManager chatManager = SCR_ChatPanelManager.GetInstance();
		if (chatManager)
			chatManager.ShowHelpMessage("[Bhelma] " + message);
	}
	
	void BH_Debug(string message)
	{
		if (m_bDebugEnabled)
			Print("[BH_Briefing] " + message, LogLevel.NORMAL);
	}
	
	void ~BH_BriefingManagerComponent()
	{
		if (s_Instance == this)
			s_Instance = null;
	}
}