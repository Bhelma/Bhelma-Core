class SCR_ScenarioFrameworkSlotAmbientHelicopterClass : SCR_ScenarioFrameworkSlotBaseClass
{
}

class SCR_ScenarioFrameworkSlotAmbientHelicopter : SCR_ScenarioFrameworkSlotBase
{
    [Attribute("", UIWidgets.Auto, desc: "Waypoints del helicóptero", category: "BH Helicóptero Ambiental")]
    protected ref array<ref BH_AmbientHelicopterWaypoint> m_Waypoins;

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

    [Attribute("2", UIWidgets.Auto, desc: "Fuerza base del rotor de cola (vuelo recto)", params: "1.7 2.3", category: "BH Helicóptero Ambiental")]
    protected float m_defaultRotationPower;

    [Attribute("15", UIWidgets.Auto, desc: "Velocidad de giro (valor menor = giro más rápido)", params: "10 20", category: "BH Helicóptero Ambiental")]
    protected float m_RotationPowerDividor;

    [Attribute("0", UIWidgets.Auto, desc: "Reducción de daño entrante al helicóptero (0-1, donde 1 = 100%)", params: "0 1", category: "BH Helicóptero Ambiental")]
    protected float m_reduceDamage;

    // Luces
    [Attribute("1", UIWidgets.CheckBox, desc: "Activar luces traseras", category: "BH Helicóptero Ambiental (Luces)")]
    protected bool m_TurnOnLights;

    [Attribute("1", UIWidgets.CheckBox, desc: "Activar luces de aterrizaje (HiBeam) al acercarse al waypoint", category: "BH Helicóptero Ambiental (Luces)")]
    protected bool m_TurnOnLandingLights;

    [Attribute("0", UIWidgets.CheckBox, desc: "Activar foco de búsqueda", category: "BH Helicóptero Ambiental (Luces)")]
    protected bool m_TurnOnSearchLight;

    [Attribute("0", UIWidgets.CheckBox, desc: "Activar luces de peligro (hazard)", category: "BH Helicóptero Ambiental (Luces)")]
    protected bool m_TurnOnHazzardLight;

    // Acciones
    [Attribute(desc: "Acciones que se ejecutan cuando el helicóptero es destruido o dañado gravemente", category: "BH Helicóptero Ambiental (Acciones)")]
    ref array<ref SCR_ScenarioFrameworkActionBase> m_aActionsOnDamage;

    [Attribute(desc: "Acciones que se ejecutan al iniciar el ataque a jugadores (solo una vez)", category: "BH Helicóptero Ambiental (Acciones)")]
    ref array<ref SCR_ScenarioFrameworkActionBase> m_aActionsOnAttack;

    // Tripulación
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

    [Attribute("0", UIWidgets.CheckBox, desc: "Bloquear el helicóptero para que los jugadores no puedan subir", category: "BH Helicóptero Ambiental (Tripulación)")]
    protected bool m_LockForPlayers;

    // Debug
    [Attribute("0", UIWidgets.CheckBox, desc: "Activar modo debug (imprime logs detallados en consola para diagnosticar problemas)", category: "BH Helicóptero Ambiental (Debug)")]
    protected bool m_DebugMode;

    protected BH_AmbientHelicopterControllerComponent m_ambientController;

    //------------------------------------------------------------------------------------------------

    override void SetEntity(IEntity entity)
    {
        if (m_Entity) return;

        m_Entity = entity;
        if (!m_Entity) return;

        BH_InitHelicopter(m_Entity);
    }

    //------------------------------------------------------------------------------------------------

    override IEntity SpawnAsset()
    {
        if (m_Entity) return m_Entity;

        IEntity asset = super.SpawnAsset();
        if (asset) GetGame().GetCallqueue().CallLater(BH_InitHelicopter, 500, false, asset);

        return asset;
    }

    //------------------------------------------------------------------------------------------------

    protected void BH_InitHelicopter(IEntity asset)
    {
        m_ambientController = BH_AmbientHelicopterControllerComponent.Cast(asset.FindComponent(BH_AmbientHelicopterControllerComponent));
        if (!m_ambientController) return;

        m_ambientController.BH_UpdateSlotBase(this);
        m_ambientController.BH_SetDebugMode(m_DebugMode);
        m_ambientController.BH_UpdateCrewSettings(m_PilotPrefab, m_CoPilotPrefab, m_CrewIsinvulnerable, m_GunnerSlotsAvailable, m_GunnerSlotCount, m_GunnerPrefab);
        m_ambientController.BH_UpdateSimSettings(m_MaxSpeed, m_AttackPlayers, m_AttackPlayersTimeout, m_MinHeightAboveTerrain, m_MinWaypointDistance, m_defaultRotationPower, m_RotationPowerDividor, m_reduceDamage);
        m_ambientController.BH_UpdateLightSettings(m_TurnOnLights, m_TurnOnLandingLights, m_TurnOnHazzardLight, m_TurnOnSearchLight);
        m_ambientController.BH_UpdateLockSetting(m_LockForPlayers);

        foreach (BH_AmbientHelicopterWaypoint waypoint : m_Waypoins)
            m_ambientController.BH_AddWaypoint(waypoint);

        if (m_LockForPlayers)
        {
            SCR_BaseLockComponent lockComp = SCR_BaseLockComponent.Cast(asset.FindComponent(SCR_BaseLockComponent));
            if (lockComp) lockComp.SetLocked(true);
        }
    }

    //------------------------------------------------------------------------------------------------

    void BH_ActionOnWaypoint(int WPindex)
    {
        if (m_Waypoins.Count() <= WPindex) return;
        BH_AmbientHelicopterWaypoint wpData = m_Waypoins[WPindex];
        foreach (SCR_ScenarioFrameworkActionBase action : wpData.m_aActionsOnWaypoint)
            action.Init(GetOwner());
    }

    //------------------------------------------------------------------------------------------------

    void BH_ActionOnPlayerCondition(int WPindex)
    {
        if (m_Waypoins.Count() <= WPindex) return;
        BH_AmbientHelicopterWaypoint wpData = m_Waypoins[WPindex];
        foreach (SCR_ScenarioFrameworkActionBase action : wpData.m_aActionsOnPlayerCondition)
            action.Init(GetOwner());
    }

    //------------------------------------------------------------------------------------------------

    void BH_ActionOnDamaged()
    {
        foreach (SCR_ScenarioFrameworkActionBase action : m_aActionsOnDamage)
            action.Init(GetOwner());
    }

    //------------------------------------------------------------------------------------------------

    void BH_ActionOnAttackStart()
    {
        foreach (SCR_ScenarioFrameworkActionBase action : m_aActionsOnAttack)
            action.Init(GetOwner());
    }
}