class BH_CuttTool_UserAction : ScriptedUserAction
{
	[Attribute("BH_Cutt_Sound", UIWidgets.EditBox, "Evento de sonido al cortar (del SoundComponent de la valla)", category: "BH Cortar")]
	protected string m_sSoundEvent;

	[Attribute("15", UIWidgets.EditBox, "Duracion de la accion en segundos (debe coincidir con Duration del componente)", category: "BH Cortar")]
	protected float m_fActionDuration;

	[Attribute("", UIWidgets.Auto, "Loadouts permitidos para usar la herramienta. Dejar vacio para permitir cualquier loadout.", category: "BH Cortar")]
	protected ref array<ResourceName> m_aAllowedLoadouts;

	protected SCR_GadgetManagerComponent m_GadgetManager;
	protected bool m_bSoundPlaying = false;
	protected bool m_bWaitingToDelete = false;
	protected IEntity m_pOwnerCached;

	//------------------------------------------------------------------------------------------------
	override void OnActionStart(IEntity pUserEntity)
	{
		if (!pUserEntity)
			return;

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(pUserEntity);
		if (playerId <= 0)
			return;

		m_pOwnerCached = GetOwner();

		// Reproducir sonido
		if (m_pOwnerCached && !m_sSoundEvent.IsEmpty())
		{
			SoundComponent soundComp = SoundComponent.Cast(m_pOwnerCached.FindComponent(SoundComponent));
			if (soundComp)
			{
				soundComp.SoundEvent(m_sSoundEvent);
				m_bSoundPlaying = true;
			}
		}

		// Programar eliminacion
		if (Replication.IsServer() || !Replication.IsRunning())
		{
			m_bWaitingToDelete = true;
			int durationMs = m_fActionDuration * 1000;
			GetGame().GetCallqueue().CallLater(BH_OnActionFinished, durationMs, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnActionCanceled(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		m_bWaitingToDelete = false;
		GetGame().GetCallqueue().Remove(BH_OnActionFinished);
		BH_StopSound(pOwnerEntity);
	}

	//------------------------------------------------------------------------------------------------
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_OnActionFinished()
	{
		if (!m_bWaitingToDelete)
			return;

		m_bWaitingToDelete = false;

		if (!m_pOwnerCached)
			return;

		BH_StopSound(m_pOwnerCached);
		SCR_EntityHelper.DeleteEntityAndChildren(m_pOwnerCached);
		m_pOwnerCached = null;
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_StopSound(IEntity owner)
	{
		if (!owner || !m_bSoundPlaying)
			return;

		SoundComponent soundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if (soundComp)
			soundComp.SoundEvent(SCR_SoundEvent.SOUND_STOP_PLAYING);

		m_bSoundPlaying = false;
	}

	//------------------------------------------------------------------------------------------------
	void BH_SetNewGadgetManager(IEntity from, IEntity to)
	{
		m_GadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(to);
	}

	//------------------------------------------------------------------------------------------------
	//! Comprueba si el jugador tiene un loadout permitido
	protected bool BH_HasRequiredLoadout(IEntity user)
	{
		// Si no se han configurado loadouts, cualquiera puede usar la herramienta
		if (!m_aAllowedLoadouts || m_aAllowedLoadouts.IsEmpty())
			return true;

		// Obtener el ID del jugador
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(user);
		if (playerId <= 0)
			return true;

		// Obtener el loadout manager
		SCR_LoadoutManager loadoutManager = GetGame().GetLoadoutManager();
		if (!loadoutManager)
			return true;

		// Obtener el loadout del jugador
		SCR_BasePlayerLoadout playerLoadout = loadoutManager.GetPlayerLoadout(playerId);
		if (!playerLoadout)
			return true;

		// Obtener el ResourceName del loadout del jugador
		ResourceName playerLoadoutResource = playerLoadout.GetLoadoutResource();

		// Comparar con la lista de loadouts permitidos
		foreach (ResourceName allowedLoadout : m_aAllowedLoadouts)
		{
			if (playerLoadoutResource == allowedLoadout)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShownScript(IEntity user)
	{
		if (!m_GadgetManager)
		{
			m_GadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(user);

			SCR_PlayerController playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (playerController)
				playerController.m_OnControlledEntityChanged.Insert(BH_SetNewGadgetManager);

			return true;
		}

		if (!BH_CuttToolComponent.Cast(m_GadgetManager.GetHeldGadgetComponent()))
			return false;

		// Comprobar rol
		if (!BH_HasRequiredLoadout(user))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformedScript(IEntity user)
	{
		if (!m_GadgetManager)
		{
			m_GadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(user);
			if (!m_GadgetManager)
				return false;
		}

		if (!BH_CuttToolComponent.Cast(m_GadgetManager.GetHeldGadgetComponent()))
			return false;

		// Comprobar rol
		if (!BH_HasRequiredLoadout(user))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void ~BH_CuttTool_UserAction()
	{
		GetGame().GetCallqueue().Remove(BH_OnActionFinished);
	}
}