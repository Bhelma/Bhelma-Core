[ComponentEditorProps(category: "Game/Gamemode", description: "Detecta kills de civiles y notifica al jugador")]
class BH_CivilianKillManagerClass : ScriptComponentClass {}

class BH_CivilianKillManager : ScriptComponent
{
    [Attribute("Civilian", UIWidgets.EditBox, "Faction key de los civiles")]
    protected string m_sCivilianFactionKey;

    [Attribute("HAS ELIMINADO A UN CIVIL", UIWidgets.EditBox, "Titulo de la notificacion")]
    protected string m_sNotificationTitle;

    [Attribute("Ten cuidado con los civiles", UIWidgets.EditBox, "Descripcion de la notificacion")]
    protected string m_sNotificationDescription;

    [Attribute("5", UIWidgets.Slider, "Duracion de la notificacion en segundos", "1 30 1")]
    protected float m_fNotificationDuration;

    [Attribute("3", UIWidgets.EditBox, "Numero de civiles eliminados para terminar la mision")]
    protected int m_iMaxCivilianKills;

    [Attribute("5", UIWidgets.Slider, "Tiempo en segundos antes de que finalice la mision", "1 60 1")]
    protected float m_fEndMissionDelay;

    protected int m_iCurrentCivilianKills;

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (!Replication.IsServer())
            return;

        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(owner);
        if (gameMode)
            gameMode.GetOnControllableDestroyed().Insert(OnEntityKilled);
    }

    protected void OnEntityKilled(SCR_InstigatorContextData contextData)
    {
        if (!contextData)
            return;

        IEntity victim = contextData.GetVictimEntity();
        if (!victim)
            return;

        if (!IsCivilian(victim))
            return;

        int playerId = contextData.GetKillerPlayerID();
        if (playerId <= 0)
            return;

        m_iCurrentCivilianKills++;

        Print(string.Format("[BH_CivilianKillManager] Civiles eliminados: %1 / %2", m_iCurrentCivilianKills, m_iMaxCivilianKills), LogLevel.WARNING);

        NotifyPlayer(playerId);

        if (m_iCurrentCivilianKills >= m_iMaxCivilianKills)
            EndMission();
    }

    protected void EndMission()
    {
        Print("[BH_CivilianKillManager] Limite de civiles alcanzado, terminando mision", LogLevel.WARNING);

        GetGame().GetCallqueue().CallLater(EndMissionDelayed, m_fEndMissionDelay * 1000, false);
    }

    protected void EndMissionDelayed()
    {
        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetOwner());
        if (!gameMode)
            return;

        gameMode.EndGameMode(SCR_GameModeEndData.CreateSimple(EGameOverTypes.EDITOR_FACTION_DEFEAT));
    }

    protected bool IsCivilian(IEntity entity)
    {
        FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(
            entity.FindComponent(FactionAffiliationComponent)
        );

        if (!factionComp)
            return false;

        Faction faction = factionComp.GetAffiliatedFaction();
        if (!faction)
            return false;

        return faction.GetFactionKey() == m_sCivilianFactionKey;
    }

    protected void NotifyPlayer(int playerId)
    {
        if (!Replication.IsRunning())
        {
            ShowNotification();
            return;
        }

        IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
        if (!playerEntity)
            return;

        RplComponent rpl = RplComponent.Cast(playerEntity.FindComponent(RplComponent));
        if (!rpl)
            return;

        Rpc(RpcDo_ShowNotification, Replication.FindId(playerEntity));
    }

    protected void ShowNotification()
    {
        SCR_PopUpNotification.GetInstance().PopupMsg(
            m_sNotificationTitle,
            m_fNotificationDuration,
            m_sNotificationDescription,
            -1,
            "", "", "", "",
            "", "", "", "",
            "",
            SCR_EPopupMsgFilter.ALL
        );
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
    protected void RpcDo_ShowNotification(RplId targetId)
    {
        IEntity localPlayer = SCR_PlayerController.GetLocalControlledEntity();
        if (!localPlayer)
            return;

        RplComponent rpl = RplComponent.Cast(localPlayer.FindComponent(RplComponent));
        if (!rpl)
            return;

        if (Replication.FindId(localPlayer) != targetId)
            return;

        ShowNotification();
    }
}