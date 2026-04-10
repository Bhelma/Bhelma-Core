// ============================================================
//  BH_NPCInteractionActions.c
//  Acciones de usuario que aparecen al acercarse al NPC.
//
//  Añadir en el ActionsManagerComponent del prefab del NPC:
//    - BH_TalkAction   → "Hablar"
//    - BH_ArrestAction → "Exposar"
//    - BH_ReleaseAction → "Soltar detenido"
//
//  El nombre visible de cada acción se configura en el editor
//  via el campo UIInfo de cada entrada del ActionsManagerComponent.
// ============================================================


// ------------------------------------------------------------
//  ACCIÓN: HABLAR CON EL NPC
// ------------------------------------------------------------

class BH_TalkAction : ScriptedUserAction
{
	protected BH_NPCInteractionComponent m_InteractionComp;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_InteractionComp = BH_NPCInteractionComponent.Cast(
			pOwnerEntity.FindComponent(BH_NPCInteractionComponent)
		);
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_InteractionComp && m_InteractionComp.BH_CanTalk();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return m_InteractionComp &&
			   m_InteractionComp.BH_CanTalk() &&
			   !m_InteractionComp.BH_IsArrested();
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_InteractionComp)
			m_InteractionComp.BH_OnTalk(pUserEntity);
	}
}


// ------------------------------------------------------------
//  ACCIÓN: EXPOSAR / ARRESTAR AL NPC
// ------------------------------------------------------------

class BH_ArrestAction : ScriptedUserAction
{
	protected BH_NPCInteractionComponent m_InteractionComp;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_InteractionComp = BH_NPCInteractionComponent.Cast(
			pOwnerEntity.FindComponent(BH_NPCInteractionComponent)
		);
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_InteractionComp && m_InteractionComp.BH_CanArrest();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return m_InteractionComp &&
			   m_InteractionComp.BH_CanArrest() &&
			   !m_InteractionComp.BH_IsArrested();
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_InteractionComp)
			m_InteractionComp.BH_OnArrest(pUserEntity);
	}
}


// ------------------------------------------------------------
//  ACCIÓN: SOLTAR AL NPC
// ------------------------------------------------------------

class BH_ReleaseAction : ScriptedUserAction
{
	protected BH_NPCInteractionComponent m_InteractionComp;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_InteractionComp = BH_NPCInteractionComponent.Cast(
			pOwnerEntity.FindComponent(BH_NPCInteractionComponent)
		);
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_InteractionComp && m_InteractionComp.BH_IsArrested();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return m_InteractionComp && m_InteractionComp.BH_IsArrested();
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_InteractionComp)
			m_InteractionComp.BH_OnRelease(pUserEntity);
	}
}


// ------------------------------------------------------------
//  ACCIÓN: RESCATAR AL NPC PRISIONERO
// ------------------------------------------------------------

class BH_RescueAction : ScriptedUserAction
{
	protected BH_NPCInteractionComponent m_InteractionComp;

	override void Init(IEntity pOwnerEntity, GenericComponent pManagerComponent)
	{
		m_InteractionComp = BH_NPCInteractionComponent.Cast(
			pOwnerEntity.FindComponent(BH_NPCInteractionComponent)
		);
	}

	override bool CanBeShownScript(IEntity user)
	{
		return m_InteractionComp && m_InteractionComp.BH_IsPrisoner();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return m_InteractionComp && m_InteractionComp.BH_IsPrisoner();
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (m_InteractionComp)
			m_InteractionComp.BH_OnRescue(pUserEntity);
	}
}