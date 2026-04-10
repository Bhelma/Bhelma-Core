[ComponentEditorProps(category: "BH", description: "Backpack inspect")]
class BH_BackpackInspectComponentClass : ScriptComponentClass {}

class BH_BackpackInspectComponent : ScriptComponent
{
    protected SCR_CharacterInventoryStorageComponent    m_Inventory;
    protected SCR_InventoryStorageManagerComponent      m_StorageMgr;
    protected ref BH_InspectBackpackAction              m_InspectAction;
    protected IEntity                                   m_Owner;

    protected static bool                               m_bIsInspectingBackpack = false;

    static bool BH_IsInspectingBackpack()
    {
        return m_bIsInspectingBackpack;
    }

    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        m_Owner      = owner;
        m_Inventory  = SCR_CharacterInventoryStorageComponent.Cast(owner.FindComponent(SCR_CharacterInventoryStorageComponent));
        m_StorageMgr = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));

        if (!m_Inventory || !m_StorageMgr)
            return;

        m_StorageMgr.m_OnItemAddedInvoker.Insert(BH_OnInventoryChanged);
        m_StorageMgr.m_OnItemRemovedInvoker.Insert(BH_OnInventoryChanged);
        m_StorageMgr.m_OnInventoryOpenInvoker.Insert(BH_OnInventoryOpen);

        InventoryItemComponent backpackItem = m_Inventory.GetItemFromLoadoutSlot(new LoadoutBackpackArea());
        if (backpackItem)
            BH_AddInspectAction();
    }

    protected void BH_OnInventoryChanged(IEntity item, BaseInventoryStorageComponent storage)
    {
        if (!m_Inventory)
            return;

        InventoryItemComponent backpackItem = m_Inventory.GetItemFromLoadoutSlot(new LoadoutBackpackArea());
        if (backpackItem)
            BH_AddInspectAction();
        else
            BH_RemoveInspectAction();
    }

    protected void BH_OnInventoryOpen(bool isOpen)
    {
        if (!isOpen)
            m_bIsInspectingBackpack = false;
    }

    protected void BH_AddInspectAction()
    {
        if (m_InspectAction || !m_Owner)
            return;

        ActionsManagerComponent actMgr = ActionsManagerComponent.Cast(
            m_Owner.FindComponent(ActionsManagerComponent)
        );
        if (!actMgr)
            return;

        m_InspectAction = new BH_InspectBackpackAction();
    }

    protected void BH_RemoveInspectAction()
    {
        if (!m_InspectAction)
            return;

        m_InspectAction = null;
    }

    void BH_SendRpc(RplId backpackRplId)
    {
        Rpc(BH_RPC_OpenBackpackMenu, backpackRplId);
    }

    [RplRpc(RplChannel.Reliable, RplRcver.Owner)]
    void BH_RPC_OpenBackpackMenu(RplId backpackRplId)
    {
        RplComponent rplComp = RplComponent.Cast(Replication.FindItem(backpackRplId));
        if (!rplComp) return;
        IEntity backpackEntity = rplComp.GetEntity();
        if (!backpackEntity) return;

        PlayerController playerCtrl = GetGame().GetPlayerController();
        if (!playerCtrl) return;
        IEntity localPlayer = playerCtrl.GetControlledEntity();
        if (!localPlayer) return;

        SCR_InventoryStorageManagerComponent storageMgr = SCR_InventoryStorageManagerComponent.Cast(localPlayer.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!storageMgr) return;

        CharacterVicinityComponent vicinity = CharacterVicinityComponent.Cast(localPlayer.FindComponent(CharacterVicinityComponent));
        if (!vicinity) return;

        IEntity ownerCharacter = backpackEntity.GetParent();
        while (ownerCharacter && !ownerCharacter.FindComponent(SCR_CharacterInventoryStorageComponent))
            ownerCharacter = ownerCharacter.GetParent();

        if (ownerCharacter)
            vicinity.SetItemOfInterest(ownerCharacter);
        else
            vicinity.SetItemOfInterest(backpackEntity);

        storageMgr.SetStorageToOpen(backpackEntity);
        storageMgr.SetLootStorage(backpackEntity);
        storageMgr.PlayItemSound(backpackEntity, SCR_SoundEvent.SOUND_CONTAINER_OPEN);

        m_bIsInspectingBackpack = true;
        storageMgr.OpenInventory();
    }
}