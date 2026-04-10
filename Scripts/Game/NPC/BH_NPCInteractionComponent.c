// ============================================================
//  BH_NPCInteractionComponent.c
//  Componente para NPC interactivo en Scenario Framework.
//
//  Funcionalidades:
//    - Hablar con el NPC (ejecuta acciones del SF)
//    - Exposar/arrestar al NPC (ejecuta acciones del SF)
//    - NPC sigue al jugador a pie via SCR_EntityWaypoint
//    - NPC sube/baja de vehículos automáticamente
//    - Soltar al NPC (recupera IA de combate)
//    - [NUEVO] Watchdog de escolta: teleport de recuperación
//      y refresco periódico de waypoint
//    - [NUEVO] Integración con grupo del jugador: la IA
//      responde al command wheel si el modo lo permite
//
//  Pendiente:
//    - Animación de manos esposadas (API no expuesta por BI aún)
//
//  Uso: Añadir al prefab del NPC junto a ActionsManagerComponent.
//       Añadir BH_TalkAction, BH_ArrestAction y BH_ReleaseAction
//       en el array Action Classes del ActionsManagerComponent.
// ============================================================

[ComponentEditorProps(category: "BH/NPC", description: "Componente de interacción para NPCs. Permite hablar, exposar y escoltar al NPC.")]
class BH_NPCInteractionComponentClass : ScriptComponentClass {}

enum BH_EEscortState
{
	INACTIVE,
	FOLLOWING,
	BOARDING,
	IN_VEHICLE,
	ALIGHTING
}

enum BH_EReleaseMode
{
	SURRENDER,    // Se rinde: queda aliado sin combate
	RESTORE_ENEMY, // Vuelve a ser enemigo: restaura facción y combate
	DISAPPEAR,    // Desaparece: elimina la entidad
	NEUTRAL       // Neutral: sin facción, sin combate, nadie le dispara
}

class BH_NPCInteractionComponent : ScriptComponent
{
	// ----------------------------------------------------------
	//  ATRIBUTOS EDITABLES EN EL EDITOR
	// ----------------------------------------------------------

	[Attribute("1", UIWidgets.CheckBox, "¿Se puede hablar con este NPC?")]
	protected bool m_bCanTalk;

	[Attribute("1", UIWidgets.CheckBox, "¿Se puede exposar/arrestar a este NPC?")]
	protected bool m_bCanArrest;

	[Attribute("1", UIWidgets.CheckBox, "¿El NPC sigue al jugador al ser arrestado?")]
	protected bool m_bEscortOnArrest;

	[Attribute("", UIWidgets.Object, "Acciones del Scenario Framework al hablar con el NPC.")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aOnTalkActions;

	[Attribute("", UIWidgets.Object, "Acciones del Scenario Framework al arrestar al NPC.")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aOnArrestActions;

	// --- MODO PRISIONERO ---

	[Attribute("0", UIWidgets.CheckBox, "¿El NPC aparece como prisionero desde el inicio?")]
	protected bool m_bStartAsPrisoner;

	[Attribute("", UIWidgets.EditBox, "Facción con la que NACE el NPC prisionero (ej: USSR, US, FIA, CIV). Dejar vacío para usar la facción del prefab. Debe coincidir con la Faction Key del FactionManager.")]
	protected string m_sPrisonerStartFactionKey;

	[Attribute("", UIWidgets.EditBox, "Facción a la que CAMBIA el NPC al ser rescatado (ej: US, USSR, FIA, CIV). Dejar vacío para no cambiar facción. Debe coincidir con la Faction Key del FactionManager.")]
	protected string m_sRescueFactionKey;

	[Attribute("", UIWidgets.ResourceNamePicker, "Arma que recibirá el NPC al ser rescatado.", "et")]
	protected ResourceName m_sRescueWeapon;

	[Attribute("", UIWidgets.Object, "Acciones del Scenario Framework al rescatar al NPC.")]
	protected ref array<ref SCR_ScenarioFrameworkActionBase> m_aOnRescueActions;

	// --- CONFIGURACIÓN DE ESCOLTA AVANZADA ---

	[Attribute("1", UIWidgets.CheckBox, "¿Intentar unir al NPC al grupo del jugador al rescatar? Permite dar órdenes via command wheel.")]
	protected bool m_bJoinPlayerGroupOnRescue;

	[Attribute("1", UIWidgets.CheckBox, "¿Intentar unir al NPC arrestado al grupo del jugador? Cambia la facción del NPC temporalmente para permitirlo.")]
	protected bool m_bJoinPlayerGroupOnArrest;

	[Attribute("0", UIWidgets.ComboBox, "¿Qué ocurre al soltar un NPC arrestado que era enemigo?\n- Rendirse: queda aliado sin combate\n- Volver enemigo: restaura facción original y combate\n- Desaparecer: elimina al NPC\n- Neutral: sin facción, sin combate", enums: ParamEnumArray.FromEnum(BH_EReleaseMode))]
	protected BH_EReleaseMode m_eReleaseBehavior;

	[Attribute("0", UIWidgets.CheckBox, "¿Activar watchdog de escolta? AVISO: Puede interferir con las órdenes del command wheel. Solo activar si el NPC NO se une al grupo del jugador o como red de seguridad.")]
	protected bool m_bEnableWatchdog;

	[Attribute("150", UIWidgets.Slider, "Distancia máxima (m) antes de teleport de emergencia del watchdog. Solo aplica si el watchdog está activado.", "50 500 10")]
	protected float m_fWatchdogMaxDistance;

	// ----------------------------------------------------------
	//  VARIABLES INTERNAS
	// ----------------------------------------------------------

	protected bool            m_bIsArrested  = false;
	protected bool            m_bHasSpoken   = false;
	protected bool            m_bIsPrisoner  = false;
	protected IEntity         m_PlayerEscorting;
	protected BH_EEscortState m_eEscortState = BH_EEscortState.INACTIVE;

	// IA
	protected AIAgent                      m_AIAgent;
	protected AIControlComponent           m_AIControl;
	protected AIGroup                      m_EscortGroup;
	protected SCR_AIConfigComponent        m_AIConfig;
	protected CharacterControllerComponent m_CharController;

	// Escolta
	protected AIWaypoint                     m_ActiveWaypoint;
	protected SCR_CompartmentAccessComponent m_PlayerCompartmentAccess;
	protected SCR_CompartmentAccessComponent m_NPCCompartmentAccess;

	// NUEVO: Watchdog y grupo
	protected ref BH_EscortWatchdog   m_Watchdog;
	protected ref BH_GroupIntegration m_GroupIntegration;

	// Facción original del NPC (para restaurar al soltar un NPC arrestado de otra facción)
	protected Faction m_OriginalFaction;

	// ScriptInvokers para suscripción externa desde otros componentes
	ref ScriptInvoker m_OnTalk    = new ScriptInvoker();
	ref ScriptInvoker m_OnArrest  = new ScriptInvoker();
	ref ScriptInvoker m_OnRelease = new ScriptInvoker();
	ref ScriptInvoker m_OnRescue  = new ScriptInvoker(); // NUEVO

	// ----------------------------------------------------------
	//  INICIALIZACIÓN
	// ----------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_AIControl            = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (m_AIControl)
			m_AIAgent = m_AIControl.GetControlAIAgent();
		m_CharController       = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		m_AIConfig             = SCR_AIConfigComponent.Cast(owner.FindComponent(SCR_AIConfigComponent));
		m_NPCCompartmentAccess = SCR_CompartmentAccessComponent.Cast(owner.FindComponent(SCR_CompartmentAccessComponent));

		BH_NPCInteractionManager manager = BH_NPCInteractionManager.GetInstance();
		if (manager)
			manager.BH_RegisterNPC(this);

		// Si el NPC comienza como prisionero, inmovilizarlo tras la inicialización completa
		if (m_bStartAsPrisoner)
			GetGame().GetCallqueue().CallLater(BH_ActivatePrisonerMode, 500, false);
	}

	override void OnDelete(IEntity owner)
	{
		BH_StopEscort();

		BH_NPCInteractionManager manager = BH_NPCInteractionManager.GetInstance();
		if (manager)
			manager.BH_UnregisterNPC(this);

		super.OnDelete(owner);
	}

	// ----------------------------------------------------------
	//  GETTERS PÚBLICOS
	// ----------------------------------------------------------

	bool    BH_CanTalk()      { return m_bCanTalk; }
	bool    BH_CanArrest()    { return m_bCanArrest; }
	bool    BH_IsArrested()   { return m_bIsArrested; }
	bool    BH_HasSpoken()    { return m_bHasSpoken; }
	bool    BH_IsPrisoner()   { return m_bIsPrisoner; }
	IEntity BH_GetOwner()     { return GetOwner(); }

	// NUEVO: Estado del watchdog
	bool BH_IsWatchdogActive()
	{
		return m_Watchdog && m_Watchdog.BH_IsActive();
	}

	// NUEVO: ¿Está en grupo del jugador?
	bool BH_IsInPlayerGroup()
	{
		return m_GroupIntegration && m_GroupIntegration.BH_IsInPlayerGroup();
	}

	// ----------------------------------------------------------
	//  MODO PRISIONERO
	// ----------------------------------------------------------

	protected void BH_ActivatePrisonerMode()
	{
		m_bIsPrisoner = true;
		BH_DisableAICombat();

		// Cambiar facción de inicio si está configurado
		if (m_sPrisonerStartFactionKey != string.Empty)
			BH_SwitchFactionByKey(m_sPrisonerStartFactionKey);

		// Obtener el grupo y limpiar todos sus waypoints
		// para que el NPC no ejecute patrullas ni defensas
		BH_EnsureAIGroup();
		if (m_EscortGroup)
		{
			array<AIWaypoint> existingWaypoints = {};
			m_EscortGroup.GetWaypoints(existingWaypoints);
			foreach (AIWaypoint wp : existingWaypoints)
				m_EscortGroup.RemoveWaypoint(wp);
		}
	}

	void BH_OnRescue(IEntity player)
	{
		if (!m_bIsPrisoner)
			return;

		Print("[BH_Rescue] Rescatando NPC...", LogLevel.NORMAL);
		m_bIsPrisoner = false;

		m_PlayerEscorting = player;

		// ============================================
		//  Cambiar facción si está configurado en el editor
		// ============================================
		if (m_sRescueFactionKey != string.Empty)
			BH_SwitchFactionByKey(m_sRescueFactionKey);

		BH_EnableAICombat();
		BH_EnsureAIGroup();

		// ============================================
		//  Intentar unir al grupo del jugador (PRIMERO)
		// ============================================
		bool joinedGroup = false;
		if (m_bJoinPlayerGroupOnRescue)
		{
			m_GroupIntegration = new BH_GroupIntegration();
			joinedGroup = m_GroupIntegration.BH_JoinPlayerGroup(GetOwner(), player);

			if (joinedGroup)
				Print("[BH_Rescue] NPC unido al grupo del jugador. Command wheel disponible.", LogLevel.NORMAL);
			else
				Print("[BH_Rescue] No se pudo unir al grupo.", LogLevel.NORMAL);
		}

		// ============================================
		//  Si NO se unió al grupo: usar waypoint Follow
		//  + eventos de vehículo + watchdog activo
		// ============================================
		if (!joinedGroup)
		{
			BH_SetStateFollowing();

			// Suscribirse a eventos de vehículo del jugador
			// (solo sin grupo, porque el commanding gestiona vehículos)
			m_PlayerCompartmentAccess = SCR_CompartmentAccessComponent.Cast(player.FindComponent(SCR_CompartmentAccessComponent));
			if (m_PlayerCompartmentAccess)
			{
				m_PlayerCompartmentAccess.GetOnPlayerCompartmentEnter().Insert(BH_OnPlayerEnterVehicle);
				m_PlayerCompartmentAccess.GetOnPlayerCompartmentExit().Insert(BH_OnPlayerExitVehicle);
			}
		}

		// Dar arma tras un breve delay
		if (m_sRescueWeapon != string.Empty)
			GetGame().GetCallqueue().CallLater(BH_GiveWeaponDelayed, 1000, false);

		// ============================================
		//  Watchdog (si está activado en el editor)
		// ============================================
		if (m_bEnableWatchdog)
		{
			m_Watchdog = new BH_EscortWatchdog();
			m_Watchdog.BH_StartWatching(GetOwner(), player, this, m_fWatchdogMaxDistance);

			// Si está en grupo, watchdog en modo pasivo
			if (joinedGroup)
				m_Watchdog.BH_SetPassiveMode(true);

			Print("[BH_Rescue] Watchdog activado.", LogLevel.NORMAL);
		}

		BH_ExecuteActions(m_aOnRescueActions);
		m_OnRescue.Invoke(GetOwner(), player);
	}

	protected void BH_GiveWeaponDelayed()
	{
		if (m_sRescueWeapon != string.Empty)
			BH_GiveWeapon(m_sRescueWeapon);
	}

	protected void BH_GiveWeapon(ResourceName weaponPrefab)
	{
		InventoryStorageManagerComponent invManager = InventoryStorageManagerComponent.Cast(
			GetOwner().FindComponent(InventoryStorageManagerComponent)
		);

		if (!invManager)
		{
			Print("[BH_NPC] ERROR: NPC no tiene InventoryStorageManagerComponent.", LogLevel.ERROR);
			return;
		}

		bool success = invManager.TrySpawnPrefabToStorage(weaponPrefab);
		if (!success)
			Print("[BH_NPC] AVISO: No se pudo entregar el arma al NPC.", LogLevel.WARNING);
	}

	// ----------------------------------------------------------
	//  ACCIÓN: HABLAR
	// ----------------------------------------------------------

	void BH_OnTalk(IEntity player)
	{
		if (!m_bCanTalk || m_bIsArrested)
			return;

		m_bHasSpoken = true;
		BH_LookAtEntity(player);
		BH_ExecuteActions(m_aOnTalkActions);
		m_OnTalk.Invoke(GetOwner(), player);
	}

	// ----------------------------------------------------------
	//  ACCIÓN: ARRESTAR / EXPOSAR
	// ----------------------------------------------------------

	void BH_OnArrest(IEntity player)
	{
		if (!m_bCanArrest || m_bIsArrested)
			return;

		m_bIsArrested     = true;
		m_PlayerEscorting = player;

		BH_DisableAICombat();

		// ============================================
		//  Intentar unir al grupo del jugador
		// ============================================
		bool joinedGroup = false;
		if (m_bJoinPlayerGroupOnArrest)
		{
			// Cambiar facción del NPC a la del jugador (necesario para el slave group)
			BH_SwitchFactionToPlayer(player);

			m_GroupIntegration = new BH_GroupIntegration();
			joinedGroup = m_GroupIntegration.BH_JoinPlayerGroup(GetOwner(), player);

			if (joinedGroup)
				Print("[BH_Arrest] NPC arrestado unido al grupo del jugador.", LogLevel.NORMAL);
			else
				Print("[BH_Arrest] No se pudo unir al grupo. Usando escolta por waypoint.", LogLevel.NORMAL);
		}

		// Si NO se unió al grupo: usar sistema de escolta antiguo
		if (!joinedGroup && m_bEscortOnArrest)
			BH_StartEscort(player);

		// Watchdog (si está activado)
		if (m_bEnableWatchdog)
		{
			m_Watchdog = new BH_EscortWatchdog();
			m_Watchdog.BH_StartWatching(GetOwner(), player, this, m_fWatchdogMaxDistance);

			if (joinedGroup)
				m_Watchdog.BH_SetPassiveMode(true);
		}

		BH_ExecuteActions(m_aOnArrestActions);
		m_OnArrest.Invoke(GetOwner(), player);
	}

	// ----------------------------------------------------------
	//  ACCIÓN: SOLTAR
	// ----------------------------------------------------------

	void BH_OnRelease(IEntity player)
	{
		if (!m_bIsArrested)
			return;

		m_bIsArrested     = false;
		m_PlayerEscorting = null;

		BH_StopEscort();

		// Si el NPC NO era enemigo (misma facción), simplemente restaurar combate
		if (!m_OriginalFaction)
		{
			BH_EnableAICombat();
			Print("[BH_Release] NPC aliado soltado. Combate restaurado.", LogLevel.NORMAL);
			m_OnRelease.Invoke(GetOwner(), player);
			return;
		}

		// El NPC era enemigo → aplicar comportamiento según configuración del editor
		switch (m_eReleaseBehavior)
		{
			case BH_EReleaseMode.SURRENDER:
			{
				// Se rinde: queda aliado (facción del jugador) sin combate
				BH_DisableAICombat();
				m_OriginalFaction = null;
				Print("[BH_Release] NPC enemigo se rinde. Queda aliado sin combate.", LogLevel.NORMAL);
				break;
			}

			case BH_EReleaseMode.RESTORE_ENEMY:
			{
				// Vuelve a ser enemigo: restaurar facción y combate
				BH_RestoreOriginalFaction();
				BH_EnableAICombat();
				Print("[BH_Release] NPC enemigo soltado. Facción enemiga restaurada.", LogLevel.NORMAL);
				break;
			}

			case BH_EReleaseMode.DISAPPEAR:
			{
				// Desaparece: eliminar la entidad
				m_OriginalFaction = null;
				Print("[BH_Release] NPC enemigo eliminado.", LogLevel.NORMAL);
				m_OnRelease.Invoke(GetOwner(), player);
				// Eliminar con un frame de delay para que el invoker se ejecute antes
				GetGame().GetCallqueue().CallLater(BH_DeleteNPC, 100, false);
				return;
			}

			case BH_EReleaseMode.NEUTRAL:
			{
				// Neutral: quitar facción, sin combate
				BH_SetNeutralFaction();
				BH_DisableAICombat();
				m_OriginalFaction = null;
				Print("[BH_Release] NPC enemigo soltado como neutral.", LogLevel.NORMAL);
				break;
			}
		}

		m_OnRelease.Invoke(GetOwner(), player);
	}

	// ----------------------------------------------------------
	//  ESCOLTA: iniciar
	// ----------------------------------------------------------

	protected void BH_StartEscort(IEntity player)
	{
		// Suscribirse a eventos de vehículo del jugador
		m_PlayerCompartmentAccess = SCR_CompartmentAccessComponent.Cast(player.FindComponent(SCR_CompartmentAccessComponent));
		if (m_PlayerCompartmentAccess)
		{
			m_PlayerCompartmentAccess.GetOnPlayerCompartmentEnter().Insert(BH_OnPlayerEnterVehicle);
			m_PlayerCompartmentAccess.GetOnPlayerCompartmentExit().Insert(BH_OnPlayerExitVehicle);
		}

		BH_EnsureAIGroup();
		BH_SetStateFollowing();

		// NUEVO: Activar watchdog también en escolta por arresto
		if (m_bEnableWatchdog)
		{
			m_Watchdog = new BH_EscortWatchdog();
			m_Watchdog.BH_StartWatching(GetOwner(), player, this);
		}
	}

	// ----------------------------------------------------------
	//  ESCOLTA: detener
	// ----------------------------------------------------------

	protected void BH_StopEscort()
	{
		// NUEVO: Detener watchdog
		if (m_Watchdog)
		{
			m_Watchdog.BH_StopWatching();
			m_Watchdog = null;
		}

		// NUEVO: Desvincular del grupo del jugador
		if (m_GroupIntegration)
		{
			m_GroupIntegration.BH_LeavePlayerGroup();
			m_GroupIntegration = null;
		}

		if (m_eEscortState == BH_EEscortState.INACTIVE)
			return;

		if (m_PlayerCompartmentAccess)
		{
			m_PlayerCompartmentAccess.GetOnPlayerCompartmentEnter().Remove(BH_OnPlayerEnterVehicle);
			m_PlayerCompartmentAccess.GetOnPlayerCompartmentExit().Remove(BH_OnPlayerExitVehicle);
			m_PlayerCompartmentAccess = null;
		}

		if (m_eEscortState == BH_EEscortState.IN_VEHICLE && m_NPCCompartmentAccess)
		{
			m_NPCCompartmentAccess.AskOwnerToGetOutFromVehicle(
				EGetOutType.ANIMATED, -1, ECloseDoorAfterActions.CLOSE_DOOR, false
			);
		}

		BH_ClearActiveWaypoint();
		ClearEventMask(GetOwner(), EntityEvent.FRAME);
		m_eEscortState = BH_EEscortState.INACTIVE;
	}

	// ----------------------------------------------------------
	//  EVENTOS DE VEHÍCULO DEL JUGADOR
	// ----------------------------------------------------------

	protected void BH_OnPlayerEnterVehicle(ChimeraCharacter playerCharacter, IEntity vehicleEntity)
	{
		if (m_eEscortState != BH_EEscortState.FOLLOWING || !vehicleEntity)
			return;

		BH_SetStateBoarding(vehicleEntity);
	}

	protected void BH_OnPlayerExitVehicle(ChimeraCharacter playerCharacter, IEntity vehicleEntity)
	{
		if (m_eEscortState != BH_EEscortState.IN_VEHICLE)
			return;

		BH_SetStateAlighting();
	}

	// ----------------------------------------------------------
	//  ESTADOS DE ESCOLTA
	// ----------------------------------------------------------

	protected void BH_SetStateFollowing()
	{
		BH_ClearActiveWaypoint();

		if (!m_EscortGroup || !m_PlayerEscorting)
			return;

		m_ActiveWaypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(
			Resource.Load("{A0509D3C4DD4475E}Prefabs/AI/Waypoints/AIWaypoint_Follow.et"),
			GetOwner().GetWorld(), null
		));

		if (!m_ActiveWaypoint)
			return;

		// Eliminar waypoints existentes para que el nuestro tenga prioridad
		array<AIWaypoint> existingWaypoints = {};
		m_EscortGroup.GetWaypoints(existingWaypoints);
		foreach (AIWaypoint wp : existingWaypoints)
			m_EscortGroup.RemoveWaypoint(wp);

		m_ActiveWaypoint.SetOrigin(m_PlayerEscorting.GetOrigin());

		// Asignar el jugador como entidad a seguir
		SCR_EntityWaypoint entityWP = SCR_EntityWaypoint.Cast(m_ActiveWaypoint);
		if (entityWP)
			entityWP.SetEntity(m_PlayerEscorting);

		m_EscortGroup.AddWaypoint(m_ActiveWaypoint);
		m_eEscortState = BH_EEscortState.FOLLOWING;
	}

	protected void BH_SetStateBoarding(IEntity vehicle)
	{
		BH_ClearActiveWaypoint();
		ClearEventMask(GetOwner(), EntityEvent.FRAME);
		m_eEscortState = BH_EEscortState.BOARDING;

		if (!m_NPCCompartmentAccess)
		{
			BH_SetStateFollowing();
			return;
		}

		// Intentar subir al asiento de pasajero disponible
		bool success = m_NPCCompartmentAccess.MoveInVehicle(vehicle, ECompartmentType.CARGO);
		if (!success)
			success = m_NPCCompartmentAccess.MoveInVehicle(vehicle, ECompartmentType.TURRET);

		if (success)
		{
			m_eEscortState = BH_EEscortState.IN_VEHICLE;
		}
		else
		{
			// Fallback: waypoint GetIn
			m_ActiveWaypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(
				Resource.Load("{712F4795CF8B91C7}Prefabs/AI/Waypoints/AIWaypoint_GetIn.et"),
				GetOwner().GetWorld(), null
			));

			if (m_ActiveWaypoint && m_EscortGroup)
			{
				m_ActiveWaypoint.SetOrigin(vehicle.GetOrigin());
				m_EscortGroup.AddWaypoint(m_ActiveWaypoint);
				m_eEscortState = BH_EEscortState.IN_VEHICLE;
			}
			else
			{
				BH_SetStateFollowing();
			}
		}
	}

	protected void BH_SetStateAlighting()
	{
		m_eEscortState = BH_EEscortState.ALIGHTING;

		if (m_NPCCompartmentAccess)
		{
			m_NPCCompartmentAccess.AskOwnerToGetOutFromVehicle(
				EGetOutType.ANIMATED, -1, ECloseDoorAfterActions.CLOSE_DOOR, false
			);
		}

		GetGame().GetCallqueue().CallLater(BH_SetStateFollowing, 2000, false);
	}

	// ----------------------------------------------------------
	//  NUEVO: REFRESCO DE WAYPOINT (llamado por el Watchdog)
	// ----------------------------------------------------------

	//! Llamado periódicamente por BH_EscortWatchdog para mantener
	//! el waypoint actualizado con la posición del jugador.
	//! También se puede llamar manualmente para forzar refresco.
	void BH_RefreshEscortWaypoint()
	{
		// Solo refrescar si estamos en modo FOLLOWING activo
		if (m_eEscortState != BH_EEscortState.FOLLOWING)
			return;

		if (!m_PlayerEscorting || !m_EscortGroup)
			return;

		// Si hay waypoint activo, actualizar su posición
		if (m_ActiveWaypoint)
		{
			m_ActiveWaypoint.SetOrigin(m_PlayerEscorting.GetOrigin());

			// Asegurarse de que el entity waypoint sigue apuntando al jugador
			SCR_EntityWaypoint entityWP = SCR_EntityWaypoint.Cast(m_ActiveWaypoint);
			if (entityWP)
				entityWP.SetEntity(m_PlayerEscorting);
		}
		else
		{
			// El waypoint se perdió de alguna forma, recrearlo
			Print("[BH_NPC] Waypoint perdido, recreando...", LogLevel.WARNING);
			BH_SetStateFollowing();
		}
	}

	// ----------------------------------------------------------
	//  CONTROL DE IA
	// ----------------------------------------------------------

	protected void BH_DisableAICombat()
	{
		if (m_AIConfig)
		{
			m_AIConfig.m_EnableAttack       = false;
			m_AIConfig.m_EnableDangerEvents = false;
			m_AIConfig.m_EnablePerception   = false;
			m_AIConfig.m_EnableTakeCover    = false;
			m_AIConfig.m_EnableMovement     = true;
		}
	}

	protected void BH_EnableAICombat()
	{
		if (m_AIConfig)
		{
			m_AIConfig.m_EnableAttack       = true;
			m_AIConfig.m_EnableDangerEvents = true;
			m_AIConfig.m_EnablePerception   = true;
			m_AIConfig.m_EnableTakeCover    = true;
			m_AIConfig.m_EnableMovement     = true;
		}
	}

	// ----------------------------------------------------------
	//  HELPERS
	// ----------------------------------------------------------

	protected void BH_EnsureAIGroup()
	{
		if (!m_AIAgent && m_AIControl)
			m_AIAgent = m_AIControl.GetControlAIAgent();

		if (!m_AIAgent)
			m_AIAgent = AIAgent.Cast(GetOwner().FindComponent(AIAgent));

		if (!m_AIAgent)
			return;

		m_EscortGroup = AIGroup.Cast(m_AIAgent.GetParentGroup());

		if (!m_EscortGroup)
		{
			m_EscortGroup = AIGroup.Cast(GetGame().SpawnEntityPrefab(
				Resource.Load("{059D0D6C49812B43}Prefabs/AI/Groups/Group_Base.et"),
				GetOwner().GetWorld(), null
			));

			if (m_EscortGroup)
				m_EscortGroup.AddAgent(m_AIAgent);
		}
	}

	protected void BH_ClearActiveWaypoint()
	{
		if (m_ActiveWaypoint && m_EscortGroup)
		{
			m_EscortGroup.RemoveWaypoint(m_ActiveWaypoint);
			SCR_EntityHelper.DeleteEntityAndChildren(m_ActiveWaypoint);
			m_ActiveWaypoint = null;
		}
	}

	protected void BH_LookAtEntity(IEntity target)
	{
		if (!target)
			return;

		vector dir = target.GetOrigin() - GetOwner().GetOrigin();
		dir.Normalize();

		float yaw = Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG;
		GetOwner().SetYawPitchRoll(Vector(yaw, 0, 0));
	}

	protected void BH_ExecuteActions(array<ref SCR_ScenarioFrameworkActionBase> actions)
	{
		if (!actions || actions.IsEmpty())
			return;

		foreach (SCR_ScenarioFrameworkActionBase action : actions)
		{
			if (action)
				action.OnActivate(GetOwner());
		}
	}

	// ----------------------------------------------------------
	//  CAMBIO DE FACCIÓN
	// ----------------------------------------------------------

	//! Cambia la facción del NPC usando una faction key (ej: "US", "USSR", "FIA")
	protected void BH_SwitchFactionByKey(string factionKey)
	{
		if (factionKey == string.Empty)
			return;

		FactionAffiliationComponent npcFactionComp = FactionAffiliationComponent.Cast(
			GetOwner().FindComponent(FactionAffiliationComponent)
		);
		if (!npcFactionComp)
		{
			Print("[BH_Faction] NPC no tiene FactionAffiliationComponent.", LogLevel.WARNING);
			return;
		}

		FactionManager factionMgr = GetGame().GetFactionManager();
		if (!factionMgr)
		{
			Print("[BH_Faction] No hay FactionManager.", LogLevel.WARNING);
			return;
		}

		Faction targetFaction = factionMgr.GetFactionByKey(factionKey);
		if (!targetFaction)
		{
			// Listar facciones disponibles para ayudar al editor a encontrar la correcta
			string available = "";
			int count = factionMgr.GetFactionsCount();
			for (int i = 0; i < count; i++)
			{
				Faction f = factionMgr.GetFactionByIndex(i);
				if (f)
				{
					if (available != "")
						available = available + ", ";
					available = available + f.GetFactionKey();
				}
			}
			Print(string.Format("[BH_Faction] ERROR: Facción '%1' no encontrada. Facciones disponibles: [%2]", factionKey, available), LogLevel.ERROR);
			return;
		}

		Faction currentFaction = npcFactionComp.GetAffiliatedFaction();
		if (currentFaction == targetFaction)
		{
			Print(string.Format("[BH_Faction] NPC ya pertenece a la facción '%1'.", factionKey), LogLevel.NORMAL);
			return;
		}

		string fromKey = "ninguna";
		if (currentFaction)
			fromKey = currentFaction.GetFactionKey();

		npcFactionComp.SetAffiliatedFaction(targetFaction);
		Print(string.Format("[BH_Faction] Facción del NPC cambiada: %1 → %2", fromKey, factionKey), LogLevel.NORMAL);
	}

	//! Guarda la facción original del NPC y la cambia a la del jugador
	//! Usado al arrestar NPCs de otra facción
	protected void BH_SwitchFactionToPlayer(IEntity player)
	{
		if (!player)
			return;

		FactionAffiliationComponent npcFactionComp = FactionAffiliationComponent.Cast(
			GetOwner().FindComponent(FactionAffiliationComponent)
		);
		if (!npcFactionComp)
		{
			Print("[BH_Faction] NPC no tiene FactionAffiliationComponent.", LogLevel.WARNING);
			return;
		}

		// Guardar facción original
		m_OriginalFaction = npcFactionComp.GetAffiliatedFaction();

		// Obtener facción del jugador
		Faction playerFaction;
		int playerID = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(player);
		if (playerID > 0)
			playerFaction = SCR_FactionManager.SGetPlayerFaction(playerID);

		if (!playerFaction)
		{
			Print("[BH_Faction] No se pudo obtener la facción del jugador.", LogLevel.WARNING);
			return;
		}

		// Si ya son de la misma facción, no hacer nada
		if (m_OriginalFaction == playerFaction)
		{
			m_OriginalFaction = null; // No necesita restaurarse
			Print("[BH_Faction] NPC ya es de la misma facción que el jugador.", LogLevel.NORMAL);
			return;
		}

		// Cambiar facción del NPC a la del jugador
		npcFactionComp.SetAffiliatedFaction(playerFaction);
		Print(string.Format("[BH_Faction] Facción del NPC cambiada a: %1", playerFaction.GetFactionKey()), LogLevel.NORMAL);
	}

	//! Restaura la facción original del NPC (si fue cambiada)
	protected void BH_RestoreOriginalFaction()
	{
		if (!m_OriginalFaction)
			return;

		FactionAffiliationComponent npcFactionComp = FactionAffiliationComponent.Cast(
			GetOwner().FindComponent(FactionAffiliationComponent)
		);
		if (!npcFactionComp)
			return;

		npcFactionComp.SetAffiliatedFaction(m_OriginalFaction);
		Print(string.Format("[BH_Faction] Facción del NPC restaurada a: %1", m_OriginalFaction.GetFactionKey()), LogLevel.NORMAL);
		m_OriginalFaction = null;
	}

	//! Pone al NPC como neutral (sin facción)
	protected void BH_SetNeutralFaction()
	{
		FactionAffiliationComponent npcFactionComp = FactionAffiliationComponent.Cast(
			GetOwner().FindComponent(FactionAffiliationComponent)
		);
		if (!npcFactionComp)
			return;

		npcFactionComp.SetAffiliatedFaction(null);
		Print("[BH_Faction] NPC establecido como neutral (sin facción).", LogLevel.NORMAL);
	}

	//! Elimina al NPC (llamado con delay desde BH_OnRelease modo DISAPPEAR)
	protected void BH_DeleteNPC()
	{
		IEntity owner = GetOwner();
		if (owner)
			SCR_EntityHelper.DeleteEntityAndChildren(owner);
	}
}