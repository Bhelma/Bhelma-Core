modded class SCR_InventoryNavigationButtonBack : SCR_InputButtonComponent
{
    override void SetParentStorage(SCR_InventoryStorageBaseUI pParentStorage)
    {
        if (BH_BackpackInspectComponent.BH_IsInspectingBackpack())
        {
            SetForceDisabled(true, false);
            SetVisible(false, false);
            return;
        }
        m_pParentStorage = pParentStorage;
    }
}