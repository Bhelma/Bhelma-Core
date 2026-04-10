// ----------------------------------------------------------------
// BH_CivilianSpyComponent
// Este componente va en el prefab base de cada civil.
// Se mantiene inactivo hasta que el spawner lo activa con BH_ActivateAsSpy().
// Como esta en una entidad con RplComponent (el civil), los Rpc() funcionan
// correctamente en servidor dedicado.
// ----------------------------------------------------------------

[EntityEditorProps(category: "BH/AI", description: "Civilian Spy Component", color: "96 255 0 255")]
class BH_CivilianSpyComponentClass : ScriptComponentClass {}

class BH_CivilianSpyComponent : ScriptComponent
{
    // ----------------------------------------------------------------
    // Atributos configurables en el prefab del civil (valores por defecto)
    // El spawner puede sobreescribirlos antes de activar el espia
    // ----------------------------------------------------------------

    [Attribute("6", UIWidgets.EditBox, "Comprobar distancia al jugador cada X segundos", category: "BH Spy")]
    protected int m_iCheckInterval;

    [Attribute("20", UIWidgets.EditBox, "Distancia a la que el espia detecta al jugador (metros)", category: "BH Spy")]
    protected float m_fDetectionDistance;

    [Attribute("60", UIWidgets.EditBox, "Distancia a la que el jugador debe alejarse para que el espia vuelva a ser civil (metros)", category: "BH Spy")]
    protected float m_fRevertDistance;

    [Attribute("50", UIWidgets.Slider, "Probabilidad (%) de que el espia ataque al ser detectado", "0 100 1", category: "BH Spy")]
    protected int m_iAttackChance;

    [Attribute("USSR", UIWidgets.EditBox, "Faction key a la que cambia el espia al activarse", category: "BH Spy")]
    protected string m_sEnemyFactionKey;

    [Attribute("Civilian", UIWidgets.EditBox, "Faction key original del civil", category: "BH Spy")]
    protected string m_sCivilianFactionKey;

    [Attribute("US", UIWidgets.EditBox, "Faction key de los jugadores que reciben el hint", category: "BH Spy")]
    protected string m_sHintFactionKey;

    [Attribute("", UIWidgets.Auto, "Arma que porta el espia (prefab)", params: "et", category: "BH Spy")]
    protected ResourceName m_WeaponPrefab;

    [Attribute("", UIWidgets.Auto, "Cargador del arma (prefab)", params: "et", category: "BH Spy")]
    protected ResourceName m_MagazinePrefab;

    [Attribute("3", UIWidgets.EditBox, "Cantidad de cargadores", category: "BH Spy")]
    protected int m_iMagazineCount;

    [Attribute("BH_Spy_Voice", UIWidgets.EditBox, "Sound event al atacar (definido en SoundComponent del civil)", category: "BH Spy")]
    protected string m_sSoundOnAttack;

    [Attribute("ATENCION: Hay un traidor entre los civiles", UIWidgets.EditBox, "Titulo del hint", category: "BH Spy")]
    protected string m_sHintTitle;

    [Attribute("Un civil ha revelado su verdadera identidad y esta atacando", UIWidgets.EditBox, "Descripcion del hint", category: "BH Spy")]
    protected string m_sHintDescription;

    [Attribute("5", UIWidgets.Slider, "Duracion del hint en segundos", "1 30 1", category: "BH Spy")]
    protected float m_fHintDuration;

    [Attribute("1", UIWidgets.CheckBox, "Mostrar hint a los jugadores cuando se activa el espia", category: "BH Spy")]
    protected bool m_bShowHint;

    [Attribute("2", UIWidgets.EditBox, "Tiempo minimo de reaccion antes de sacar arma (segundos)", category: "BH Spy")]
    protected float m_fReactionTimeMin;

    [Attribute("6", UIWidgets.EditBox, "Tiempo maximo de reaccion antes de sacar arma (segundos)", category: "BH Spy")]
    protected float m_fReactionTimeMax;

    [Attribute("0", UIWidgets.Slider, "Probabilidad (%) de que el espia huya en vez de atacar. 0 siempre ataca.", "0 100 1", category: "BH Spy")]
    protected int m_iFleeChance;

    // ----------------------------------------------------------------
    // Variables internas
    // ----------------------------------------------------------------

    protected bool m_bIsActiveSpy = false;
    protected bool m_bIsAttacking = false;
    protected bool m_bIsFleeing = false;
    protected bool m_bIsInReactionPhase = false;
    protected SCR_ChimeraCharacter m_ownerChar;
    protected FactionAffiliationComponent m_factionComp;
    protected PlayerManager m_playerManager;
    protected BaseWorld m_world;

    // Referencia al spawner para el sistema de cooldown
    protected BH_AICivilianSpawnerComponent m_spawnerRef;

    // Guardamos los targets detectados para usarlos tras el tiempo de reaccion
    protected ref array<IEntity> m_pendingTargets = new array<IEntity>();

    static const string ATTACK_WAYPOINT_PREFAB = "{1B0E3436C30FA211}Prefabs/AI/Waypoints/AIWaypoint_Attack.et";
    static const string MOVE_WAYPOINT_PREFAB = "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et";

    // ----------------------------------------------------------------

    override protected void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        if (SCR_Global.IsEditMode()) return;
        if (!Replication.IsServer()) return;

        m_ownerChar = SCR_ChimeraCharacter.Cast(owner);
        m_playerManager = GetGame().GetPlayerManager();
        m_world = GetGame().GetWorld();
    }

    // ----------------------------------------------------------------
    // API publica
    // ----------------------------------------------------------------

    void BH_Configure(float detectionDist, float revertDist, int attackChance, string enemyFaction, string civilianFaction, string hintFaction, ResourceName weaponPrefab, ResourceName magazinePrefab, int magazineCount, string soundOnAttack)
    {
        m_fDetectionDistance = detectionDist;
        m_fRevertDistance = revertDist;
        m_iAttackChance = attackChance;
        m_sEnemyFactionKey = enemyFaction;
        m_sCivilianFactionKey = civilianFaction;
        m_sHintFactionKey = hintFaction;
        m_WeaponPrefab = weaponPrefab;
        m_MagazinePrefab = magazinePrefab;
        m_iMagazineCount = magazineCount;
        m_sSoundOnAttack = soundOnAttack;
    }

    void BH_ConfigureBehavior(string hintTitle, string hintDesc, float hintDuration, bool showHint, float reactionMin, float reactionMax, int fleeChance)
    {
        m_sHintTitle = hintTitle;
        m_sHintDescription = hintDesc;
        m_fHintDuration = hintDuration;
        m_bShowHint = showHint;
        m_fReactionTimeMin = reactionMin;
        m_fReactionTimeMax = reactionMax;
        m_iFleeChance = fleeChance;
    }

    void BH_SetSpawnerRef(BH_AICivilianSpawnerComponent spawner)
    {
        m_spawnerRef = spawner;
    }

    void BH_ActivateAsSpy()
    {
        if (!m_ownerChar || m_bIsActiveSpy) return;

        m_bIsActiveSpy = true;
        m_factionComp = FactionAffiliationComponent.Cast(m_ownerChar.FindComponent(FactionAffiliationComponent));

        int rndStart = (m_iCheckInterval + Math.RandomIntInclusive(1, 4)) * 1000;
        GetGame().GetCallqueue().CallLater(BH_CheckPlayerDistance, rndStart, false);
    }

    bool BH_IsActiveSpy() { return m_bIsActiveSpy; }
    bool BH_IsAttacking() { return m_bIsAttacking; }
    bool BH_IsFleeing() { return m_bIsFleeing; }

    // ----------------------------------------------------------------
    // Logica principal (solo server)
    // ----------------------------------------------------------------

    protected void BH_CheckPlayerDistance()
    {
        if (!m_ownerChar || m_bIsInReactionPhase) return;

        SCR_CharacterDamageManagerComponent dmgMgr = SCR_CharacterDamageManagerComponent.Cast(m_ownerChar.GetDamageManager());
        if (!dmgMgr || dmgMgr.GetHealth() <= 0) return;

        if (!m_playerManager) return;

        array<int> playerIDs = new array<int>();
        m_playerManager.GetPlayers(playerIDs);

        float closestDistance = float.MAX;
        ref array<IEntity> nearbyPlayers = new array<IEntity>();

        foreach (int playerID : playerIDs)
        {
            IEntity playerEnt = m_playerManager.GetPlayerControlledEntity(playerID);
            if (!playerEnt) continue;

            SCR_ChimeraCharacter playerChar = SCR_ChimeraCharacter.Cast(playerEnt);
            if (!playerChar) continue;

            SCR_CharacterDamageManagerComponent playerDmg = SCR_CharacterDamageManagerComponent.Cast(playerChar.GetDamageManager());
            if (!playerDmg || playerDmg.GetHealth() <= 0) continue;

            float dist = vector.Distance(m_ownerChar.GetOrigin(), playerEnt.GetOrigin());
            if (dist < closestDistance)
                closestDistance = dist;

            if (dist <= m_fDetectionDistance)
                nearbyPlayers.Insert(playerEnt);
        }

        // Espia en reposo: comprobar si debe activarse
        if (!m_bIsAttacking && !m_bIsFleeing)
        {
            if (nearbyPlayers.Count() > 0 && Math.RandomIntInclusive(0, 100) <= m_iAttackChance)
            {
                // Cooldown: preguntar al spawner si puede activarse
                if (m_spawnerRef && !m_spawnerRef.BH_CanSpyActivate())
                {
                    GetGame().GetCallqueue().CallLater(BH_CheckPlayerDistance, m_iCheckInterval * 1000, false);
                    return;
                }

                if (m_spawnerRef)
                    m_spawnerRef.BH_OnSpyActivated();

                BH_StartReactionPhase(nearbyPlayers);
                return;
            }
        }
        // Espia activo: comprobar si debe revertir
        else
        {
            if (closestDistance >= m_fRevertDistance)
            {
                BH_RevertToFriendly();
                return;
            }

            if (m_bIsAttacking)
                BH_UpdateTarget();
        }

        GetGame().GetCallqueue().CallLater(BH_CheckPlayerDistance, m_iCheckInterval * 1000, false);
    }

    // ----------------------------------------------------------------
    // FASE DE REACCION: el espia se delata pero tarda en actuar
    // El jugador recibe el hint y oye el sonido, pero el espia aun
    // no ha sacado el arma. Genera tension.
    // ----------------------------------------------------------------

    protected void BH_StartReactionPhase(array<IEntity> targets)
    {
        if (!m_ownerChar || !m_factionComp) return;

        m_bIsInReactionPhase = true;

        // Guardar targets para despues
        m_pendingTargets.Clear();
        foreach (IEntity t : targets)
            m_pendingTargets.Insert(t);

        // Cambiar faccion a enemiga (se delata)
        FactionManager factionMgr = GetGame().GetFactionManager();
        if (factionMgr)
        {
            Faction enemyFaction = factionMgr.GetFactionByKey(m_sEnemyFactionKey);
            if (enemyFaction)
                m_factionComp.SetAffiliatedFaction(enemyFaction);
        }

        // Sonido de alerta
        if (!m_sSoundOnAttack.IsEmpty())
        {
            BH_PlayAttackSound();
            Rpc(BH_RpcPlayAttackSound);
        }

        // Hint a jugadores
        if (m_bShowHint)
            BH_SendHintToFaction();

        // Parar IA actual (el espia se queda quieto)
        AIControlComponent aiCtrl = AIControlComponent.Cast(m_ownerChar.FindComponent(AIControlComponent));
        if (aiCtrl)
        {
            AIAgent agent = aiCtrl.GetAIAgent();
            if (agent)
            {
                SCR_AIUtilityComponent utilComp = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
                if (utilComp)
                {
                    SCR_AIBehaviorBase behaviour = utilComp.GetCurrentBehavior();
                    if (behaviour) behaviour.Complete();
                }
            }
        }

        // Tiempo de reaccion aleatorio
        float reactionTime = Math.RandomFloatInclusive(m_fReactionTimeMin, m_fReactionTimeMax);
        int reactionMs = (reactionTime * 1000);

        GetGame().GetCallqueue().CallLater(BH_OnReactionComplete, reactionMs, false);
    }

    protected void BH_OnReactionComplete()
    {
        m_bIsInReactionPhase = false;

        if (!m_ownerChar) return;

        SCR_CharacterDamageManagerComponent dmgMgr = SCR_CharacterDamageManagerComponent.Cast(m_ownerChar.GetDamageManager());
        if (!dmgMgr || dmgMgr.GetHealth() <= 0) return;

        // Decidir: atacar o huir
        if (Math.RandomIntInclusive(0, 100) <= m_iFleeChance)
            BH_StartFlee();
        else
            BH_ExecuteAttack(m_pendingTargets);

        m_pendingTargets.Clear();
    }

    // ----------------------------------------------------------------
    // ATAQUE
    // ----------------------------------------------------------------

    protected void BH_ExecuteAttack(array<IEntity> targets)
    {
        if (!m_ownerChar || !m_factionComp) return;

        m_bIsAttacking = true;

        // Dar arma
        if (m_WeaponPrefab != string.Empty)
        {
            BH_AddItemToInventory(m_ownerChar, m_WeaponPrefab, 100);
            for (int i = 0; i < m_iMagazineCount; i++)
            {
                if (m_MagazinePrefab != string.Empty)
                    BH_AddItemToInventory(m_ownerChar, m_MagazinePrefab, 200 + (i * 100));
            }
        }

        // Asignar objetivos
        SCR_AICombatComponent combatComp = SCR_AICombatComponent.Cast(m_ownerChar.FindComponent(SCR_AICombatComponent));
        if (combatComp)
            combatComp.SetAssignedTargets(targets, null);

        GetGame().GetCallqueue().CallLater(BH_CheckPlayerDistance, m_iCheckInterval * 1000, false);
    }

    // ----------------------------------------------------------------
    // HUIDA
    // ----------------------------------------------------------------

    protected void BH_StartFlee()
    {
        if (!m_ownerChar) return;

        m_bIsFleeing = true;

        IEntity closestPlayer = BH_GetClosestAlivePlayer();
        if (!closestPlayer)
        {
            GetGame().GetCallqueue().CallLater(BH_CheckPlayerDistance, m_iCheckInterval * 1000, false);
            return;
        }

        // Calcular punto de huida: direccion opuesta al jugador, ~100m
        vector spyPos = m_ownerChar.GetOrigin();
        vector playerPos = closestPlayer.GetOrigin();
        vector fleeDir = spyPos - playerPos;
        fleeDir.Normalize();
        vector fleeTarget = spyPos + (fleeDir * 100);
        fleeTarget[1] = m_world.GetSurfaceY(fleeTarget[0], fleeTarget[2]);

        SCR_AIGroup spyGroup = BH_GetGroup();
        if (spyGroup)
        {
            array<AIWaypoint> waypoints = new array<AIWaypoint>();
            spyGroup.GetWaypoints(waypoints);
            foreach (AIWaypoint wp : waypoints)
                spyGroup.RemoveWaypointFromGroup(wp);

            Resource wpResource = Resource.Load(MOVE_WAYPOINT_PREFAB);
            if (wpResource && wpResource.IsValid())
            {
                EntitySpawnParams spawnParams = new EntitySpawnParams();
                spawnParams.Transform[3] = fleeTarget;

                AIWaypoint fleeWP = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(wpResource, m_world, spawnParams));
                if (fleeWP)
                {
                    spyGroup.AddWaypointToGroup(fleeWP);

                    AIGroupMovementComponent moveComp = AIGroupMovementComponent.Cast(spyGroup.FindComponent(AIGroupMovementComponent));
                    if (moveComp)
                        moveComp.SetGroupCharactersWantedMovementType(EMovementType.RUN);
                }
            }
        }

        GetGame().GetCallqueue().CallLater(BH_CheckPlayerDistance, m_iCheckInterval * 1000, false);
    }

    // ----------------------------------------------------------------
    // REVERTIR a civil
    // ----------------------------------------------------------------

    protected void BH_RevertToFriendly()
    {
        if (!m_ownerChar || !m_factionComp) return;

        m_bIsAttacking = false;
        m_bIsFleeing = false;

        FactionManager factionMgr = GetGame().GetFactionManager();
        if (factionMgr)
        {
            Faction civilFaction = factionMgr.GetFactionByKey(m_sCivilianFactionKey);
            if (civilFaction)
                m_factionComp.SetAffiliatedFaction(civilFaction);
        }

        SCR_AIGroup spyGroup = BH_GetGroup();
        if (spyGroup)
        {
            array<AIWaypoint> waypoints = new array<AIWaypoint>();
            spyGroup.GetWaypoints(waypoints);
            foreach (AIWaypoint wp : waypoints)
                spyGroup.RemoveWaypointFromGroup(wp);

            AIGroupMovementComponent moveComp = AIGroupMovementComponent.Cast(spyGroup.FindComponent(AIGroupMovementComponent));
            if (moveComp)
                moveComp.SetGroupCharactersWantedMovementType(EMovementType.WALK);
        }

        GetGame().GetCallqueue().CallLater(BH_CheckPlayerDistance, m_iCheckInterval * 1000, false);
    }

    // ----------------------------------------------------------------
    // ACTUALIZAR OBJETIVO
    // ----------------------------------------------------------------

    protected void BH_UpdateTarget()
    {
        if (!m_ownerChar) return;

        SCR_CharacterDamageManagerComponent dmgMgr = SCR_CharacterDamageManagerComponent.Cast(m_ownerChar.GetDamageManager());
        if (!dmgMgr || dmgMgr.GetHealth() <= 0) return;

        IEntity closestPlayer = BH_GetClosestAlivePlayer();
        if (!closestPlayer) return;

        SCR_AIGroup spyGroup = BH_GetGroup();
        if (!spyGroup) return;

        array<AIWaypoint> waypoints = new array<AIWaypoint>();
        spyGroup.GetWaypoints(waypoints);
        foreach (AIWaypoint wp : waypoints)
            spyGroup.RemoveWaypointFromGroup(wp);

        Resource wpResource = Resource.Load(ATTACK_WAYPOINT_PREFAB);
        if (!wpResource || !wpResource.IsValid()) return;

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = closestPlayer.GetOrigin();

        AIWaypoint attackWP = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(wpResource, m_world, spawnParams));
        if (attackWP)
        {
            spyGroup.AddWaypointToGroup(attackWP);

            AIGroupMovementComponent moveComp = AIGroupMovementComponent.Cast(spyGroup.FindComponent(AIGroupMovementComponent));
            if (moveComp)
                moveComp.SetGroupCharactersWantedMovementType(EMovementType.RUN);
        }
    }

    // ----------------------------------------------------------------
    // SONIDO
    // ----------------------------------------------------------------

    protected void BH_PlayAttackSound()
    {
        SoundComponent soundComp = SoundComponent.Cast(GetOwner().FindComponent(SoundComponent));
        if (soundComp) soundComp.SoundEvent(m_sSoundOnAttack);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void BH_RpcPlayAttackSound()
    {
        if (System.IsConsoleApp()) return;
        BH_PlayAttackSound();
    }

    // ----------------------------------------------------------------
    // HINT
    // ----------------------------------------------------------------

    protected void BH_SendHintToFaction()
    {
        if (!m_playerManager) return;

        if (!Replication.IsRunning())
        {
            BH_ShowHintLocal();
            return;
        }

        if (!Replication.IsServer()) return;

        SCR_FactionManager scrFactionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
        if (!scrFactionMgr) return;

        array<int> playerIDs = new array<int>();
        m_playerManager.GetPlayers(playerIDs);

        foreach (int playerID : playerIDs)
        {
            Faction playerFaction = scrFactionMgr.GetPlayerFaction(playerID);
            if (!playerFaction) continue;
            if (playerFaction.GetFactionKey() != m_sHintFactionKey) continue;

            IEntity playerEntity = m_playerManager.GetPlayerControlledEntity(playerID);
            if (!playerEntity) continue;

            RplId playerId = Replication.FindId(playerEntity);
            if (!playerId.IsValid()) continue;

            Rpc(BH_RpcShowHint, playerId);
        }
    }

    protected void BH_ShowHintLocal()
    {
        SCR_HintManagerComponent.ShowCustomHint(m_sHintDescription, m_sHintTitle, m_fHintDuration);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void BH_RpcShowHint(RplId targetPlayerId)
    {
        if (System.IsConsoleApp()) return;

        IEntity localPlayer = SCR_PlayerController.GetLocalControlledEntity();
        if (!localPlayer) return;
        if (Replication.FindId(localPlayer) != targetPlayerId) return;

        BH_ShowHintLocal();
    }

    // ----------------------------------------------------------------
    // Utilidades
    // ----------------------------------------------------------------

    protected SCR_AIGroup BH_GetGroup()
    {
        if (!m_ownerChar) return null;
        AIControlComponent aiComp = AIControlComponent.Cast(m_ownerChar.FindComponent(AIControlComponent));
        if (!aiComp) return null;
        AIAgent agent = aiComp.GetAIAgent();
        if (!agent) return null;
        AIGroup group = agent.GetParentGroup();
        if (!group) return null;
        return SCR_AIGroup.Cast(group);
    }

    protected IEntity BH_GetClosestAlivePlayer()
    {
        if (!m_playerManager) return null;

        array<int> playerIDs = new array<int>();
        m_playerManager.GetPlayers(playerIDs);

        IEntity closest = null;
        float closestDist = float.MAX;

        foreach (int playerID : playerIDs)
        {
            IEntity playerEnt = m_playerManager.GetPlayerControlledEntity(playerID);
            if (!playerEnt) continue;
            SCR_ChimeraCharacter playerChar = SCR_ChimeraCharacter.Cast(playerEnt);
            if (!playerChar) continue;
            SCR_CharacterDamageManagerComponent playerDmg = SCR_CharacterDamageManagerComponent.Cast(playerChar.GetDamageManager());
            if (!playerDmg || playerDmg.GetHealth() <= 0) continue;

            float dist = vector.Distance(m_ownerChar.GetOrigin(), playerEnt.GetOrigin());
            if (dist < closestDist)
            {
                closestDist = dist;
                closest = playerEnt;
            }
        }
        return closest;
    }

    protected void BH_AddItemToInventory(IEntity charEnt, ResourceName resName, int spawnDelay)
    {
        if (!resName || !charEnt) return;
        if (!Replication.IsServer()) return;
        GetGame().GetCallqueue().CallLater(BH_SpawnItemToInventory, spawnDelay, false, charEnt, resName);
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
        if (spawnedEntity)
            invStoreMgr.TryInsertItemInStorage(spawnedEntity, baseStore);
    }
}