[ComponentEditorProps(category: "BH", description: "Backpack inject")]
class BH_BackpackInspectGameModeComponentClass : ScriptComponentClass {}

class BH_BackpackInspectGameModeComponent : ScriptComponent
{
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (Replication.IsClient())
            return;

        SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (gameMode)
            gameMode.GetOnPlayerSpawned().Insert(BH_OnPlayerSpawned);
    }

    protected void BH_OnPlayerSpawned(int playerId, IEntity character)
    {
        if (!character)
            return;

        if (character.FindComponent(BH_BackpackInspectComponent))
            return;

        Print("[BH] WARN: BH_BackpackInspectComponent no encontrado en " + character.GetName() + ". Aniadelo al prefab del Character.", LogLevel.WARNING);
    }
}