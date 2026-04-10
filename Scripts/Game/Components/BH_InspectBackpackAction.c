[BaseContainerProps()]
class BH_InspectBackpackAction : ScriptedUserAction
{
    protected const float BH_MAX_INSPECT_DISTANCE = 1.0;

    override bool CanBeShownScript(IEntity user)
    {
        if (!user || user == GetOwner())
            return false;

        IEntity owner = GetOwner();
        if (!owner)
            return false;

        SCR_CharacterDamageManagerComponent damageMgr = SCR_CharacterDamageManagerComponent.Cast(
            owner.FindComponent(SCR_CharacterDamageManagerComponent)
        );
        if (damageMgr && damageMgr.IsDestroyed())
            return false;

        SCR_CharacterInventoryStorageComponent invComp = SCR_CharacterInventoryStorageComponent.Cast(
            owner.FindComponent(SCR_CharacterInventoryStorageComponent)
        );
        if (!invComp)
            return false;

        InventoryItemComponent backpackItem = invComp.GetItemFromLoadoutSlot(new LoadoutBackpackArea());
        if (!backpackItem)
            return false;

        return true;
    }

    override bool CanBePerformedScript(IEntity user)
    {
        if (!user || !GetOwner())
            return false;

        IEntity owner = GetOwner();

        SCR_CharacterDamageManagerComponent damageMgr = SCR_CharacterDamageManagerComponent.Cast(
            owner.FindComponent(SCR_CharacterDamageManagerComponent)
        );
        if (damageMgr && damageMgr.IsDestroyed())
            return false;

        SCR_CharacterInventoryStorageComponent invComp = SCR_CharacterInventoryStorageComponent.Cast(
            owner.FindComponent(SCR_CharacterInventoryStorageComponent)
        );
        if (!invComp)
            return false;

        InventoryItemComponent backpackItem = invComp.GetItemFromLoadoutSlot(new LoadoutBackpackArea());
        if (!backpackItem)
            return false;

        float distance = vector.Distance(user.GetOrigin(), owner.GetOrigin());
        if (distance > BH_MAX_INSPECT_DISTANCE)
            return false;

        return true;
    }

    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        if (!pOwnerEntity || !pUserEntity)
            return;

        SCR_CharacterInventoryStorageComponent invComp = SCR_CharacterInventoryStorageComponent.Cast(
            pOwnerEntity.FindComponent(SCR_CharacterInventoryStorageComponent)
        );
        if (!invComp)
            return;

        InventoryItemComponent backpackItemComp = invComp.GetItemFromLoadoutSlot(new LoadoutBackpackArea());
        if (!backpackItemComp)
            return;

        IEntity backpackEntity = backpackItemComp.GetOwner();
        if (!backpackEntity)
            return;

        RplComponent backpackRpl = RplComponent.Cast(backpackEntity.FindComponent(RplComponent));
        if (!backpackRpl)
            return;

        BH_BackpackInspectComponent inspectComp = BH_BackpackInspectComponent.Cast(
            pUserEntity.FindComponent(BH_BackpackInspectComponent)
        );
        if (!inspectComp)
            return;

        inspectComp.BH_SendRpc(backpackRpl.Id());
    }

    override bool HasLocalEffectOnlyScript()
    {
        return false;
    }
}