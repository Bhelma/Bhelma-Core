// ============================================================================
// BH_DeployButtonLock.c
// Mod: Bhelma - Sistema de Briefing Pre-Partida
//
// DESCRIPCION:
//   Boton INICIAR MISION usando WLib_ButtonText.layout:
//   - Usa m_OnClicked (de SCR_ButtonBaseComponent) que se dispara
//     directamente en OnClick() sin los checks de IsParentMenuFocused
//     ni modal que bloquean m_OnActivated.
//   - m_OnClicked se invoca SIN parametros (ScriptInvokerVoid)
//   - m_OnActivated se invoca CON parametros (component, actionName)
//     -> nuestro callback sin params no matchea y falla silenciosamente
//
//   SCR_MapDrawingUI: permite dibujar en SPAWNSCREEN durante briefing
// ============================================================================

modded class SCR_DeployMenuMain
{
	protected Widget m_wBH_StartButtonRoot;
	
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (briefingMgr && briefingMgr.IsBriefing())
		{
			GetGame().GetCallqueue().CallLater(CreateStartMissionButton, 300, false);
		}
	}
	
	override void OnMenuClose()
	{
		DestroyStartMissionButton();
		super.OnMenuClose();
	}
	
	// -------------------------------------------------------
	protected void CreateStartMissionButton()
	{
		if (m_wBH_StartButtonRoot)
			return;
		
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return;
		
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		bool isAdmin = pm.HasPlayerRole(pc.GetPlayerId(), EPlayerRole.ADMINISTRATOR);
		
		if (briefingMgr && briefingMgr.IsAdminBypass())
			isAdmin = true;
		
		if (!isAdmin)
			return;
		
		Widget rootWidget = GetRootWidget();
		if (!rootWidget)
			return;
		
		// Crear boton desde layout de BI
		m_wBH_StartButtonRoot = GetGame().GetWorkspace().CreateWidgets(
			"{75C912A1C89BE6C2}UI/layouts/WidgetLibrary/Buttons/WLib_ButtonText.layout",
			rootWidget
		);
		
		if (!m_wBH_StartButtonRoot)
		{
			if (briefingMgr)
				briefingMgr.BH_Debug("ERROR: No se pudo crear WLib_ButtonText.layout");
			return;
		}
		
		// Posicionar arriba centro
		FrameSlot.SetAnchorMin(m_wBH_StartButtonRoot, 0.3, 0.01);
		FrameSlot.SetAnchorMax(m_wBH_StartButtonRoot, 0.7, 0.06);
		FrameSlot.SetOffsets(m_wBH_StartButtonRoot, 0, 0, 0, 0);
		
		// Buscar SCR_ButtonTextComponent
		SCR_ButtonTextComponent btnComp = SCR_ButtonTextComponent.Cast(
			m_wBH_StartButtonRoot.FindHandler(SCR_ButtonTextComponent)
		);
		
		if (!btnComp)
		{
			Widget buttonWidget = m_wBH_StartButtonRoot.FindAnyWidget("Button");
			if (buttonWidget)
				btnComp = SCR_ButtonTextComponent.Cast(buttonWidget.FindHandler(SCR_ButtonTextComponent));
		}
		
		if (btnComp)
		{
			// CLAVE: usar m_OnClicked (ScriptInvokerVoid de SCR_ButtonBaseComponent)
			// que se dispara en OnClick() directamente, sin checks extra.
			// NO usar m_OnActivated que invoca con (component, actionName)
			// y ademas tiene checks de IsParentMenuFocused y modal.
			btnComp.m_OnClicked.Insert(OnStartMissionButtonClicked);
			btnComp.SetText("INICIAR MISION [ADMIN]");
			
			if (briefingMgr)
				briefingMgr.BH_Debug("Boton creado con m_OnClicked de SCR_ButtonBaseComponent");
		}
		else
		{
			if (briefingMgr)
				briefingMgr.BH_Debug("AVISO: SCR_ButtonTextComponent no encontrado");
		}
	}
	
	// -------------------------------------------------------
	// m_OnClicked es ScriptInvokerVoid -> callback sin parametros
	// -------------------------------------------------------
	protected void OnStartMissionButtonClicked()
	{
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (briefingMgr)
		{
			briefingMgr.BH_Debug(">>> BOTON INICIAR MISION PULSADO <<<");
			briefingMgr.TryStartMission();
		}
	}
	
	// -------------------------------------------------------
	protected void DestroyStartMissionButton()
	{
		if (m_wBH_StartButtonRoot)
		{
			m_wBH_StartButtonRoot.RemoveFromHierarchy();
			m_wBH_StartButtonRoot = null;
		}
	}
	
	// -------------------------------------------------------
	override protected void RequestRespawn()
	{
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (briefingMgr && briefingMgr.IsBriefing())
			return;
		
		BH_DeathSpectatorComponent deathComp = BH_DeathSpectatorComponent.GetInstance();
		if (deathComp)
		{
			PlayerController pc = GetGame().GetPlayerController();
			if (pc && deathComp.IsPlayerPermaDead(pc.GetPlayerId()))
				return;
		}
		
		super.RequestRespawn();
	}
	
	override protected void UpdateRespawnButton()
	{
		super.UpdateRespawnButton();
		
		bool shouldBlock = false;
		
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (briefingMgr && briefingMgr.IsBriefing())
			shouldBlock = true;
		
		if (!shouldBlock)
		{
			BH_DeathSpectatorComponent deathComp = BH_DeathSpectatorComponent.GetInstance();
			if (deathComp)
			{
				PlayerController pc = GetGame().GetPlayerController();
				if (pc && deathComp.IsPlayerPermaDead(pc.GetPlayerId()))
					shouldBlock = true;
			}
		}
		
		if (shouldBlock && m_RespawnButton)
			m_RespawnButton.SetEnabled(false);
		
		if (briefingMgr && briefingMgr.IsMissionStarted())
			DestroyStartMissionButton();
	}
	
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (briefingMgr && briefingMgr.IsBriefing())
		{
			AudioSystem.SetMasterVolume(AudioSystem.SFX, true);
			AudioSystem.SetMasterVolume(AudioSystem.VoiceChat, true);
			AudioSystem.SetMasterVolume(AudioSystem.Dialog, true);
		}
	}
}

// -------------------------------------------------------
// MODDED SCR_MapDrawingUI
// -------------------------------------------------------
modded class SCR_MapDrawingUI
{
	protected bool m_bBH_BriefingDrawActive = false;
	
	override protected void SetDrawMode(bool state, bool cacheDrawn = false)
	{
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		
		if (!briefingMgr || !briefingMgr.IsBriefing())
		{
			m_bBH_BriefingDrawActive = false;
			super.SetDrawMode(state, cacheDrawn);
			return;
		}
		
		if (state)
		{
			GetGame().GetInputManager().RemoveActionListener("MapSelect", EActionTrigger.UP, BH_OnMapClickBriefing);
			GetGame().GetInputManager().AddActionListener("MapSelect", EActionTrigger.UP, BH_OnMapClickBriefing);
			m_bActivationThrottle = true;
			m_bBH_BriefingDrawActive = true;
			
			for (int i = 0; i < m_iLineCount; i++)
			{
				if (m_aLines[i].m_bIsLineDrawn)
					m_aLines[i].SetButtonVisible(true);
			}
			
			m_bIsDrawModeActive = true;
			m_ToolMenuEntry.SetActive(true);
			m_ToolMenuEntry.m_ButtonComp.SetTextVisible(true);
			UpdateLineCount();
		}
		else
		{
			GetGame().GetInputManager().RemoveActionListener("MapSelect", EActionTrigger.UP, BH_OnMapClickBriefing);
			GetGame().GetInputManager().RemoveActionListener("MapContextualMenu", EActionTrigger.UP, BH_OnMapModifierClick);
			m_bBH_BriefingDrawActive = false;
			
			for (int i = 0; i < m_iLineCount; i++)
			{
				if (m_bIsLineBeingDrawn && i == m_iLineID)
				{
					m_aLines[i].DestroyLine(false);
					m_bIsLineBeingDrawn = false;
					m_iLineID = -1;
				}
				else if (cacheDrawn)
				{
					m_aLines[i].DestroyLine(true);
				}
				else if (m_aLines[i].m_bIsLineDrawn)
				{
					m_aLines[i].SetButtonVisible(false);
				}
			}
			
			m_bIsDrawModeActive = false;
			m_ToolMenuEntry.SetActive(false);
			m_ToolMenuEntry.m_ButtonComp.SetTextVisible(false);
			UpdateLineCount();
		}
	}
	
	protected void BH_OnMapClickBriefing(float value, EActionTrigger reason)
	{
		if (m_bActivationThrottle)
		{
			m_bActivationThrottle = false;
			return;
		}
		
		if (m_bIsLineBeingDrawn)
		{
			m_bIsLineBeingDrawn = false;
			m_aLines[m_iLineID].m_bIsLineDrawn = true;
			m_iLineID = -1;
			SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_STOP);
			GetGame().GetInputManager().RemoveActionListener("MapContextualMenu", EActionTrigger.UP, BH_OnMapModifierClick);
			return;
		}
		
		if (m_iLinesDrawn >= m_iLineCount)
			return;
		
		for (int i = 0; i < m_iLineCount; i++)
		{
			if (!m_aLines[i].m_bIsLineDrawn)
			{
				m_aLines[i].CreateLine(m_wDrawingContainer, true);
				m_bIsLineBeingDrawn = true;
				m_iLineID = i;
				SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_START);
				GetGame().GetInputManager().AddActionListener("MapContextualMenu", EActionTrigger.UP, BH_OnMapModifierClick);
				return;
			}
		}
	}
	
	protected void BH_OnMapModifierClick(float value, EActionTrigger reason)
	{
		GetGame().GetInputManager().RemoveActionListener("MapContextualMenu", EActionTrigger.UP, BH_OnMapModifierClick);
		if (!m_bIsLineBeingDrawn || m_iLineID < 0)
			return;
		m_bIsLineBeingDrawn = false;
		m_aLines[m_iLineID].DestroyLine();
		m_iLineID = -1;
		SCR_UISoundEntity.SoundEvent(SCR_SoundEvent.SOUND_MAP_GADGET_MARKER_DRAW_STOP);
	}
}