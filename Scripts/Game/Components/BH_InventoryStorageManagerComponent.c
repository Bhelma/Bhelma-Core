modded class SCR_InventoryStorageManagerComponent : ScriptedInventoryStorageManagerComponent
{
    override protected bool ShouldForbidRemoveByInstigator(InventoryStorageManagerComponent instigatorManager, BaseInventoryStorageComponent fromStorage, IEntity item)
    {
        // Permitir sacar ítems de la mochila si estamos siendo inspeccionados
        if (fromStorage && fromStorage.GetOwner())
        {
            IEntity storageOwner = fromStorage.GetOwner();
            InventoryItemComponent itemComp = InventoryItemComponent.Cast(
                storageOwner.FindComponent(InventoryItemComponent));

            if (itemComp)
            {
                InventoryStorageSlot parentSlot = itemComp.GetParentSlot();
                if (parentSlot)
                {
                    EquipedLoadoutStorageComponent loadoutStorage = EquipedLoadoutStorageComponent.Cast(
                        parentSlot.GetStorage());
                    if (loadoutStorage)
                    {
                        IEntity backpackEntity = loadoutStorage.GetClothFromArea(LoadoutBackpackArea);
                        if (backpackEntity && backpackEntity == storageOwner)
                            return false;
                    }
                }
            }
        }

        return super.ShouldForbidRemoveByInstigator(instigatorManager, fromStorage, item);
    }
}