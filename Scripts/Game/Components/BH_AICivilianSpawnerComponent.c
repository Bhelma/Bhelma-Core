[ComponentEditorProps(category: "BH/AI", description: "Spawner de civiles con IA")]
class BH_AICivilianSpawnerComponentClass : ScriptComponentClass {}

class BH_AICivilianSpawnerComponent : ScriptComponent
{
    [Attribute("500", UIWidgets.EditBox, "Distancia minima al jugador para spawnear civiles (metros)", category: "BH Civilian Spawner")]
    protected int m_MinimumDistanceToPlayer;
    [Attribute("6", UIWidgets.EditBox, "Comprobar distancia al jugador cada X segundos", category: "BH Civilian Spawner")]
    protected int m_CheckDistanceEachSeconds;
    [Attribute("{4ECD14650D82F5CA}Prefabs/AI/Waypoints/AIWaypoint_Loiter_CO.et", UIWidgets.Auto, "Prefab del waypoint", params: "et", category: "BH Civilian Spawner")]
    protected ResourceName m_WaypointPrefab;
    [Attribute("400", UIWidgets.EditBox, "Radio del waypoint de patrulla", category: "BH Civilian Spawner")]
    protected int m_WaypointRange;
    [Attribute("1", UIWidgets.CheckBox, "Mostrar radio del waypoint en el editor", category: "BH Civilian Spawner")]
    protected bool m_ShowRangeInEditor;
    [Attribute("8", UIWidgets.EditBox, "Cantidad de civiles a spawnear", category: "BH Civilian Spawner")]
    protected int m_CivCount;
    [Attribute("1", UIWidgets.CheckBox, "Spawnear la mitad de civiles de noche", category: "BH Civilian Spawner")]
    protected bool m_CivCountNighttime;
    [Attribute("", UIWidgets.Auto, "Prefabs de los civiles", category: "BH Civilian Spawner")]
    protected ref array<ResourceName> m_CivPrefabs;
    [Attribute("", UIWidgets.Auto, "Prefab del grupo de civiles", params: "et", category: "BH Civilian Spawner")]
    protected ResourceName m_GroupPrefab;
    [Attribute("1", UIWidgets.CheckBox, "Usar un solo grupo para todos los civiles", category: "BH Civilian Spawner")]
    protected bool m_UseSharedGroup;
    [Attribute("1", UIWidgets.CheckBox, "Elegir civiles aleatoriamente de la lista", category: "BH Civilian Spawner")]
    protected bool m_CivPickRandom;
    [Attribute("1", UIWidgets.CheckBox, "Dar un vendaje a cada civil", category: "BH Civilian Spawner")]
    protected bool m_CivGetBandage;
    [Attribute("1", UIWidgets.CheckBox, "Ocultar civiles en Game Master", category: "BH Civilian Spawner")]
    protected bool m_DisableGameMasterAccess;

    // Spy System
    [Attribute("0", UIWidgets.CheckBox, "Activar sistema de espias/traidores", category: "BH Spy System")]
    protected bool m_bEnableSpySystem;
    [Attribute("30", UIWidgets.Slider, "Probabilidad (%) de que un civil sea espia", "0 100 1", category: "BH Spy System")]
    protected int m_iSpyChance;
    [Attribute("20", UIWidgets.EditBox, "Distancia a la que el espia detecta al jugador (metros)", category: "BH Spy System")]
    protected float m_fSpyDetectionDistance;
    [Attribute("50", UIWidgets.Slider, "Probabilidad (%) de que el espia ataque al ser detectado", "0 100 1", category: "BH Spy System")]
    protected int m_iSpyAttackChance;
    [Attribute("60", UIWidgets.EditBox, "Distancia a la que el jugador debe alejarse para que el espia vuelva a ser civil (metros)", category: "BH Spy System")]
    protected float m_fSpyRevertDistance;
    [Attribute("USSR", UIWidgets.EditBox, "Faction key a la que cambia el espia al activarse", category: "BH Spy System")]
    protected string m_sSpyEnemyFactionKey;
    [Attribute("Civilian", UIWidgets.EditBox, "Faction key original de los civiles", category: "BH Spy System")]
    protected string m_sSpyCivilianFactionKey;
    [Attribute("US", UIWidgets.EditBox, "Faction key de los jugadores que reciben el hint del espia", category: "BH Spy System")]
    protected string m_sSpyHintFactionKey;
    [Attribute("", UIWidgets.Auto, "Arma que porta el espia oculta (prefab)", params: "et", category: "BH Spy System")]
    protected ResourceName m_SpyWeaponPrefab;
    [Attribute("", UIWidgets.Auto, "Cargador del arma del espia (prefab)", params: "et", category: "BH Spy System")]
    protected ResourceName m_SpyMagazinePrefab;
    [Attribute("3", UIWidgets.EditBox, "Cantidad de cargadores", category: "BH Spy System")]
    protected int m_iSpyMagazineCount;
    [Attribute("BH_Spy_Voice", UIWidgets.EditBox, "Sound event al atacar", category: "BH Spy System")]
    protected string m_sSpySoundOnAttack;
    [Attribute("1", UIWidgets.CheckBox, "Mostrar hint a los jugadores cuando se activa un espia", category: "BH Spy System")]
    protected bool m_bShowSpyHint;
    [Attribute("ATENCION: Hay un traidor entre los civiles", UIWidgets.EditBox, "Titulo del hint cuando se activa un espia", category: "BH Spy System")]
    protected string m_sSpyHintTitle;
    [Attribute("Un civil ha revelado su verdadera identidad y esta atacando", UIWidgets.EditBox, "Descripcion del hint cuando se activa un espia", category: "BH Spy System")]
    protected string m_sSpyHintDescription;
    [Attribute("5", UIWidgets.Slider, "Duracion del hint en segundos", "1 30 1", category: "BH Spy System")]
    protected float m_fSpyHintDuration;

    // Spy Behavior
    [Attribute("2", UIWidgets.EditBox, "Tiempo MINIMO de reaccion del espia antes de actuar (segundos)", category: "BH Spy Behavior")]
    protected float m_fSpyReactionTimeMin;
    [Attribute("6", UIWidgets.EditBox, "Tiempo MAXIMO de reaccion del espia antes de actuar (segundos)", category: "BH Spy Behavior")]
    protected float m_fSpyReactionTimeMax;
    [Attribute("0", UIWidgets.Slider, "Probabilidad (%) de que el espia huya en vez de atacar (0 = siempre ataca, 100 = siempre huye)", "0 100 1", category: "BH Spy Behavior")]
    protected int m_iSpyFleeChance;
    [Attribute("30", UIWidgets.EditBox, "Cooldown (segundos) entre activaciones de espias. Evita que todos se delaten a la vez.", category: "BH Spy Behavior")]
    protected int m_iSpyCooldown;

    // Internal
    protected ref array<IEntity> m_SpawnedCivs = new array<IEntity>();
    protected bool m_CivsSpawned = false;
    protected SCR_TimedWaypoint m_SpawnedWaypoint;
    protected IEntity m_ownerEntity;
    protected PlayerManager m_playerManager;
    protected BaseWorld m_world;
    protected SCR_AIGroup m_SharedGroup;
    protected float m_fLastSpyActivationTime = 0;
    protected ResourceName m_bandagePrefab = "{C3F1FA1E2EC2B345}Prefabs/Items/Medicine/FieldDressing_01/FieldDressing_USSR_01.et";

#ifdef WORKBENCH
    protected ref Shape m_DebugShapeOuter;
    protected ref Shape m_DebugShapeCenter;
    protected int m_WaypointShapeColor = ARGB(100, 0x00, 0x10, 0xFF);
#endif

    override void OnPostInit(IEntity owner)
    {
        m_ownerEntity = owner;
        if (m_CivPrefabs.Count() == 0) return;
        SetEventMask(owner, EntityEvent.INIT);
    }

    override void EOnInit(IEntity owner)
    {
#ifdef WORKBENCH
        if (m_ShowRangeInEditor) BH_DrawWaypointRange(m_ShowRangeInEditor);
#endif
        if (SCR_Global.IsEditMode()) return;
        if (!Replication.IsServer()) return;
        m_world = GetGame().GetWorld();
        BH_SpawnWaypoint();
        m_playerManager = GetGame().GetPlayerManager();
        int waitRnd = Math.RandomFloatInclusive(3000, 7000);
        GetGame().GetCallqueue().CallLater(BH_GetClosestPlayerDistance, waitRnd, false);
    }

#ifdef WORKBENCH
    override bool _WB_OnKeyChanged(IEntity owner, BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
    {
        if (key == "m_WaypointRange") BH_DrawWaypointRange(m_ShowRangeInEditor);
        return false;
    }
    override event void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
    {
        if (m_ShowRangeInEditor) BH_DrawWaypointRange(m_ShowRangeInEditor);
    }
    protected void BH_DrawWaypointRange(bool draw)
    {
        if (!draw) return;
        m_DebugShapeCenter = Shape.CreateCylinder(m_WaypointShapeColor, ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOZWRITE | ShapeFlags.ONCE | ShapeFlags.NOOUTLINE, m_ownerEntity.GetOrigin(), 0.25, 10);
        m_DebugShapeOuter = Shape.CreateCylinder(m_WaypointShapeColor, ShapeFlags.TRANSP | ShapeFlags.DOUBLESIDE | ShapeFlags.NOZWRITE | ShapeFlags.ONCE | ShapeFlags.NOOUTLINE, m_ownerEntity.GetOrigin(), m_WaypointRange, 2);
    }
#endif

    bool BH_GetCivSpawned() { return m_CivsSpawned; }
    void BH_SetCivSpawned(bool value) { m_CivsSpawned = value; }

    // Cooldown API para los BH_CivilianSpyComponent
    bool BH_CanSpyActivate()
    {
        if (m_iSpyCooldown <= 0) return true;
        float currentTime = System.GetTickCount() / 1000.0;
        return (currentTime - m_fLastSpyActivationTime) >= m_iSpyCooldown;
    }

    void BH_OnSpyActivated()
    {
        m_fLastSpyActivationTime = System.GetTickCount() / 1000.0;
    }

    protected void BH_GetClosestPlayerDistance()
    {
        if (m_CivsSpawned) return;
        if (!m_ownerEntity || !m_playerManager) return;
        array<int> playerIDs = new array<int>();
        m_playerManager.GetPlayers(playerIDs);
        bool playerIsClose = false;
        if (playerIDs.Count() > 0)
        {
            vector wpCenter = m_ownerEntity.GetOrigin();
            foreach (int playerID : playerIDs)
            {
                IEntity charEnt = m_playerManager.GetPlayerControlledEntity(playerID);
                if (!charEnt) continue;
                SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(charEnt);
                if (!ch) continue;
                SCR_CharacterDamageManagerComponent dmgMgr = SCR_CharacterDamageManagerComponent.Cast(ch.GetDamageManager());
                if (!dmgMgr) continue;
                if (dmgMgr.GetHealth() > 0)
                {
                    float distance = vector.Distance(charEnt.GetOrigin(), wpCenter);
                    if (distance < m_MinimumDistanceToPlayer) { playerIsClose = true; break; }
                }
            }
        }
        if (playerIsClose && !m_CivsSpawned) { BH_SpawnAllCIVs(); m_CivsSpawned = true; return; }
        GetGame().GetCallqueue().CallLater(BH_GetClosestPlayerDistance, m_CheckDistanceEachSeconds * 1000, false);
    }

    protected void BH_SpawnAllCIVs()
    {
        if (m_SpawnedCivs.Count() == 0 && m_CivCount > 0)
        {
            if (m_CivCountNighttime && SCR_WorldTools.BH_IsNightTime() && m_CivCount > 3) m_CivCount /= 2;
            int listIndex = 0;
            float spawnDelay = 300;
            for (int i = 0; i < m_CivCount; ++i)
            {
                ResourceName prefabToSpawn;
                if (m_CivPickRandom)
                    prefabToSpawn = m_CivPrefabs[Math.RandomIntInclusive(0, m_CivPrefabs.Count() - 1)];
                else
                {
                    if (listIndex > m_CivPrefabs.Count() - 1) listIndex = 0; else listIndex++;
                    prefabToSpawn = m_CivPrefabs[listIndex];
                }
                GetGame().GetCallqueue().CallLater(BH_SpawnCIV, spawnDelay, false, prefabToSpawn);
                spawnDelay += 300;
            }
        }
    }

    protected void BH_SpawnWaypoint()
    {
        if (!m_ownerEntity) return;
        Resource resource = Resource.Load(m_WaypointPrefab);
        if (!resource || !resource.IsValid()) return;
        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = m_ownerEntity.GetOrigin();
        IEntity newWP = GetGame().SpawnEntityPrefab(resource, m_world, spawnParams);
        if (newWP)
        {
            m_SpawnedWaypoint = SCR_TimedWaypoint.Cast(newWP);
            if (m_SpawnedWaypoint) { m_SpawnedWaypoint.SetCompletionRadius(m_WaypointRange); m_SpawnedWaypoint.SetPriorityLevel(0); m_SpawnedWaypoint.SetHoldingTime(-1); }
            else { delete newWP; }
        }
    }

    protected void BH_SpawnCIV(ResourceName prefabToSpawn)
    {
        if (!m_ownerEntity || !prefabToSpawn || !m_SpawnedWaypoint) return;
        Resource resource = Resource.Load(prefabToSpawn);
        if (!resource || !resource.IsValid()) return;

        vector center = m_ownerEntity.GetOrigin();
        center[0] = center[0] + Math.RandomInt(0 - (m_WaypointRange / 2), (m_WaypointRange / 2));
        center[2] = center[2] + Math.RandomInt(0 - (m_WaypointRange / 2), (m_WaypointRange / 2));

        vector spawnPos, freePos;
        const vector charBoundsMin = "-0.65 0.004 -0.25";
        const vector charBoundsMax = "0.65 1.7 0.35";
        bool spawnPosIsEmpty = SCR_EmptyPositionHelper.BH_TryFindNearbyGeometryPositionForEntity(null, center, charBoundsMin, charBoundsMax, freePos);
        if (spawnPosIsEmpty) spawnPos = freePos;
        else { array<IEntity> exclude = new array<IEntity>(); SCR_TerrainHelper.SnapToGeometry(spawnPos, center, exclude); }

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = spawnPos;
        IEntity charEnt = GetGame().SpawnEntityPrefab(resource, m_world, spawnParams);
        if (charEnt)
        {
            SCR_WorldTools.BH_SnapEntityToGeometry(charEnt);
            SCR_AIGroup newGRP;
            if (m_UseSharedGroup)
            {
                if (!m_SharedGroup)
                {
                    m_SharedGroup = BH_CreateNewGroup(charEnt);
                    if (m_SharedGroup) { m_SharedGroup.AddWaypointToGroup(m_SpawnedWaypoint); m_SharedGroup.SetDeleteWhenEmpty(true); }
                }
                newGRP = m_SharedGroup;
            }
            else
            {
                newGRP = BH_CreateNewGroup(charEnt);
                if (newGRP) { newGRP.AddWaypointToGroup(m_SpawnedWaypoint); newGRP.SetDeleteWhenEmpty(true); }
            }
            if (newGRP)
            {
                newGRP.AddAIEntityToGroup(charEnt);
                m_SpawnedCivs.Insert(charEnt);
                AIGroupMovementComponent moveComp = AIGroupMovementComponent.Cast(newGRP.FindComponent(AIGroupMovementComponent));
                if (moveComp) moveComp.SetGroupCharactersWantedMovementType(EMovementType.WALK);
            }
            else { SCR_EntityHelper.DeleteEntityAndChildren(charEnt); return; }

            if (m_CivGetBandage) BH_AddItemToInventory(charEnt, m_bandagePrefab, Math.RandomInt(1000, 4000));

            if (m_bEnableSpySystem)
            {
                bool isSpy = Math.RandomIntInclusive(0, 100) <= m_iSpyChance;
                if (isSpy) BH_ConfigureAndActivateSpy(charEnt);
            }
        }
    }

    protected void BH_ConfigureAndActivateSpy(IEntity civEntity)
    {
        if (!civEntity) return;
        BH_CivilianSpyComponent spyComp = BH_CivilianSpyComponent.Cast(civEntity.FindComponent(BH_CivilianSpyComponent));
        if (!spyComp) { Print("[BH_CivSpawner] AVISO: Civil no tiene BH_CivilianSpyComponent"); return; }

        spyComp.BH_Configure(
            m_fSpyDetectionDistance, m_fSpyRevertDistance, m_iSpyAttackChance,
            m_sSpyEnemyFactionKey, m_sSpyCivilianFactionKey, m_sSpyHintFactionKey,
            m_SpyWeaponPrefab, m_SpyMagazinePrefab, m_iSpyMagazineCount,
            m_sSpySoundOnAttack
        );
        spyComp.BH_ConfigureBehavior(
            m_sSpyHintTitle, m_sSpyHintDescription, m_fSpyHintDuration, m_bShowSpyHint,
            m_fSpyReactionTimeMin, m_fSpyReactionTimeMax, m_iSpyFleeChance
        );
        spyComp.BH_SetSpawnerRef(this);
        spyComp.BH_ActivateAsSpy();
    }

    protected SCR_AIGroup BH_CreateNewGroup(IEntity ownerAgent)
    {
        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = ownerAgent.GetOrigin();
        Resource groupResource = Resource.Load(m_GroupPrefab);
        if (!groupResource.IsValid()) return null;
        SCR_AIGroup newAIGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(groupResource, m_world, spawnParams));
        if (!newAIGroup) return null;
        if (m_DisableGameMasterAccess)
        {
            SCR_EditableGroupComponent editComp = SCR_EditableGroupComponent.Cast(newAIGroup.FindComponent(SCR_EditableGroupComponent));
            if (editComp) editComp.SetVisible(false);
        }
        return newAIGroup;
    }

    protected void BH_AddItemToInventory(IEntity charEnt, ResourceName resName, int spawnDelay = 1000)
    {
        if (!resName || !charEnt) return;
        if (!Replication.IsServer()) return;
        if (spawnDelay > 0) GetGame().GetCallqueue().CallLater(BH_SpawnItemToInventory, spawnDelay, false, charEnt, resName);
        else BH_SpawnItemToInventory(charEnt, resName);
    }

    protected void BH_SpawnItemToInventory(IEntity charEnt, ResourceName resName)
    {
        if (!resName || !charEnt) return;
        SCR_InventoryStorageManagerComponent invStoreMgr = SCR_InventoryStorageManagerComponent.Cast(charEnt.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!invStoreMgr) return;
        BaseInventoryStorageComponent baseStore = invStoreMgr.FindStorageForResource(resName);
        if (!baseStore) return;
        Resource loadedRes = Resource.Load(resName);
        if (!loadedRes.IsValid()) return;
        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = charEnt.GetOrigin();
        IEntity spawnedEntity = GetGame().SpawnEntityPrefab(loadedRes, GetGame().GetWorld(), spawnParams);
        if (spawnedEntity) invStoreMgr.TryInsertItemInStorage(spawnedEntity, baseStore);
    }
}

class BH_AICivilianSpawnerComponentSerializer : ScriptedComponentSerializer
{
    override static typename GetTargetType() { return BH_AICivilianSpawnerComponent; }
    override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationSaveContext context)
    {
        BH_AICivilianSpawnerComponent spawnerComponent = BH_AICivilianSpawnerComponent.Cast(component);
        if (!spawnerComponent) return ESerializeResult.OK;
        context.WriteValue("version", 1);
        context.WriteValue("m_CIVsSpawned", spawnerComponent.BH_GetCivSpawned());
        return ESerializeResult.OK;
    }
    override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull BaseSerializationLoadContext context)
    {
        BH_AICivilianSpawnerComponent spawnerComponent = BH_AICivilianSpawnerComponent.Cast(component);
        if (!spawnerComponent) return true;
        int version;
        context.Read(version);
        bool groupWasSpawned;
        context.ReadValue("m_CIVsSpawned", groupWasSpawned);
        spawnerComponent.BH_SetCivSpawned(groupWasSpawned);
        return true;
    }
}