//------------------------------------------------------------------------------------------------
class BH_FakePilotCompartmentSlot : PilotCompartmentSlot
{
}

//------------------------------------------------------------------------------------------------

[EntityEditorProps(category: "BH/Props/", description: "Ambient Helicopter Controller", color: "96 255 0 255")]
class BH_AmbientHelicopterControllerComponentClass : ScriptComponentClass
{
}

class BH_AmbientHelicopterControllerComponent : ScriptComponent
{
    [Attribute("", UIWidgets.Auto, desc: "Waypoints del helicóptero", category: "BH Helicóptero Ambiental")]
    protected ref array<ref BH_AmbientHelicopterWaypoint> m_Waypoins;

    [Attribute("1", UIWidgets.Auto, desc: "Frecuencia de actualización de la simulación en segundos", params: "0.5 3", category: "BH Helicóptero Ambiental")]
    protected float m_RefreshRate;

    [Attribute("100", UIWidgets.Auto, desc: "Velocidad máxima en vuelo", params: "20 120", category: "BH Helicóptero Ambiental")]
    protected float m_MaxSpeed;

    [Attribute("0", UIWidgets.CheckBox, desc: "Ralentizar y atacar si hay jugadores cerca en el suelo", category: "BH Helicóptero Ambiental")]
    protected bool m_AttackPlayers;

    [Attribute("120", UIWidgets.Auto, desc: "Durante cuántos segundos debe atacar", params: "20 500", category: "BH Helicóptero Ambiental")]
    protected int m_AttackPlayersTimeout;

    [Attribute("30", UIWidgets.Auto, desc: "Altura mínima sobre el terreno durante el vuelo (metros)", params: "10 100", category: "BH Helicóptero Ambiental")]
    protected int m_MinHeightAboveTerrain;

    [Attribute("40", UIWidgets.Auto, desc: "Distancia mínima al waypoint para cambiar al siguiente", params: "30 60", category: "BH Helicóptero Ambiental")]
    protected int m_MinWaypointDistance;

    [Attribute("2", UIWidgets.Auto, desc: "Fuerza base del rotor de cola (vuelo recto)", params: "0 4", category: "BH Helicóptero Ambiental")]
    protected float m_defaultRotationPower;

    [Attribute("2", UIWidgets.Auto, desc: "Fuerza del rotor de cola al arrancar", params: "0 4", category: "BH Helicóptero Ambiental")]
    protected float m_startRotationPower;

    [Attribute("0", UIWidgets.CheckBox, desc: "¿Invertir dirección de rotación? (horario / antihorario)", category: "BH Helicóptero Ambiental")]
    protected bool m_tailRotorClockwise;

    [Attribute("15", UIWidgets.Auto, desc: "Velocidad de giro (valor menor = giro más rápido, valor mayor = giro más lento)", params: "10 20", category: "BH Helicóptero Ambiental")]
    protected float m_RotationPowerDividor;

    [Attribute("1.3", UIWidgets.Auto, desc: "Potencia base del rotor principal", params: "1 3", category: "BH Helicóptero Ambiental")]
    protected float m_defaultMainRotorPower;

    [Attribute("0", UIWidgets.Auto, desc: "Reducción de daño entrante al helicóptero (0-1, donde 1 = 100%)", params: "0 1", category: "BH Helicóptero Ambiental")]
    protected float m_reduceDamage;

    // Lights
    [Attribute("1", UIWidgets.CheckBox, desc: "Activar luces traseras", category: "BH Helicóptero Ambiental (Luces)")]
    protected bool m_TurnOnLights;

    [Attribute("1", UIWidgets.CheckBox, desc: "Activar luces de aterrizaje (HiBeam) al acercarse al waypoint", category: "BH Helicóptero Ambiental (Luces)")]
    protected bool m_TurnOnLandingLights;

    [Attribute("0", UIWidgets.CheckBox, desc: "Activar foco de búsqueda", category: "BH Helicóptero Ambiental (Luces)")]
    protected bool m_TurnOnSearchLight;

    [Attribute("0", UIWidgets.CheckBox, desc: "Activar luces de peligro (hazard)", category: "BH Helicóptero Ambiental (Luces)")]
    protected bool m_TurnOnHazzardLight;

    // Crew
    [Attribute("{42A502E3BB727CEB}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_HeliPilot.et", UIWidgets.ResourcePickerThumbnail, desc: "Prefab del piloto", params: "et", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected ResourceName m_PilotPrefab;

    [Attribute("{15CD521098748195}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_HeliCrew.et", UIWidgets.ResourcePickerThumbnail, desc: "Prefab del copiloto", params: "et", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected ResourceName m_CoPilotPrefab;

    [Attribute("0", UIWidgets.CheckBox, desc: "¿El helicóptero tiene artillero/s?", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected bool m_GunnerSlotsAvailable;

    [Attribute("2", UIWidgets.Auto, desc: "Número de artilleros a spawnear", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected int m_GunnerSlotCount;

    [Attribute("{15CD521098748195}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_HeliCrew.et", UIWidgets.ResourcePickerThumbnail, desc: "Prefab del artillero", params: "et", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected ResourceName m_GunnerPrefab;

    [Attribute("1", UIWidgets.CheckBox, desc: "Hacer a la tripulación invulnerable", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected bool m_CrewIsinvulnerable;

    [Attribute("0.5", UIWidgets.Auto, desc: "Reducción de daño a la tripulación (0.5 = 50%, 1 = 100%)", params: "0 1", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected float m_CrewDamageReduction;

    [Attribute("1", UIWidgets.CheckBox, desc: "Matar a la tripulación si el helicóptero se estrella", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected bool m_KillCrewOnCrash;

    // Debug
    [Attribute("0", UIWidgets.CheckBox, desc: "Activar modo debug (imprime logs detallados en consola para diagnosticar problemas)", category: "BH Helicóptero Ambiental (Debug)")]
    protected bool m_DebugModeAttr;

    // Internal vars
    protected bool m_spawnIsDone = false;
    protected SCR_AIGroup m_crewGroup;
    protected IEntity m_owner;
    IEntity m_PilotEnt;
    IEntity m_CoPilotEnt;
    protected IEntity m_GunnerLastTarget;
    ref array<IEntity> m_aGunners = new array<IEntity>();
    protected int m_currentAttackTimer = 0;
    protected SCR_AIGroupUtilityComponent m_aiGroupUtil;

    protected VehicleHelicopterSimulation m_simulation;
    protected BaseLightManagerComponent m_lightMgr;
    protected SCR_VehicleDamageManagerComponent m_dmgMgr;
    protected SignalsManagerComponent m_signalsMgr;
    protected SCR_VehicleBuoyancyComponent m_vehicleBuoyancy;
    protected BaseWorld m_world;

    protected float m_simSteeringState = 0;
    protected float m_simSpeedState = 0;
    protected bool m_simIsHardSteering = false;
    bool m_simIsDestroyed = false;
    protected bool m_simShouldNailOnGround = false;

    protected int m_sSpeed = 0;
    protected int m_sRollAngle = 0;
    protected int m_sYawAngle = 0;
    protected int m_sPitchAngle = 0;
	
	protected string m_sRadioFactionKey;

    ref BH_AmbientHelicopterWaypoint m_currentWPdata;
    bool m_isLandingPhase = false;
    IEntity m_currentWP;
    int m_currentWPIndex = 0;
    float m_currentWPheight = 0;
    vector m_currentWPTransform[4];
    protected float m_tempSpeed = 100;

    bool m_OnAttackActionsCalled = false;
    bool m_OnDamageActionsCalled = false;
    protected SCR_ScenarioFrameworkSlotAmbientHelicopter m_SlotBase;
	
	// Debug
	protected bool m_DebugMode = false;
	
	// Lock
	protected bool m_LockForPlayers = false;
	
	// Variables sistema embarque
	protected bool m_BoardingStarted = false;
	protected int m_PassengersOnBoard = 0;
	protected bool m_MinReached = false;

    //------------------------------------------------------------------------------------------------
	
	// Helper de debug - solo imprime si m_DebugMode esta activo
	protected void BH_Log(string msg)
	{
		if (m_DebugMode)
			Print("[BH_Heli_DBG] " + msg);
	}

    //------------------------------------------------------------------------------------------------

   override protected void OnPostInit(IEntity owner)
	{
	    Print("[BH_Heli] OnPostInit START");
	
	    if (SCR_Global.IsEditMode()) { Print("[BH_Heli] IsEditMode, saliendo"); return; }
	
	    // Asignar m_owner SIEMPRE (server y clientes) para que los RPCs funcionen
	    m_owner = owner;
	
	    RplComponent rplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
	    if (!rplComp) { Print("[BH_Heli] ERROR: No RplComponent"); return; }
	    if (!rplComp.IsMaster()) { Print("[BH_Heli] No es master, saliendo (m_owner asignado para RPCs)"); return; }
	
	    Print("[BH_Heli] OnPostInit OK, seteando EventMask INIT");
	    SetEventMask(owner, EntityEvent.INIT);
	}

    //------------------------------------------------------------------------------------------------

    override protected void EOnInit(IEntity owner)
	{
	    Print("[BH_Heli] EOnInit START");
	
	    m_lightMgr = BaseLightManagerComponent.Cast(owner.FindComponent(BaseLightManagerComponent));
	    if (!m_lightMgr) { Print("[BH_Heli] ERROR: No m_lightMgr"); return; }
	
	    m_simulation = VehicleHelicopterSimulation.Cast(owner.FindComponent(VehicleHelicopterSimulation));
	    if (!m_simulation) { Print("[BH_Heli] ERROR: No m_simulation"); return; }
	
	    m_dmgMgr = SCR_VehicleDamageManagerComponent.Cast(owner.FindComponent(SCR_VehicleDamageManagerComponent));
	    if (!m_dmgMgr) { Print("[BH_Heli] ERROR: No m_dmgMgr"); return; }
	
	    m_signalsMgr = SignalsManagerComponent.Cast(owner.FindComponent(SignalsManagerComponent));
	    if (!m_signalsMgr) { Print("[BH_Heli] ERROR: No m_signalsMgr"); return; }
	
	    m_world = GetGame().GetWorld();
	    if (!m_world) { Print("[BH_Heli] ERROR: No m_world"); return; }
	
	    BaseCompartmentManagerComponent compartMgr = BaseCompartmentManagerComponent.Cast(owner.FindComponent(BaseCompartmentManagerComponent));
	    if (!compartMgr) { Print("[BH_Heli] ERROR: No compartMgr"); return; }
	
	    bool fakeCompartmentSet = false;
	    array<BaseCompartmentSlot> aSlots = new array<BaseCompartmentSlot>();
	    compartMgr.GetCompartments(aSlots);
	    Print("[BH_Heli] Slots encontrados: " + aSlots.Count());
	
	    foreach (BaseCompartmentSlot slot : aSlots)
	    {
	        Print("[BH_Heli] Slot tipo: " + slot.Type());
	        if (slot == BH_FakePilotCompartmentSlot.Cast(slot))
	        {
	            Print("[BH_Heli] FakePilotSlot encontrado!");
	            slot.SetReserved(m_owner);
	            slot.SetCompartmentAccessible(false);
	
	            BaseVehicleControllerComponent vController = BaseVehicleControllerComponent.Cast(m_owner.FindComponent(BaseVehicleControllerComponent));
	            if (vController)
	            {
	                vController.SetPilotCompartmentSlot(PilotCompartmentSlot.Cast(slot));
	                fakeCompartmentSet = true;
	                Print("[BH_Heli] vController configurado OK");
	            }
	            else Print("[BH_Heli] ERROR: No vController");
	            break;
	        }
	    }
	
	    if (!fakeCompartmentSet) { Print("[BH_Heli] ERROR: fakeCompartmentSet = false, abortando"); return; }
	
	    Print("[BH_Heli] EOnInit completado OK");
	
	    m_dmgMgr.GetOnDamageStateChanged().Insert(OnVehicleDamageStateChanged);
	    m_dmgMgr.GetOnVehicleDestroyed().Insert(OnVehicleDestroyed);
	
	    m_vehicleBuoyancy = SCR_VehicleBuoyancyComponent.Cast(owner.FindComponent(SCR_VehicleBuoyancyComponent));
	    if (m_vehicleBuoyancy) m_vehicleBuoyancy.GetOnWaterEnter().Insert(OnWaterEnter);
	
	    const float heightAboveT = SCR_TerrainHelper.GetHeightAboveTerrain(m_owner.GetOrigin(), m_world, true);
	    m_simShouldNailOnGround = true;
	    BH_NailOnGround();

	    // Aplicar debug del atributo del componente (el Slot puede sobreescribirlo despues via BH_SetDebugMode)
	    if (m_DebugModeAttr) m_DebugMode = true;

	    GetGame().GetCallqueue().CallLater(OnInitComplete, 1000, false);
	}

    //------------------------------------------------------------------------------------------------

    void OnInitComplete()
    {
        if (m_PilotEnt)
        {
            GetGame().GetCallqueue().CallLater(OnStartFromSerializer, 1000);
            return;
        }
        OnStartDelayed();
    }

    //------------------------------------------------------------------------------------------------

    void OnStartFromSerializer()
    {
        if (m_simIsDestroyed) return;

        if (!m_currentWP) BH_SetWaypoint(m_currentWPIndex);

        const float heightAboveT = SCR_TerrainHelper.GetHeightAboveTerrain(m_owner.GetOrigin(), m_world, true);
        if (heightAboveT > 10)
        {
            m_simShouldNailOnGround = false;
            BH_StartEngine(true);
            GetGame().GetCallqueue().Remove(BH_Simulate);
            GetGame().GetCallqueue().CallLater(BH_Simulate, 500, false);
        }
        else
        {
            m_simShouldNailOnGround = true;
            BH_NailOnGround();
        }

        if (m_PilotEnt && !m_CrewIsinvulnerable)
        {
            SCR_CharacterControllerComponent pilotController = SCR_CharacterControllerComponent.Cast(m_PilotEnt.FindComponent(SCR_CharacterControllerComponent));
            if (pilotController) pilotController.m_OnLifeStateChanged.Insert(OnLifeStateChanged_pilot);
        }

        BH_ApplyCrewDamageReduction();
        BH_ApplyHelicopterDamageReduction();
    }

    //------------------------------------------------------------------------------------------------

    protected void OnStartDelayed()
    {
        if (!m_spawnIsDone)
        {
            if (!m_crewGroup) m_crewGroup = BH_CreateHelicopterGroup();

            if (!m_PilotEnt) GetGame().GetCallqueue().CallLater(BH_SpawnPilot, 100, false, m_PilotPrefab, ECompartmentType.PILOT);
            if (!m_CoPilotEnt) GetGame().GetCallqueue().CallLater(BH_SpawnPilot, 1000, false, m_CoPilotPrefab, ECompartmentType.PILOT);

            if (m_GunnerSlotsAvailable && m_GunnerSlotCount > 0 && m_aGunners.Count() != m_GunnerSlotCount)
            {
                int delay = 1500;
                for (int i = 0; i < m_GunnerSlotCount; i++)
                {
                    GetGame().GetCallqueue().CallLater(BH_SpawnGunner, delay, false, m_GunnerPrefab);
                    delay += 1000;
                }
            }

            BH_ApplyHelicopterDamageReduction();

            m_spawnIsDone = true;
            BH_Log("OnStartDelayed: spawn completado, esperando 2s para continuar");
            GetGame().GetCallqueue().CallLater(OnStartDelayed, 2000, false);
            return;
        }

        BH_Log("OnStartDelayed: spawn done, waypoints=" + m_Waypoins.Count());

        if (m_Waypoins.Count() > 0)
        {
            BH_AmbientHelicopterWaypoint firstWP = m_Waypoins[0];

            if (firstWP.m_WaitForPlayersGetIn)
            {
                BH_SetWaypoint(0);
                BH_ResetBoardingState();
                
                // Desbloquear helicóptero para que puedan subir los jugadores
                BH_UnlockHelicopter();
                
                BH_Log("OnStartDelayed: WP0 espera embarque, iniciando BH_BoardingCheck");
                GetGame().GetCallqueue().Remove(BH_BoardingCheck);
                GetGame().GetCallqueue().CallLater(BH_BoardingCheck, 2000, false);
                return;
            }

            BH_Log("OnStartDelayed: arrancando motor y simulacion");
            BH_StartEngine();
			GetGame().GetCallqueue().Remove(BH_Simulate);
			GetGame().GetCallqueue().CallLater(BH_Simulate, 1000, false);
        }
        else
        {
            BH_Log("OnStartDelayed: AVISO - no hay waypoints configurados, el helicoptero no hara nada");
        }
    }

    //------------------------------------------------------------------------------------------------

    override protected void OnDelete(IEntity owner)
    {
        if (m_PilotEnt) SCR_EntityHelper.DeleteEntityAndChildren(m_PilotEnt);
        if (m_CoPilotEnt) SCR_EntityHelper.DeleteEntityAndChildren(m_CoPilotEnt);
        if (m_dmgMgr)
        {
            m_dmgMgr.GetOnVehicleDestroyed().Remove(OnVehicleDestroyed);
            m_dmgMgr.GetOnDamageStateChanged().Remove(OnVehicleDamageStateChanged);
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void OnVehicleDestroyed(int playerId)
    {
        if (m_dmgMgr.GetHealth() < 1)
        {
            m_dmgMgr.GetOnVehicleDestroyed().Remove(OnVehicleDestroyed);

            if (m_PilotEnt) SCR_EntityHelper.DeleteEntityAndChildren(m_PilotEnt);
            if (m_CoPilotEnt) SCR_EntityHelper.DeleteEntityAndChildren(m_CoPilotEnt);

            foreach (IEntity gunner : m_aGunners)
                SCR_EntityHelper.DeleteEntityAndChildren(gunner);

            m_simIsDestroyed = true;
            if (m_simulation)
            {
                m_simulation.SetThrottle(0);
                m_simulation.EngineStop();
            }

            if (m_TurnOnLights) BH_ToggleLights(false);
            if (m_TurnOnSearchLight) BH_ToggleSearchLights(false);
            if (m_TurnOnHazzardLight) BH_ToggleHazzardLights(false);

            if (m_SlotBase && !m_OnDamageActionsCalled)
            {
                m_OnDamageActionsCalled = true;
                m_SlotBase.BH_ActionOnDamaged();
            }

            SCR_GarbageSystem garbageSystem = SCR_GarbageSystem.GetByEntityWorld(m_owner);
            if (garbageSystem) garbageSystem.Insert(m_owner, 300, true);

            SCR_HelicopterSoundComponent soundComp = SCR_HelicopterSoundComponent.Cast(m_owner.FindComponent(SCR_HelicopterSoundComponent));
            if (soundComp) soundComp.TerminateAll();
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void OnVehicleDamageStateChanged(EDamageState state)
    {
        if (!m_simIsDestroyed && m_simulation.RotorGetState(0) == RotorState.DESTROYED) m_simIsDestroyed = true;
        if (!m_simIsDestroyed && m_simulation.RotorGetState(1) == RotorState.DESTROYED) m_simIsDestroyed = true;
        if (!m_simIsDestroyed && state == EDamageState.DESTROYED) m_simIsDestroyed = true;
        if (!m_simIsDestroyed && m_dmgMgr.GetDamageOverTime(EDamageType.FIRE) > 0) m_simIsDestroyed = true;
        if (!m_simIsDestroyed && m_dmgMgr.GetMovementDamage() > 0.5) m_simIsDestroyed = true;

        if (m_simIsDestroyed)
        {
            BH_Log("OnVehicleDamageStateChanged: helicoptero DESTRUIDO/danado critico, state=" + state);
            SCR_GarbageSystem garbageSystem = SCR_GarbageSystem.GetByEntityWorld(m_owner);
            if (m_dmgMgr) m_dmgMgr.GetOnDamageStateChanged().Remove(OnVehicleDamageStateChanged);

            if (m_KillCrewOnCrash)
            {
                SCR_CharacterDamageManagerComponent CharDmgMgr;
                Instigator killerEnt = m_dmgMgr.GetInstigator();

                if (m_PilotEnt)
                {
                    CharDmgMgr = SCR_CharacterDamageManagerComponent.Cast(m_PilotEnt.FindComponent(SCR_CharacterDamageManagerComponent));
                    if (CharDmgMgr)
                    {
                        BH_SetCharacterBloody(CharDmgMgr);
                        CharDmgMgr.Kill(killerEnt);
                        if (garbageSystem) garbageSystem.Insert(m_PilotEnt, 300, true);
                    }
                }

                if (m_CoPilotEnt)
                {
                    CharDmgMgr = SCR_CharacterDamageManagerComponent.Cast(m_CoPilotEnt.FindComponent(SCR_CharacterDamageManagerComponent));
                    if (CharDmgMgr)
                    {
                        BH_SetCharacterBloody(CharDmgMgr);
                        CharDmgMgr.Kill(killerEnt);
                        if (garbageSystem) garbageSystem.Insert(m_CoPilotEnt, 300, true);
                    }
                }

                foreach (IEntity gunner : m_aGunners)
                {
                    if (gunner)
                    {
                        CharDmgMgr = SCR_CharacterDamageManagerComponent.Cast(gunner.FindComponent(SCR_CharacterDamageManagerComponent));
                        if (CharDmgMgr)
                        {
                            BH_SetCharacterBloody(CharDmgMgr);
                            CharDmgMgr.Kill(killerEnt);
                            if (garbageSystem) garbageSystem.Insert(gunner, 300, true);
                        }
                    }
                }
            }

            if (m_SlotBase && !m_OnDamageActionsCalled)
            {
                m_OnDamageActionsCalled = true;
                m_SlotBase.BH_ActionOnDamaged();
            }

            if (garbageSystem) garbageSystem.Insert(m_owner, 301, true);
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void OnWaterEnter()
    {
        if (m_vehicleBuoyancy) m_vehicleBuoyancy.GetOnWaterEnter().Remove(OnWaterEnter);

        if (m_dmgMgr)
        {
            Instigator dmgDealer = m_dmgMgr.GetInstigator();
            m_dmgMgr.Kill(dmgDealer);
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_SetCharacterBloody(SCR_CharacterDamageManagerComponent dmgMgr)
    {
        if (!dmgMgr) return;

        array<HitZone> hitzones = new array<HitZone>();
        dmgMgr.GetAllHitZonesInHierarchy(hitzones);
        foreach (HitZone hZone : hitzones)
        {
            SCR_CharacterHitZone charHitZone = SCR_CharacterHitZone.Cast(hZone);
            if (charHitZone)
            {
                dmgMgr.AddBloodToClothes(charHitZone, 255);
                charHitZone.SetWoundedSubmesh(true);
            }
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_ApplyDamageReductionToEntity(IEntity ent, float reduction)
	{
	    // No hay API pública en Reforger para reducción parcial de daño por HitZone
	    // Dejamos la entidad con daño normal
	}

    //------------------------------------------------------------------------------------------------

    protected void BH_ApplyCrewDamageReduction()
	{
	    // Sin implementación - crew recibe daño normal
	}

    //------------------------------------------------------------------------------------------------

    protected void BH_ApplyHelicopterDamageReduction()
	{
	    // Sin implementación - helicóptero recibe daño normal
	}

    //------------------------------------------------------------------------------------------------

    protected bool BH_SetWaypoint(int index)
    {
        if (m_Waypoins.Count() <= index)
        {
            BH_Log("BH_SetWaypoint FALLO: indice " + index + " fuera de rango (total WPs: " + m_Waypoins.Count() + ")");
            return false;
        }

        m_currentWPdata = m_Waypoins[index];
		m_sRadioFactionKey = m_currentWPdata.m_sRadioFactionKey;
        m_currentWP = m_world.FindEntityByName(m_currentWPdata.m_WaypoinName);
        if (!m_currentWP)
        {
            BH_Log("BH_SetWaypoint FALLO: no se encontro entidad con nombre '" + m_currentWPdata.m_WaypoinName + "' para WP index " + index);
            return false;
        }

        if (m_currentWPdata.m_DoOverrideMaxSpeed)
        {
            m_tempSpeed = m_MaxSpeed;
            m_MaxSpeed = m_currentWPdata.m_OverrideMaxSpeed;
        }
        else
        {
            if (m_tempSpeed > 20) m_MaxSpeed = m_tempSpeed;
        }

        if (m_currentWPdata.m_DoOverrideRotationPowerDividor) m_RotationPowerDividor = m_currentWPdata.m_OverrideRotationPowerDividor;
        if (m_currentWPdata.m_DoOverrideMinHeightAboveTerrain) m_MinHeightAboveTerrain = m_currentWPdata.m_OverrideMinHeightAboveTerrain;

        m_currentWP.GetWorldTransform(m_currentWPTransform);
        float terrainH = m_world.GetSurfaceY(m_currentWPTransform[3][0], m_currentWPTransform[3][2]);
        m_currentWPTransform[3][1] = m_MinHeightAboveTerrain + terrainH;
        m_currentWP.SetWorldTransform(m_currentWPTransform);
        m_currentWPheight = m_MinHeightAboveTerrain + terrainH;

        BH_Log("BH_SetWaypoint OK: index=" + index + " nombre='" + m_currentWPdata.m_WaypoinName + "' landing=" + m_currentWPdata.m_IsLandingEvent + " waitGetIn=" + m_currentWPdata.m_WaitForPlayersGetIn + " waitGetOut=" + m_currentWPdata.m_WaitForPlayersGetOut + " delete=" + m_currentWPdata.m_DeleteHelicopter);
        return true;
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_ComputeRotation(float rotationToWp, float cSpeed)
    {
        float requiredAngle = rotationToWp;
        if (requiredAngle < 0) requiredAngle = Math.AbsFloat(requiredAngle);

        float newTailForce = (requiredAngle / m_RotationPowerDividor) * (cSpeed / 100);

        if (rotationToWp > 0)
        {
            if (!m_simIsHardSteering)
            {
                float diff = m_simSteeringState / (m_defaultRotationPower - newTailForce);
                if (diff > 2 || diff < 0.2)
                {
                    newTailForce = newTailForce / 2;
                    m_simIsHardSteering = true;
                }
            }
            else m_simIsHardSteering = false;

            if (cSpeed < 15) newTailForce = newTailForce / 2;

            if (m_tailRotorClockwise)
                m_simSteeringState = m_defaultRotationPower + newTailForce;
            else
                m_simSteeringState = m_defaultRotationPower - newTailForce;
        }
        else
        {
            if (!m_simIsHardSteering)
            {
                float diff = m_simSteeringState / (m_defaultRotationPower + newTailForce);
                if (diff > 2 || diff < 0.2)
                {
                    newTailForce = newTailForce / 2;
                    m_simIsHardSteering = true;
                }
            }
            else m_simIsHardSteering = false;

            if (cSpeed < 15) newTailForce = newTailForce / 2;

            if (m_tailRotorClockwise)
                m_simSteeringState = m_defaultRotationPower - newTailForce;
            else
                m_simSteeringState = m_defaultRotationPower + newTailForce;
        }

        m_simulation.RotorSetForceScaleState(1, m_simSteeringState);
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_Simulate()
    {
        if (!m_simIsDestroyed && m_simulation.EngineIsOn())
        {
            if (m_simulation.RotorGetState(0) == RotorState.DESTROYED || m_simulation.RotorGetState(1) == RotorState.DESTROYED)
            {
                m_simIsDestroyed = true;
                OnVehicleDamageStateChanged(EDamageState.DESTROYED);
                return;
            }

            const Physics ownerPhysics = m_owner.GetPhysics();
            if (!ownerPhysics) return;

            const float yaw = m_signalsMgr.GetSignalValue(m_sYawAngle);
            const float roll = m_signalsMgr.GetSignalValue(m_sRollAngle);
            const float cSpeed = m_signalsMgr.GetSignalValue(m_sSpeed);
            const float cAlt = m_simulation.GetAltitudeAGL();

            // Misalignment fix
            if (cAlt > 8 && (roll > 2.7 || roll < -2.7))
                m_owner.SetYawPitchRoll(Vector(yaw, 0, 0));

            // Handle waypoint
            if (!m_currentWP && !BH_SetWaypoint(m_currentWPIndex)) return;

            vector ownerOrigin = m_owner.GetOrigin();
            float distanceToWP = vector.DistanceXZ(m_currentWP.GetOrigin(), ownerOrigin);

            if (distanceToWP < m_MinWaypointDistance)
            {
                if (m_currentWPdata.m_DeleteHelicopter)
                {
                    BH_Log("Simulate: WP index=" + m_currentWPIndex + " con DeleteHelicopter, eliminando entidad");
                    SCR_EntityHelper.DeleteEntityAndChildren(m_owner);
                    return;
                }

                if (!m_isLandingPhase)
                {
                    if (m_Waypoins.Count() - 1 > m_currentWPIndex)
                    {
                        if (m_SlotBase) m_SlotBase.BH_ActionOnWaypoint(m_currentWPIndex);

                        m_currentWPIndex++;
                        BH_Log("Simulate: avanzando a WP index=" + m_currentWPIndex);

                        if (BH_SetWaypoint(m_currentWPIndex))
                            distanceToWP = vector.Distance(m_currentWP.GetOrigin(), ownerOrigin);
                        else
                            return;
                    }
                    else
                    {
                        BH_Log("Simulate: ultimo WP alcanzado, volviendo a WP 0 (loop)");
                        m_currentWPIndex = 0;
                        BH_SetWaypoint(m_currentWPIndex);
                    }
                }
            }

            if (m_currentWPdata.m_IsLandingEvent && !m_isLandingPhase && distanceToWP < 350)
            {
                if (m_TurnOnLandingLights) BH_ToggleLandingLights(true);
                m_isLandingPhase = true;
                BH_Log("Simulate: iniciando fase de aterrizaje en WP index=" + m_currentWPIndex + " dist=" + distanceToWP);
            }

            // Handle height
            if (distanceToWP > m_MinWaypointDistance)
            {
                const float terrainDistance = 70 + ownerPhysics.GetVelocity().Length();
                const vector direction = m_owner.GetYawPitchRoll().AnglesToVector();
                const vector forwardVector = ownerOrigin + (direction * terrainDistance);
                const float cForwardTerrainHeight = SCR_TerrainHelper.GetTerrainY(forwardVector, m_world, true);
                const float cHelicopterHeight = SCR_TerrainHelper.GetTerrainY(m_owner.GetOrigin(), m_world, true) + cAlt;

                if (!m_isLandingPhase && cHelicopterHeight < (cForwardTerrainHeight + m_MinHeightAboveTerrain))
                {
                    const float diff = (cForwardTerrainHeight + m_MinHeightAboveTerrain) - cHelicopterHeight;
                    if (diff > 45)
                        m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower + 15);
                    else if (diff > 35)
                        m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower + 5);
                    else if (diff > 10)
                        m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower + 1.5);
                    else
                        m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower + 0.5);
                }
                else
                {
                    const bool altCheckWP = float.AlmostEqual(cHelicopterHeight, m_currentWPheight, 2);
                    const bool altCheckTerrain = float.AlmostEqual(cAlt, m_MinHeightAboveTerrain, 2);

                    if (!altCheckWP || !altCheckTerrain)
                    {
                        if (cHelicopterHeight < m_currentWPheight || cAlt < m_MinHeightAboveTerrain)
                        {
                            const float diff = (m_currentWPheight / cHelicopterHeight) + (m_MinHeightAboveTerrain / cAlt);
                            if (diff > 2.5)
                                m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower + 0.5);
                            else
                                m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower + 0.25);
                        }
                        else
                        {
                            float diff = (cHelicopterHeight / m_currentWPheight) + (cAlt / m_MinHeightAboveTerrain);
                            if (diff > 2.5)
                                m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower - 0.5);
                            else
                                m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower - 0.25);
                        }
                    }
                }
            }
            else
            {
                if (m_isLandingPhase)
                {
                    if (cAlt > 6)
                        m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower - 0.6);
                    else
                        m_simulation.RotorSetForceScaleState(0, m_defaultMainRotorPower - 0.5);
                }
            }

            // Handle steering
            if (m_currentAttackTimer == 0 || m_currentAttackTimer > m_AttackPlayersTimeout)
            {
                if (cAlt > 4)
                {
                    vector matOwner[4];
                    m_owner.GetTransform(matOwner);
                    const vector angleToWP = SCR_Global.GetDirectionAngles(matOwner, m_currentWPTransform[3]);
                    BH_ComputeRotation(angleToWP[0], cSpeed);
                }
            }

            // Handle speed
            if (!m_isLandingPhase)
            {
                if (cSpeed < m_MaxSpeed && cAlt > 10) m_simSpeedState = 1.8;
                if (cSpeed > m_MaxSpeed && cAlt > 10)
                {
                    if (cSpeed > (m_MaxSpeed + 10))
                        m_simSpeedState = -0.1;
                    else
                        m_simSpeedState = 0;
                }
            }

            if (m_isLandingPhase && distanceToWP < 400)
            {
                if (distanceToWP > 50)
                {
                    if (cSpeed > 20)
                        m_simSpeedState = 0 - ((cSpeed / distanceToWP) * 3);
                    else
                        m_simSpeedState = 0.5;
                }
                else
                {
                    if (cSpeed > 10)
                        m_simSpeedState = -0.2;
                    else
                        m_simSpeedState = 0.2;
                }
            }

            // Handle attack
            if (m_AttackPlayers && !m_isLandingPhase)
            {
                if (m_currentAttackTimer < m_AttackPlayersTimeout)
                {
                    IEntity closePlayer = BH_GetClosestPlayerEntity(m_owner);
                    float closePlayerDistance = 0;
                    if (closePlayer) closePlayerDistance = vector.Distance(m_owner.GetOrigin(), closePlayer.GetOrigin());

                    if (closePlayer && closePlayerDistance > (m_MinHeightAboveTerrain + 200) && (closePlayerDistance < 400 || m_currentAttackTimer > 0))
                    {
                        if (!m_OnAttackActionsCalled && m_SlotBase)
                        {
                            m_SlotBase.BH_ActionOnAttackStart();
                            m_OnAttackActionsCalled = true;
                        }

                        Vehicle playerVehicle = Vehicle.Cast(closePlayer.GetRootParent());
                        if (playerVehicle)
                        {
                            Physics vehPhysics = playerVehicle.GetPhysics();
                            int vehSpeed = 0;
                            if (vehPhysics) vehSpeed = vehPhysics.GetVelocity().Length();

                            float speedDiff = cSpeed - vehSpeed;

                            if (speedDiff > 50)
                            {
                                if (cSpeed > 15) m_simSpeedState = -1;
                                else m_simSpeedState = 0.8;
                            }
                            else if (speedDiff > 30)
                            {
                                if (cSpeed > 15) m_simSpeedState = -0.7;
                                else m_simSpeedState = 0.4;
                            }
                            else
                            {
                                if (speedDiff < -30) m_simSpeedState = 1;
                                else m_simSpeedState = 0.5;
                            }
                        }
                        else
                        {
                            if (cSpeed > 30) m_simSpeedState = -0.5;
                            else m_simSpeedState = 0.5;
                        }

                        m_currentAttackTimer++;

                        vector matOwner[4];
                        m_owner.GetTransform(matOwner);
                        const vector angleToPlayer = SCR_Global.GetDirectionAngles(matOwner, closePlayer.GetOrigin());
                        const float rotationToPlayer = angleToPlayer[0];

                        if (rotationToPlayer > -15 && rotationToPlayer < 15)
                        {
                            m_simulation.RotorSetForceScaleState(1, m_defaultRotationPower);
                        }
                        else
                        {
                            if (rotationToPlayer > 0)
                            {
                                if (rotationToPlayer > 70)
                                {
                                    if (cSpeed > 30) m_simulation.RotorSetForceScaleState(1, m_defaultRotationPower - 1.7);
                                    else m_simulation.RotorSetForceScaleState(1, m_defaultRotationPower - 0.8);
                                }
                                else m_simulation.RotorSetForceScaleState(1, m_defaultRotationPower - 0.5);
                            }
                            else
                            {
                                if (rotationToPlayer < -70)
                                {
                                    if (cSpeed > 30) m_simulation.RotorSetForceScaleState(1, m_defaultRotationPower + 1.7);
                                    else m_simulation.RotorSetForceScaleState(1, m_defaultRotationPower + 0.8);
                                }
                                else m_simulation.RotorSetForceScaleState(1, m_defaultRotationPower + 0.5);
                            }
                        }

                        if (closePlayer != m_GunnerLastTarget)
                        {
                            BH_AddGunnerTarget(closePlayer);
                            m_GunnerLastTarget = closePlayer;
                        }
                    }
                    else
                    {
                        m_currentAttackTimer = 0;
                    }
                }
                else
                {
                    m_currentAttackTimer++;
                    if (m_currentAttackTimer > (m_AttackPlayersTimeout + 30)) m_currentAttackTimer = 0;
                }
            }

            // Apply velocity
            if (m_simSpeedState != 0)
            {
                const vector velOrig = ownerPhysics.GetVelocity();
                const vector rotVector = m_owner.GetAngles();
                const vector forward = m_owner.GetWorldTransformAxis(2);
                const float dot = velOrig.Normalized() * forward.Normalized();
                float velX = 0;
                float velY = 0;

                if (dot > 0 && cAlt > 4)
                {
                    velX = velOrig[0] + Math.Sin(rotVector[1] * Math.DEG2RAD) * m_simSpeedState;
                    velY = velOrig[2] + Math.Cos(rotVector[1] * Math.DEG2RAD) * m_simSpeedState;
                }
                else
                {
                    if (cSpeed > 2 && cAlt < 4)
                    {
                        velX = Math.Sin(rotVector[1] * Math.DEG2RAD) * 0.05;
                        velY = Math.Cos(rotVector[1] * Math.DEG2RAD) * 0.05;
                    }
                    else if (cSpeed > 5 && cAlt > 5)
                    {
                        velX = Math.Sin(rotVector[1] * Math.DEG2RAD) * 3;
                        velY = Math.Cos(rotVector[1] * Math.DEG2RAD) * 3;
                    }
                }

                if (velX != 0 && velY != 0)
                {
                    vector vel;
                    vel[0] = velX;
                    vel[1] = velOrig[1];
                    vel[2] = velY;
                    ownerPhysics.SetVelocity(vel);
                }
            }

            // Landing on ground
            if (m_isLandingPhase && cAlt < 2)
            {
                m_isLandingPhase = false;
                m_simulation.SetThrottle(0);
                m_simulation.RotorSetForceScaleState(0, -0.2);
                m_simulation.RotorSetForceScaleState(1, m_startRotationPower);

                ownerPhysics.SetVelocity(vector.Zero);
                ownerPhysics.SetAngularVelocity(vector.Zero);

                if (m_currentWPdata.m_ShutdownEngine)
                    BH_StopEngine();
                else
                {
                    m_simShouldNailOnGround = true;
                    BH_NailOnGround();
                }

                BH_ToggleLandingLights(false);
                if (m_SlotBase) m_SlotBase.BH_ActionOnWaypoint(m_currentWPIndex);

                BH_Log("ATERRIZAJE en WP index=" + m_currentWPIndex + " waitGetIn=" + m_currentWPdata.m_WaitForPlayersGetIn + " waitGetOut=" + m_currentWPdata.m_WaitForPlayersGetOut + " waitSec=" + m_currentWPdata.m_WaitSeconds);

                // Si este WP espera embarque de pasajeros, desbloquear e iniciar boarding
                if (m_currentWPdata.m_WaitForPlayersGetIn)
                {
                    BH_UnlockHelicopter();
                    BH_ResetBoardingState();
                    BH_Log("ATERRIZAJE: WP espera embarque, desbloqueando heli e iniciando BoardingCheck");
                    GetGame().GetCallqueue().Remove(BH_BoardingCheck);
                    GetGame().GetCallqueue().CallLater(BH_BoardingCheck, 2000, false);
                    GetGame().GetCallqueue().Remove(BH_Simulate);
                    return;
                }

                if (m_currentWPdata.m_WaitForPlayersGetOut || m_currentWPdata.m_WaitSeconds > 0)
                {
                    float waitSec = m_currentWPdata.m_WaitSeconds * 1000;
                    if (waitSec == 0) waitSec = 3000;
                    
                    // Si esperamos desembarque, desbloquear para que puedan salir
                    if (m_currentWPdata.m_WaitForPlayersGetOut)
                        BH_UnlockHelicopter();
                    
                    BH_Log("ATERRIZAJE: esperando " + waitSec + "ms antes de BH_RestartFromWaypoint");
                    GetGame().GetCallqueue().Remove(BH_RestartFromWaypoint);
                    GetGame().GetCallqueue().CallLater(BH_RestartFromWaypoint, waitSec, false);
                }

                GetGame().GetCallqueue().Remove(BH_Simulate);
                return;
            }
        }

        if (m_simIsDestroyed)
        {
            m_simulation.SetThrottle(0);
            m_simulation.RotorSetForceScaleState(0, 0.5);
            m_simulation.RotorSetForceScaleState(1, 3);
        }
        else
        {
            GetGame().GetCallqueue().CallLater(BH_Simulate, m_RefreshRate * 1000, false);
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_NailOnGround()
    {
        const Physics ownerPhysics = m_owner.GetPhysics();
        if (!ownerPhysics) return;

        if (!m_simIsDestroyed && (m_simulation.RotorGetRPM(0) < m_simulation.RotorGetRPMTarget(0) / 1.5 || m_simShouldNailOnGround))
        {
            ownerPhysics.SetVelocity(vector.Zero);
            ownerPhysics.SetAngularVelocity(vector.Zero);
            ownerPhysics.ChangeSimulationState(SimulationState.COLLISION);
            GetGame().GetCallqueue().Remove(BH_NailOnGround);
            GetGame().GetCallqueue().CallLater(BH_NailOnGround, 1000, false);
        }
        else
        {
            BH_Log("BH_NailOnGround: rotores con RPM suficiente, liberando helicoptero");
            ownerPhysics.ChangeSimulationState(SimulationState.SIMULATION);
            ownerPhysics.SetVelocity(vector.Up);
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_StartEngine(bool ForceStart = false)
    {
        BH_Log("BH_StartEngine: arrancando motor, ForceStart=" + ForceStart);
        m_simShouldNailOnGround = false;

        if (m_TurnOnLights) GetGame().GetCallqueue().CallLater(BH_ToggleLights, 500, false, true);
        if (m_TurnOnSearchLight) GetGame().GetCallqueue().CallLater(BH_ToggleSearchLights, 600, false, true);
        if (m_TurnOnHazzardLight) GetGame().GetCallqueue().CallLater(BH_ToggleHazzardLights, 700, false, true);

        m_simSpeedState = 1;
        m_simSteeringState = m_defaultRotationPower;

        // Si el motor estaba apagado, necesitamos rearrancarlo
        if (!m_simulation.EngineIsOn())
        {
            BH_Log("BH_StartEngine: motor estaba apagado, rearrancando");
            m_simulation.EngineStart();
        }

        m_simulation.SetThrottle(1);
        m_simulation.RotorSetForceScaleState(0, 1.8);

        if (m_tailRotorClockwise)
            m_simulation.RotorSetForceScaleState(1, Math.AbsFloat(m_startRotationPower));
        else
            m_simulation.RotorSetForceScaleState(1, m_startRotationPower);

        m_sSpeed = m_signalsMgr.FindSignal("Speed");
        m_sRollAngle = m_signalsMgr.FindSignal("RollAngle");
        m_sYawAngle = m_signalsMgr.FindSignal("YawAngle");
        m_sPitchAngle = m_signalsMgr.FindSignal("PitchAngle");

        // ForceStart: usado cuando el helicoptero ya estaba en vuelo (ej: OnStartFromSerializer)
        // En ese caso no necesitamos NailOnGround, solo asegurar que el motor arranca
        if (ForceStart)
        {
            m_simulation.EngineStart();
            BH_Log("BH_StartEngine: ForceStart - motor forzado para re-arranque en vuelo");
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_StopEngine()
    {
        BH_Log("BH_StopEngine: apagando motor");
        m_simSpeedState = 0;
        m_simSteeringState = 0;
        m_simulation.EngineStop();
        m_simulation.SetThrottle(0);
        m_simulation.RotorSetForceScaleState(0, 0);
        m_simulation.RotorSetForceScaleState(1, 0.2);

        Physics ownerPhysics = m_owner.GetPhysics();
        if (ownerPhysics)
        {
            ownerPhysics.SetVelocity(vector.Zero);
            ownerPhysics.SetAngularVelocity(vector.Zero);
        }
    }

    
	// ----------------------------------------------------------------

	protected void BH_PlayHelicopterSound(string soundEvent)
	{
	    if (!m_owner) return;

	    // En local/singleplayer: reproducir directamente
	    if (!Replication.IsRunning())
	    {
	        BH_PlaySoundLocal(soundEvent);
	        return;
	    }

	    // En dedicado: solo el server envia el RPC a todos los clientes
	    if (!Replication.IsServer()) return;

	    Rpc(BH_RpcPlaySound, soundEvent);
	}

	protected void BH_PlaySoundLocal(string soundEvent)
	{
	    if (!m_owner) return;
	    // Buscar SCR_HelicopterSoundComponent (hereda de SoundComponent, es donde estan los sound events)
	    SCR_HelicopterSoundComponent heliSoundComp = SCR_HelicopterSoundComponent.Cast(m_owner.FindComponent(SCR_HelicopterSoundComponent));
	    if (heliSoundComp)
	    {
	        heliSoundComp.SoundEvent(soundEvent);
	        Print("[BH_Heli] BH_PlaySoundLocal: " + soundEvent + " via SCR_HelicopterSoundComponent");
	        return;
	    }
	    // Fallback: SoundComponent generico (por si se añade uno separado al prefab)
	    SoundComponent soundComp = SoundComponent.Cast(m_owner.FindComponent(SoundComponent));
	    if (soundComp)
	    {
	        soundComp.SoundEvent(soundEvent);
	        Print("[BH_Heli] BH_PlaySoundLocal: " + soundEvent + " via SoundComponent");
	        return;
	    }
	    Print("[BH_Heli] BH_PlaySoundLocal: NO sound component found!");
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void BH_RpcPlaySound(string soundEvent)
	{
	    // No reproducir en el servidor dedicado (no tiene salida de audio)
	    if (System.IsConsoleApp()) return;
	    BH_PlaySoundLocal(soundEvent);
	}

	// ----------------------------------------------------------------
	
	protected void BH_ShowRadioMessage(string message)
	{
	    if (!message || message.IsEmpty()) return;
	
	    // En local llamamos directamente sin RPC
	    if (!Replication.IsRunning())
	    {
	        BH_DisplayNotification(message);
	        return;
	    }
	
	    if (!Replication.IsServer()) return;
	
	    PlayerManager pMgr = GetGame().GetPlayerManager();
	    if (!pMgr) return;
	
	    array<int> playerIDs = new array<int>();
	    pMgr.GetPlayers(playerIDs);
	
	    foreach (int playerID : playerIDs)
	    {
	        if (!m_sRadioFactionKey.IsEmpty())
	        {
	            SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
	            if (!factionMgr) continue;
	            Faction playerFaction = factionMgr.GetPlayerFaction(playerID);
	            if (!playerFaction) continue;
	            if (playerFaction.GetFactionKey() != m_sRadioFactionKey) continue;
	        }
	
	        IEntity playerEntity = pMgr.GetPlayerControlledEntity(playerID);
	        if (!playerEntity) continue;
	
	        RplComponent rpl = RplComponent.Cast(playerEntity.FindComponent(RplComponent));
	        if (!rpl) continue;
	
	        Rpc(BH_RpcShowRadioMessage, Replication.FindId(playerEntity), message);
	    }
	}
	
	protected void BH_DisplayNotification(string message)
	{
	    SCR_PopUpNotification popUp = SCR_PopUpNotification.GetInstance();
	    if (!popUp) return;
	
	    popUp.PopupMsg(
	        message,
	        5,
	        "",
	        -1,
	        "", "", "", "",
	        "", "", "", "",
	        "",
	        SCR_EPopupMsgFilter.ALL
	    );
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void BH_RpcShowRadioMessage(RplId targetId, string message)
	{
	    IEntity localPlayer = SCR_PlayerController.GetLocalControlledEntity();
	    if (!localPlayer) return;
	
	    if (Replication.FindId(localPlayer) != targetId) return;
	
	    BH_DisplayNotification(message);
	}
	
	// ----------------------------------------------------------------
	
	protected void BH_BoardingCheck()
	{
	    if (!m_owner || m_simIsDestroyed) return;
	    if (!m_currentWPdata) return;
	    if (!m_currentWPdata.m_WaitForPlayersGetIn) return;

	    // Contar jugadores a bordo
	    int playersOnBoard = 0;
	    PlayerManager pMgr = GetGame().GetPlayerManager();
	    if (pMgr)
	    {
	        array<int> pIDs = new array<int>();
	        pMgr.GetPlayers(pIDs);

	        foreach (int pID : pIDs)
	        {
	            IEntity pChar = pMgr.GetPlayerControlledEntity(pID);
	            if (!pChar) continue;
	            if (pChar.GetRootParent() == m_owner)
	                playersOnBoard++;
	        }
	    }

	    // Contar IAs (no-jugador) en compartimentos de pasajero/cargo
	    int aiOnBoard = BH_CountAIPassengers();
	    int totalPassengers = playersOnBoard + aiOnBoard;
	    m_PassengersOnBoard = totalPassengers;

	    BH_Log("BoardingCheck: jugadores=" + playersOnBoard + " IAs=" + aiOnBoard + " total=" + totalPassengers + " min=" + m_currentWPdata.m_FixedPlayerCount + " started=" + m_BoardingStarted + " minReached=" + m_MinReached);

	    // El primer pasajero sube, arranca el contador
	    if (totalPassengers > 0 && !m_BoardingStarted)
	    {
	        m_BoardingStarted = true;

	        BH_ShowRadioMessage(m_currentWPdata.m_RadioMsgBoarding);
	        BH_PlayHelicopterSound("BH_Heli_Boarding");

	        BH_Log("BoardingCheck: primer pasajero detectado, iniciando secuencia de embarque");

	        // Programamos el primer mensaje periodico
	        GetGame().GetCallqueue().CallLater(BH_BoardingPeriodicMessage, m_currentWPdata.m_RadioMessageInterval * 1000, false);
	        // Programamos el timeout
	        GetGame().GetCallqueue().CallLater(BH_BoardingTimeout, m_currentWPdata.m_WaitTimeout * 1000, false);
	    }

	    if (!m_BoardingStarted)
	    {
	        GetGame().GetCallqueue().CallLater(BH_BoardingCheck, 2000, false);
	        return;
	    }

	    // Comprobamos si se ha alcanzado el minimo (jugadores + IAs)
	    if (!m_MinReached && m_currentWPdata.m_UseFixedPlayerCount && totalPassengers >= m_currentWPdata.m_FixedPlayerCount)
	    {
	        m_MinReached = true;

	        // Cancelamos timeout y mensajes periodicos
	        GetGame().GetCallqueue().Remove(BH_BoardingTimeout);
	        GetGame().GetCallqueue().Remove(BH_BoardingPeriodicMessage);

	        BH_ShowRadioMessage(m_currentWPdata.m_RadioMsgMinReached);
	        BH_PlayHelicopterSound("BH_Heli_MinReached");

	        BH_Log("BoardingCheck: minimo alcanzado (" + totalPassengers + "/" + m_currentWPdata.m_FixedPlayerCount + "), despegando en " + m_currentWPdata.m_WaitExtraTime + "s");

	        // Tiempo extra antes de despegar
	        GetGame().GetCallqueue().CallLater(BH_BoardingDone, m_currentWPdata.m_WaitExtraTime * 1000, false);
	        return;
	    }

	    GetGame().GetCallqueue().CallLater(BH_BoardingCheck, 2000, false);
	}
	
	// ----------------------------------------------------------------
	
	protected void BH_BoardingPeriodicMessage()
	{
	    if (!m_BoardingStarted || m_MinReached || m_simIsDestroyed) return;
	    if (!m_currentWPdata) return;

	    string periodicMsg = m_currentWPdata.m_RadioMsgPeriodic + " - " + m_PassengersOnBoard + "/" + m_currentWPdata.m_FixedPlayerCount;
	    BH_ShowRadioMessage(periodicMsg);
	    BH_PlayHelicopterSound("BH_Heli_Periodic");

	    // Siguiente mensaje periodico
	    GetGame().GetCallqueue().CallLater(BH_BoardingPeriodicMessage, m_currentWPdata.m_RadioMessageInterval * 1000, false);
	}

	// ----------------------------------------------------------------
	
	protected void BH_BoardingTimeout()
	{
	    if (m_MinReached || m_simIsDestroyed) return;

	    GetGame().GetCallqueue().Remove(BH_BoardingPeriodicMessage);

	    BH_Log("BoardingTimeout: tiempo agotado, pasajeros a bordo=" + m_PassengersOnBoard + ", forzando despegue en 3s");

	    BH_ShowRadioMessage(m_currentWPdata.m_RadioMsgTimeout);
	    BH_PlayHelicopterSound("BH_Heli_Timeout");

	    // Pequeña pausa antes de despegar para que se escuche el mensaje
	    GetGame().GetCallqueue().CallLater(BH_BoardingDone, 3000, false);
	}
	
	// ----------------------------------------------------------------
	
	protected void BH_BoardingExtraTime()
	{
	    BH_BoardingDone();
	}
	
	// ----------------------------------------------------------------
	
	protected void BH_BoardingDone()
	{
	    BH_ResetBoardingState();

	    if (!m_owner || m_simIsDestroyed)
	    {
	        BH_Log("BoardingDone: owner null o destruido, abortando");
	        return;
	    }

	    // Re-bloquear helicóptero si estaba configurado como bloqueado
	    BH_RelockHelicopter();

	    // Avanzamos al siguiente waypoint y despegamos
	    if (m_Waypoins.Count() - 1 > m_currentWPIndex)
	        m_currentWPIndex++;
	    else
	        m_currentWPIndex = 0;

	    BH_Log("BoardingDone: avanzando a WP index=" + m_currentWPIndex + ", arrancando motor y despegando");

	    BH_SetWaypoint(m_currentWPIndex);
	    BH_StartEngine();
	    GetGame().GetCallqueue().Remove(BH_Simulate);
	    GetGame().GetCallqueue().CallLater(BH_Simulate, 3000, false);
	}
	
	//------------------------------------------------------------------------------------------------

    protected void BH_RestartFromWaypoint()
	{
	    if (!m_owner || m_simIsDestroyed)
	    {
	        BH_Log("RestartFromWaypoint: owner null o destruido, abortando");
	        return;
	    }

	    Physics ownerPhysics = m_owner.GetPhysics();
	    if (ownerPhysics)
	    {
	        ownerPhysics.SetVelocity(vector.Zero);
	        ownerPhysics.SetAngularVelocity(vector.Zero);
	    }

	    // Null safety: si no tenemos datos del WP, directamente despegamos
	    if (!m_currentWPdata)
	    {
	        BH_Log("RestartFromWaypoint: AVISO - m_currentWPdata es null, despegando directamente");
	        BH_BoardingDone();
	        return;
	    }

	    if (m_currentWPdata.m_WaitForPlayersGetIn)
	    {
	        BH_ResetBoardingState();
	        BH_UnlockHelicopter();

	        BH_Log("RestartFromWaypoint: WP espera embarque, iniciando BoardingCheck");
	        GetGame().GetCallqueue().Remove(BH_BoardingCheck);
	        GetGame().GetCallqueue().CallLater(BH_BoardingCheck, 2000, false);
	        return;
	    }

	    if (m_currentWPdata.m_WaitForPlayersGetOut)
	    {
	        float waitSec = m_currentWPdata.m_WaitSeconds * 1000;
	        if (waitSec < 3000) waitSec = 3000;

	        // Contar pasajeros (jugadores + IAs)
	        int playersOnBoard = 0;
	        PlayerManager pMgr = GetGame().GetPlayerManager();
	        if (pMgr)
	        {
	            array<int> pIDs = new array<int>();
	            pMgr.GetPlayers(pIDs);

	            foreach (int pID : pIDs)
	            {
	                IEntity pChar = pMgr.GetPlayerControlledEntity(pID);
	                if (!pChar) continue;
	                if (pChar.GetRootParent() == m_owner)
	                    playersOnBoard++;
	            }
	        }

	        int aiOnBoard = BH_CountAIPassengers();
	        int totalOnBoard = playersOnBoard + aiOnBoard;

	        BH_Log("RestartFromWaypoint: esperando desembarque, jugadores=" + playersOnBoard + " IAs=" + aiOnBoard + " total=" + totalOnBoard);

	        if (totalOnBoard == 0)
	        {
	            BH_Log("RestartFromWaypoint: todos desembarcados, despegando");
	            BH_BoardingDone();
	        }
	        else
	            GetGame().GetCallqueue().CallLater(BH_RestartFromWaypoint, waitSec, false);

	        return;
	    }

	    BH_Log("RestartFromWaypoint: sin condiciones especiales, despegando");
	    BH_BoardingDone();
	}

    //------------------------------------------------------------------------------------------------


    //------------------------------------------------------------------------------------------------

    protected IEntity BH_GetClosestPlayerEntity(IEntity origin)
    {
        PlayerManager pMgr = GetGame().GetPlayerManager();
        if (!pMgr) return null;

        array<int> pIDs = new array<int>();
        pMgr.GetPlayers(pIDs);

        IEntity closest = null;
        float closestDist = float.MAX;

        foreach (int pID : pIDs)
        {
            IEntity pChar = pMgr.GetPlayerControlledEntity(pID);
            if (!pChar) continue;

            SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(pChar);
            if (!ch || ch.GetDamageManager().GetHealth() <= 0) continue;

            IEntity root = pChar.GetRootParent();
            if (root && root != pChar)
            {
                Vehicle veh = Vehicle.Cast(root);
                if (veh)
                {
                    float vehAlt = SCR_TerrainHelper.GetHeightAboveTerrain(veh.GetOrigin(), m_world, true);
                    if (vehAlt > 10) continue;
                }
            }

            float dist = vector.Distance(origin.GetOrigin(), pChar.GetOrigin());
            if (dist < closestDist)
            {
                closestDist = dist;
                closest = pChar;
            }
        }

        return closest;
    }

    //------------------------------------------------------------------------------------------------

    void BH_ToggleLights(bool state)
    {
        if (m_lightMgr)
        {
            m_lightMgr.SetLightsState(ELightType.Rear, state);
            m_lightMgr.SetLightsState(ELightType.Reverse, state);
        }
    }

    void BH_ToggleSearchLights(bool state)
    {
        if (m_lightMgr) m_lightMgr.SetLightsState(ELightType.SearchLight, state);
    }

    void BH_ToggleHazzardLights(bool state)
    {
        if (m_lightMgr) m_lightMgr.SetLightsState(ELightType.Hazard, state);
    }

    void BH_ToggleLandingLights(bool state)
    {
        if (m_lightMgr) m_lightMgr.SetLightsState(ELightType.HiBeam, state);
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_SpawnPilot(ResourceName spawnRes, ECompartmentType compartmentType)
    {
        Resource pilotRes = Resource.Load(spawnRes);
        if (!pilotRes.IsValid())
        {
            BH_Log("BH_SpawnPilot FALLO: recurso no valido '" + spawnRes + "'");
            return;
        }

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = m_owner.GetOrigin();

        IEntity SpawnedEntity = GetGame().SpawnEntityPrefab(pilotRes, GetGame().GetWorld(), spawnParams);
        if (!SpawnedEntity)
        {
            BH_Log("BH_SpawnPilot FALLO: no se pudo spawnear entidad para '" + spawnRes + "'");
            return;
        }

        if (!m_PilotEnt)
        {
            m_PilotEnt = SpawnedEntity;

            if (!m_CrewIsinvulnerable)
            {
                SCR_CharacterControllerComponent pilotController = SCR_CharacterControllerComponent.Cast(m_PilotEnt.FindComponent(SCR_CharacterControllerComponent));
                if (pilotController) pilotController.m_OnLifeStateChanged.Insert(OnLifeStateChanged_pilot);
            }
        }
        else
        {
            m_CoPilotEnt = SpawnedEntity;
        }

        float pilotReduction = m_CrewDamageReduction;
        if (m_CrewIsinvulnerable) pilotReduction = 1;
        if (m_CrewIsinvulnerable || m_CrewDamageReduction > 0)
            BH_ApplyDamageReductionToEntity(SpawnedEntity, pilotReduction);

        AIControlComponent Aicontrol = AIControlComponent.Cast(SpawnedEntity.FindComponent(AIControlComponent));
        if (Aicontrol)
        {
            SCR_ChimeraAIAgent ownerAgent = SCR_ChimeraAIAgent.Cast(Aicontrol.GetAIAgent());
            if (ownerAgent)
            {
                ownerAgent.PreventMaxLOD();
                if (m_crewGroup) m_crewGroup.AddAgent(ownerAgent);
            }
            Aicontrol.DeactivateAI();
        }

        SCR_CompartmentAccessComponent compartComp = SCR_CompartmentAccessComponent.Cast(SpawnedEntity.FindComponent(SCR_CompartmentAccessComponent));
        if (compartComp)
        {
            BaseCompartmentSlot freeSlot = compartComp.FindFreeCompartment(m_owner, compartmentType, false);
            if (freeSlot)
            {
                if (!compartComp.GetInVehicle(freeSlot.GetOwner(), freeSlot, true, -1, ECloseDoorAfterActions.INVALID, true))
                {
                    BH_Log("BH_SpawnPilot FALLO: GetInVehicle devolvio false, eliminando entidad");
                    SCR_EntityHelper.DeleteEntityAndChildren(SpawnedEntity);
                }
                else
                {
                    freeSlot.SetReserved(SpawnedEntity);
                    freeSlot.SetCompartmentAccessible(false);
                    string crewRole = "copiloto";
                    if (SpawnedEntity == m_PilotEnt) crewRole = "piloto";
                    BH_Log("BH_SpawnPilot OK: " + crewRole + " sentado en compartimento");
                }
            }
            else
            {
                BH_Log("BH_SpawnPilot FALLO: no hay compartimento libre tipo " + compartmentType + ", eliminando entidad");
                SCR_EntityHelper.DeleteEntityAndChildren(SpawnedEntity);
            }
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_SpawnGunner(ResourceName spawnRes)
    {
        if (!m_crewGroup) m_crewGroup = BH_CreateHelicopterGroup();

        Resource gunnerRes = Resource.Load(spawnRes);
        if (!gunnerRes.IsValid()) return;

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = m_owner.GetOrigin();

        IEntity SpawnedEntity = GetGame().SpawnEntityPrefab(gunnerRes, GetGame().GetWorld(), spawnParams);
        if (!SpawnedEntity) return;

        m_aGunners.Insert(SpawnedEntity);

        AIControlComponent Aicontrol = AIControlComponent.Cast(SpawnedEntity.FindComponent(AIControlComponent));
        if (Aicontrol)
        {
            SCR_ChimeraAIAgent ownerAgent = SCR_ChimeraAIAgent.Cast(Aicontrol.GetAIAgent());
            if (ownerAgent)
            {
                ownerAgent.PreventMaxLOD();
                if (m_crewGroup) m_crewGroup.AddAgent(ownerAgent);
            }

            SCR_AICombatComponent AiCombat = SCR_AICombatComponent.Cast(SpawnedEntity.FindComponent(SCR_AICombatComponent));
            if (AiCombat)
            {
                AiCombat.SetAISkill(EAISkill.CYLON);
                AiCombat.SetPerceptionFactor(5.0);
            }

            GetGame().GetCallqueue().CallLater(BH_ActivateAi, 2000, false, Aicontrol);
        }

        float gunnerReduction = m_CrewDamageReduction;
        if (m_CrewIsinvulnerable) gunnerReduction = 1;
        if (m_CrewIsinvulnerable || m_CrewDamageReduction > 0)
            BH_ApplyDamageReductionToEntity(SpawnedEntity, gunnerReduction);

        SCR_CompartmentAccessComponent compartComp = SCR_CompartmentAccessComponent.Cast(SpawnedEntity.FindComponent(SCR_CompartmentAccessComponent));
        if (compartComp)
        {
            BaseCompartmentSlot freeSlot = compartComp.FindFreeCompartment(m_owner, ECompartmentType.TURRET, false);
            if (freeSlot)
            {
                if (!compartComp.GetInVehicle(freeSlot.GetOwner(), freeSlot, true, -1, ECloseDoorAfterActions.INVALID, true))
                    SCR_EntityHelper.DeleteEntityAndChildren(SpawnedEntity);
            }
            else SCR_EntityHelper.DeleteEntityAndChildren(SpawnedEntity);
        }
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_ActivateAi(AIControlComponent Aicontrol)
    {
        if (Aicontrol) Aicontrol.ActivateAI();
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_AddGunnerTarget(IEntity target)
    {
        if (!target || m_aGunners.Count() == 0) return;

        BaseTarget newTarget;
        bool isNewTarget;

        foreach (IEntity gunner : m_aGunners)
        {
            if (!gunner) continue;

            PerceptionComponent perception = PerceptionComponent.Cast(gunner.FindComponent(PerceptionComponent));
            if (perception)
            {
                newTarget = perception.GetTargetPerceptionObject(target, ETargetCategory.ENEMY);
                if (newTarget && m_aiGroupUtil) m_aiGroupUtil.m_Perception.AddOrUpdateTarget(newTarget, isNewTarget);
            }
        }
    }

    //------------------------------------------------------------------------------------------------

    protected SCR_AIGroup BH_CreateHelicopterGroup()
    {
        if (m_crewGroup) return m_crewGroup;

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = m_owner.GetOrigin();

        Resource groupResource = Resource.Load("{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et");
        if (!groupResource.IsValid()) return null;

        SCR_AIGroup newAIGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(groupResource, GetGame().GetWorld(), spawnParams));
        if (!newAIGroup) return null;

        newAIGroup.PreventMaxLOD();

        SCR_AIVehicleUsageComponent vehUseComp = SCR_AIVehicleUsageComponent.Cast(m_owner.FindComponent(SCR_AIVehicleUsageComponent));
        SCR_AIGroupUtilityComponent grpUtilComp = newAIGroup.GetGroupUtilityComponent();
        if (grpUtilComp && vehUseComp) grpUtilComp.AddUsableVehicle(vehUseComp);
        if (grpUtilComp) m_aiGroupUtil = grpUtilComp;

        return newAIGroup;
    }

    //------------------------------------------------------------------------------------------------

    void OnLifeStateChanged_pilot(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
    {
        if (newLifeState == ECharacterLifeState.DEAD || newLifeState == ECharacterLifeState.INCAPACITATED)
        {
            m_simIsDestroyed = true;
            GetGame().GetCallqueue().Remove(BH_Simulate);
            m_simulation.RotorSetForceScaleState(0, 0);
            m_simulation.RotorSetForceScaleState(1, 1.5);

            if (m_PilotEnt)
            {
                SCR_CharacterControllerComponent pilotController = SCR_CharacterControllerComponent.Cast(m_PilotEnt.FindComponent(SCR_CharacterControllerComponent));
                if (pilotController) pilotController.m_OnLifeStateChanged.Remove(OnLifeStateChanged_pilot);
            }
        }
    }

    //------------------------------------------------------------------------------------------------
    // Metodos helper internos
    //------------------------------------------------------------------------------------------------

    // Cuenta IAs (no-jugadores) sentadas en compartimentos de pasajero/cargo del helicoptero
    protected int BH_CountAIPassengers()
    {
        if (!m_owner) return 0;

        BaseCompartmentManagerComponent compartMgr = BaseCompartmentManagerComponent.Cast(m_owner.FindComponent(BaseCompartmentManagerComponent));
        if (!compartMgr) return 0;

        array<BaseCompartmentSlot> aSlots = new array<BaseCompartmentSlot>();
        compartMgr.GetCompartments(aSlots);

        // Recoger IDs de jugadores para excluirlos
        PlayerManager pMgr = GetGame().GetPlayerManager();
        array<int> playerIDs = new array<int>();
        if (pMgr) pMgr.GetPlayers(playerIDs);

        int aiCount = 0;
        foreach (BaseCompartmentSlot slot : aSlots)
        {
            // Solo contar compartimentos de cargo/pasajero, no piloto/copiloto/torreta
            CargoCompartmentSlot cargoSlot = CargoCompartmentSlot.Cast(slot);
            if (!cargoSlot) continue;

            IEntity occupant = slot.GetOccupant();
            if (!occupant) continue;

            // Comprobar que no es un jugador
            bool isPlayer = false;
            if (pMgr)
            {
                foreach (int pID : playerIDs)
                {
                    IEntity pChar = pMgr.GetPlayerControlledEntity(pID);
                    if (pChar && pChar == occupant)
                    {
                        isPlayer = true;
                        break;
                    }
                }
            }

            if (!isPlayer) aiCount++;
        }

        return aiCount;
    }

    //------------------------------------------------------------------------------------------------

    // Desbloquea el helicoptero para que los jugadores puedan subir
    protected void BH_UnlockHelicopter()
    {
        if (!m_owner) return;

        SCR_BaseLockComponent lockComp = SCR_BaseLockComponent.Cast(m_owner.FindComponent(SCR_BaseLockComponent));
        if (lockComp)
        {
            lockComp.SetLocked(false);
            BH_Log("BH_UnlockHelicopter: helicoptero desbloqueado");
        }
    }

    //------------------------------------------------------------------------------------------------

    // Re-bloquea el helicoptero si originalmente estaba configurado como bloqueado
    protected void BH_RelockHelicopter()
    {
        if (!m_owner || !m_LockForPlayers) return;

        SCR_BaseLockComponent lockComp = SCR_BaseLockComponent.Cast(m_owner.FindComponent(SCR_BaseLockComponent));
        if (lockComp)
        {
            lockComp.SetLocked(true);
            BH_Log("BH_RelockHelicopter: helicoptero re-bloqueado");
        }
    }

    //------------------------------------------------------------------------------------------------

    // Resetea todas las variables del sistema de embarque
    protected void BH_ResetBoardingState()
    {
        m_BoardingStarted = false;
        m_PassengersOnBoard = 0;
        m_MinReached = false;

        // Limpiar callqueues de boarding por si hay pendientes
        GetGame().GetCallqueue().Remove(BH_BoardingCheck);
        GetGame().GetCallqueue().Remove(BH_BoardingPeriodicMessage);
        GetGame().GetCallqueue().Remove(BH_BoardingTimeout);
        GetGame().GetCallqueue().Remove(BH_BoardingDone);
    }

    //------------------------------------------------------------------------------------------------
    // Metodos para SlotBase
    //------------------------------------------------------------------------------------------------

    void BH_AddWaypoint(BH_AmbientHelicopterWaypoint waypoint)
    {
        m_Waypoins.Insert(waypoint);
    }

    void BH_UpdateSimSettings(int maxSpeed, bool AttackPlayers, int AttackPlayersTimeout, int minHeight, int waypointDistance, float defaultRotationPower, float RotationPowerDividor, float damageReduction)
    {
        m_MaxSpeed = maxSpeed;
        m_AttackPlayers = AttackPlayers;
        m_AttackPlayersTimeout = AttackPlayersTimeout;
        m_MinHeightAboveTerrain = minHeight;
        m_MinWaypointDistance = waypointDistance;
        m_defaultRotationPower = defaultRotationPower;
        m_RotationPowerDividor = RotationPowerDividor;
        m_reduceDamage = damageReduction;
    }

    void BH_UpdateCrewSettings(ResourceName Pilot, ResourceName CoPilot, bool CrewIsinvulnerable, bool GunnerSlotsAvailable, int GunnerSlotCount, ResourceName GunnerPrefab)
    {
        m_PilotPrefab = Pilot;
        m_CoPilotPrefab = CoPilot;
        m_CrewIsinvulnerable = CrewIsinvulnerable;
        m_GunnerSlotsAvailable = GunnerSlotsAvailable;
        m_GunnerSlotCount = GunnerSlotCount;
        m_GunnerPrefab = GunnerPrefab;
    }

    void BH_UpdateLightSettings(bool turnOnLights, bool turnOnLandingLights, bool turnOnHazzardLights, bool turnOnSearchLights)
    {
        m_TurnOnLights = turnOnLights;
        m_TurnOnLandingLights = turnOnLandingLights;
        m_TurnOnHazzardLight = turnOnHazzardLights;
        m_TurnOnSearchLight = turnOnSearchLights;
    }

    void BH_UpdateSlotBase(SCR_ScenarioFrameworkSlotAmbientHelicopter slotBase)
    {
        m_SlotBase = slotBase;
    }

    void BH_SetDebugMode(bool enabled)
    {
        m_DebugMode = enabled;
        if (m_DebugMode)
            Print("[BH_Heli_DBG] Debug mode ACTIVADO");
    }

    void BH_UpdateLockSetting(bool lockForPlayers)
    {
        m_LockForPlayers = lockForPlayers;
    }
}