// ============================================================
//  BH_GroupIntegration.c
//  Integra NPC rescatado en el slave group del jugador.
//  Si no existe slave group, lo crea directamente.
// ============================================================

class BH_GroupIntegration
{
	// ----------------------------------------------------------
	//  VARIABLES INTERNAS
	// ----------------------------------------------------------

	protected IEntity     m_NPC;
	protected IEntity     m_Player;
	protected AIAgent     m_NPCAgent;
	protected AIGroup     m_OriginalGroup;
	protected SCR_AIGroup m_PlayerGroup;
	protected bool        m_bJoined = false;
	protected bool        m_bAvailable = false;

	// Control de reintentos
	protected static const int BH_MAX_RETRIES = 3;
	protected int m_iRetryCount = 0;

	// ----------------------------------------------------------
	//  API PÚBLICA
	// ----------------------------------------------------------

	bool BH_JoinPlayerGroup(IEntity npc, IEntity player)
	{
		if (m_bJoined)
			BH_LeavePlayerGroup();

		if (!npc || !player)
		{
			Print("[BH_GroupInt] ERROR: NPC o jugador nulos.", LogLevel.ERROR);
			return false;
		}

		m_NPC    = npc;
		m_Player = player;

		SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsMgr)
		{
			Print("[BH_GroupInt] GroupsManager no disponible.", LogLevel.NORMAL);
			m_bAvailable = false;
			return false;
		}

		m_bAvailable = true;

		AIControlComponent aiControl = AIControlComponent.Cast(npc.FindComponent(AIControlComponent));
		if (!aiControl)
		{
			Print("[BH_GroupInt] NPC no tiene AIControlComponent.", LogLevel.WARNING);
			return false;
		}

		m_NPCAgent = aiControl.GetControlAIAgent();
		if (!m_NPCAgent)
		{
			Print("[BH_GroupInt] NPC no tiene AIAgent.", LogLevel.WARNING);
			return false;
		}

		m_OriginalGroup = m_NPCAgent.GetParentGroup();

		m_PlayerGroup = BH_FindPlayerGroup(player);
		if (!m_PlayerGroup)
		{
			Print("[BH_GroupInt] No se encontró grupo del jugador.", LogLevel.NORMAL);
			return false;
		}

		// Obtener slave group
		SCR_AIGroup slaveGroup = m_PlayerGroup.GetSlave();

		if (!slaveGroup)
		{
			// No existe slave group, crearlo directamente
			Print("[BH_GroupInt] No hay slave group, creando uno...", LogLevel.NORMAL);
			slaveGroup = BH_CreateSlaveGroupDirect(m_PlayerGroup);

			if (!slaveGroup)
			{
				// Si la creación directa falla, intentar via RequestCreateSlaveGroup
				if (m_iRetryCount < BH_MAX_RETRIES)
				{
					m_iRetryCount++;
					BH_RequestCreateSlaveGroupViaRPC(player, m_PlayerGroup);
					GetGame().GetCallqueue().CallLater(BH_RetryJoin, 1500, false);
					Print(string.Format("[BH_GroupInt] Reintento %1/%2 programado.", m_iRetryCount, BH_MAX_RETRIES), LogLevel.NORMAL);
				}
				else
				{
					Print("[BH_GroupInt] Máximo de reintentos alcanzado. Slave group no disponible.", LogLevel.WARNING);
					m_iRetryCount = 0;
				}
				return false;
			}
		}

		// Sacar al NPC de su grupo actual
		if (m_OriginalGroup)
		{
			m_OriginalGroup.RemoveAgent(m_NPCAgent);
		}

		// Activar IA del slave si no está activa
		if (!slaveGroup.IsAIActivated())
			slaveGroup.ActivateAI();

		// Añadir al NPC
		slaveGroup.AddAgentFromControlledEntity(npc);

		// Notificar al GroupsManager para replicación
		RplComponent groupRplComp = RplComponent.Cast(slaveGroup.FindComponent(RplComponent));
		RplComponent npcRplComp = RplComponent.Cast(npc.FindComponent(RplComponent));
		if (groupRplComp && npcRplComp)
			groupsMgr.AskAddAiMemberToGroup(groupRplComp.Id(), npcRplComp.Id());

		m_bJoined = true;
		m_iRetryCount = 0;
		Print("[BH_GroupInt] NPC unido al slave group del jugador.", LogLevel.NORMAL);
		return true;
	}

	void BH_LeavePlayerGroup()
	{
		if (!m_bJoined)
			return;

		// Cancelar reintentos pendientes
		GetGame().GetCallqueue().Remove(BH_RetryJoin);

		Print("[BH_GroupInt] Desvinculando NPC del slave group.", LogLevel.NORMAL);

		SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();

		if (m_PlayerGroup && m_NPC)
		{
			SCR_AIGroup slaveGroup = m_PlayerGroup.GetSlave();
			if (slaveGroup)
			{
				if (slaveGroup.GetAgentsCount() == 1)
					slaveGroup.Deactivate();

				slaveGroup.RemoveAgentFromControlledEntity(m_NPC);

				if (groupsMgr)
				{
					RplComponent groupRplComp = RplComponent.Cast(slaveGroup.FindComponent(RplComponent));
					RplComponent npcRplComp = RplComponent.Cast(m_NPC.FindComponent(RplComponent));
					if (groupRplComp && npcRplComp)
						groupsMgr.AskRemoveAiMemberFromGroup(groupRplComp.Id(), npcRplComp.Id());
				}
			}
		}

		if (m_OriginalGroup && m_NPCAgent)
		{
			m_OriginalGroup.AddAgent(m_NPCAgent);
		}
		else if (m_NPCAgent && m_NPC)
		{
			AIGroup newGroup = AIGroup.Cast(GetGame().SpawnEntityPrefab(
				Resource.Load("{059D0D6C49812B43}Prefabs/AI/Groups/Group_Base.et"),
				m_NPC.GetWorld(), null
			));
			if (newGroup)
				newGroup.AddAgent(m_NPCAgent);
		}

		m_bJoined     = false;
		m_PlayerGroup = null;
		m_NPC         = null;
		m_Player      = null;
		m_NPCAgent    = null;
		m_iRetryCount = 0;
	}

	bool BH_IsInPlayerGroup()        { return m_bJoined; }
	bool BH_IsGroupSystemAvailable() { return m_bAvailable; }

	// ----------------------------------------------------------
	//  CREACIÓN DIRECTA DEL SLAVE GROUP
	// ----------------------------------------------------------

	protected SCR_AIGroup BH_CreateSlaveGroupDirect(SCR_AIGroup masterGroup)
	{
		if (!masterGroup)
			return null;

		// Intentar obtener el prefab del CommandingManager (si existe)
		ResourceName groupPrefab;
		SCR_CommandingManagerComponent commandingMgr = SCR_CommandingManagerComponent.GetInstance();
		if (commandingMgr)
		{
			groupPrefab = commandingMgr.GetGroupPrefab();
		}

		// Fallback: usar Group_Base si no hay CommandingManager
		if (groupPrefab.IsEmpty())
			groupPrefab = "{059D0D6C49812B43}Prefabs/AI/Groups/Group_Base.et";

		Resource res = Resource.Load(groupPrefab);
		if (!res || !res.IsValid())
		{
			Print("[BH_GroupInt] ERROR: No se pudo cargar prefab de grupo.", LogLevel.ERROR);
			return null;
		}

		IEntity groupEntity = GetGame().SpawnEntityPrefab(res);
		if (!groupEntity)
		{
			Print("[BH_GroupInt] ERROR: No se pudo spawnear grupo.", LogLevel.ERROR);
			return null;
		}

		SCR_AIGroup slaveGroup = SCR_AIGroup.Cast(groupEntity);
		if (!slaveGroup)
		{
			Print("[BH_GroupInt] ERROR: Entidad spawneada no es SCR_AIGroup.", LogLevel.ERROR);
			return null;
		}

		// Configurar como slave: no se borra cuando está vacío
		slaveGroup.SetDeleteWhenEmpty(false);

		// Vincular al master group via GroupsManager
		SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
		if (groupsMgr)
		{
			RplComponent masterRpl = RplComponent.Cast(masterGroup.FindComponent(RplComponent));
			RplComponent slaveRpl = RplComponent.Cast(slaveGroup.FindComponent(RplComponent));
			if (masterRpl && slaveRpl)
			{
				groupsMgr.RequestSetGroupSlave(masterRpl.Id(), slaveRpl.Id());
				Print("[BH_GroupInt] Slave group creado y vinculado directamente.", LogLevel.NORMAL);
				return slaveGroup;
			}
		}

		Print("[BH_GroupInt] ERROR: No se pudo vincular slave al master.", LogLevel.ERROR);
		// Limpiar el grupo huérfano
		SCR_EntityHelper.DeleteEntityAndChildren(slaveGroup);
		return null;
	}

	// ----------------------------------------------------------
	//  FALLBACK: CREACIÓN VIA RPC
	// ----------------------------------------------------------

	protected void BH_RequestCreateSlaveGroupViaRPC(IEntity player, SCR_AIGroup masterGroup)
	{
		if (!player || !masterGroup)
			return;

		int playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(player);
		if (playerID <= 0)
			return;

		PlayerController playerCtrl = GetGame().GetPlayerManager().GetPlayerController(playerID);
		if (!playerCtrl)
			return;

		SCR_PlayerControllerGroupComponent groupComp = SCR_PlayerControllerGroupComponent.Cast(
			playerCtrl.FindComponent(SCR_PlayerControllerGroupComponent)
		);
		if (!groupComp)
			return;

		RplComponent masterRplComp = RplComponent.Cast(masterGroup.FindComponent(RplComponent));
		if (!masterRplComp)
			return;

		groupComp.RequestCreateSlaveGroup(masterRplComp.Id());
	}

	protected void BH_RetryJoin()
	{
		if (!m_NPC || !m_Player || m_bJoined)
			return;

		BH_JoinPlayerGroup(m_NPC, m_Player);
	}

	// ----------------------------------------------------------
	//  HELPERS
	// ----------------------------------------------------------

	protected SCR_AIGroup BH_FindPlayerGroup(IEntity player)
	{
		if (!player)
			return null;

		int playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(player);
		if (playerID > 0)
		{
			SCR_GroupsManagerComponent groupsMgr = SCR_GroupsManagerComponent.GetInstance();
			if (groupsMgr)
			{
				SCR_AIGroup group = groupsMgr.GetPlayerGroup(playerID);
				if (group)
					return group;
			}
		}

		return null;
	}
}