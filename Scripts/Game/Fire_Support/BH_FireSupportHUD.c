// ============================================================================
// BH_FireSupportHUD.c
//
// Descripción:
//   Componente HUD del sistema de Soporte de Fuego por Radio.
//   Se añade al PlayerController de la misión.
//
//   Cuando el jugador abre el mapa Y lleva la radio de mochila equipada,
//   aparece automáticamente un panel lateral izquierdo dentro del mapa
//   con las 4 opciones de soporte y sus cooldowns en tiempo real.
//
//   El jugador hace clic en una opción y luego hace clic en el mapa
//   para marcar la posición objetivo. El cursor ya está libre porque
//   el mapa lo gestiona nativamente.
//
//   Al cerrar el mapa el panel desaparece automáticamente.
//
// Autor: Bhelma
//
// Dónde se añade:
//   Al prefab del PlayerController de la misión como ScriptComponent.
//
// Dependencias:
//   - BH_FireSupportComponent.c
// ============================================================================

// ---------------------------------------------------------------------------
// Estados internos
// ---------------------------------------------------------------------------
enum BH_EFireSupportHUDState
{
	CLOSED      = 0,   // Panel cerrado / mapa no abierto
	MENU_OPEN   = 1,   // Panel visible, esperando selección
	WAITING_MAP = 2    // Esperando clic en el mapa para marcar posición
}

// ---------------------------------------------------------------------------
// Class descriptor
// ---------------------------------------------------------------------------
[EntityEditorProps(category: "BH/FireSupport/", description: "Fire Support HUD Component", color: "255 200 0 255")]
class BH_FireSupportHUDClass : ScriptComponentClass
{
}

// ---------------------------------------------------------------------------
// Componente HUD
// ---------------------------------------------------------------------------
class BH_FireSupportHUD : ScriptComponent
{
	// -----------------------------------------------------------------------
	// ATRIBUTOS
	// -----------------------------------------------------------------------

	[Attribute(defvalue: "0",
		desc: "Activar logs de debug en consola.",
		category: "BH Debug")]
	bool m_bBH_Debug;

	[Attribute("5.0", UIWidgets.EditBox,
		desc: "Segundos que se muestra el mensaje de confirmación en pantalla.",
		params: "1 15",
		category: "BH Fire Support - HUD")]
	protected float m_fNotificationDuration;

	// -----------------------------------------------------------------------
	// Constantes de layout del panel
	// -----------------------------------------------------------------------

	protected static const int PANEL_WIDTH    = 320;
	protected static const int PANEL_MARGIN   = 12;
	protected static const int BTN_HEIGHT     = 52;
	protected static const int BTN_MARGIN     = 6;
	protected static const int HEADER_HEIGHT  = 40;
	protected static const int STATUS_HEIGHT  = 30;

	// -----------------------------------------------------------------------
	// Estado interno
	// -----------------------------------------------------------------------

	protected BH_EFireSupportHUDState  m_eState;
	protected BH_EFireSupportType      m_ePendingType;
	protected BH_FireSupportComponent  m_pFireSupportComp;
	protected SCR_MapEntity            m_pMapEntity;
	protected bool                     m_bMapClickRegistered;

	// Widgets del panel (hijos del root del mapa)
	protected Widget     m_wPanelRoot;
	protected TextWidget m_wTitleText;
	protected TextWidget m_wStatusText;
	protected Widget     m_wBtnArtillery;
	protected Widget     m_wBtnHeliAttack;
	protected Widget     m_wBtnHeliSupply;
	protected Widget     m_wBtnHeliExtract;
	protected TextWidget m_wCooldownArtillery;
	protected TextWidget m_wCooldownHeliAttack;
	protected TextWidget m_wCooldownHeliSupply;
	protected TextWidget m_wCooldownHeliExtract;

	// Botón actualmente seleccionado (resalte visual)
	protected Widget     m_wSelectedBtn;

	// Ignorar el primer GetOnSelection tras seleccionar tipo
	// (el mismo clic que selecciona el botón no debe marcar en el mapa)
	protected bool       m_bIgnoreNextMapClick;
	protected int        m_iMapClickIgnoreCount;

	// Widget de notificación (hijo del workspace, independiente del mapa)
	protected Widget m_wNotifRoot;
	protected float  m_fNotifTimer;
	protected bool   m_bNotifActive;

	// Loop de actualización de cooldowns
	protected bool m_bLoopActive;

	// -----------------------------------------------------------------------
	// LIFECYCLE
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (System.IsConsoleApp())
			return;

		m_eState = BH_EFireSupportHUDState.CLOSED;

		// Suscribirse a los eventos del mapa desde el inicio
		SCR_MapEntity.GetOnMapInit().Insert(BH_OnMapOpened);
		SCR_MapEntity.GetOnMapClose().Insert(BH_OnMapClosed);

		BH_Log("OnPostInit: HUD inicializado.");
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		SCR_MapEntity.GetOnMapInit().Remove(BH_OnMapOpened);
		SCR_MapEntity.GetOnMapClose().Remove(BH_OnMapClosed);
		SCR_MapEntity.GetOnSelection().Remove(BH_OnMapClick);

		BH_DestroyPanel();
		BH_HideNotification();
		BH_StopLoop();

		super.OnDelete(owner);
	}

	// -----------------------------------------------------------------------
	// EVENTOS DEL MAPA
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// El jugador abrió el mapa — comprobar si lleva la radio equipada
	protected Widget m_pPendingMapRoot;

	protected void BH_OnMapOpened(MapConfiguration config)
	{
		if (System.IsConsoleApp())
			return;

		m_pFireSupportComp = BH_FindFireSupportComp();
		if (!m_pFireSupportComp)
		{
			BH_Log("BH_OnMapOpened: sin radio equipada, no se muestra el panel.");
			return;
		}

		m_pMapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_pMapEntity)
			return;

		// Guardar root y crear con delay para que el mapa esté inicializado
		m_pPendingMapRoot = config.RootWidgetRef;
		GetGame().GetCallqueue().CallLater(BH_CreatePanelDelayed, 500, false);

		BH_Log("BH_OnMapOpened: panel de soporte creado.");
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_CreatePanelDelayed()
	{
		if (!m_pPendingMapRoot || !m_pFireSupportComp)
			return;

		BH_CreatePanel(m_pPendingMapRoot);
		m_pPendingMapRoot = null;
		m_eState = BH_EFireSupportHUDState.MENU_OPEN;
		BH_StartLoop();
	}

	//------------------------------------------------------------------------------------------------
	// El jugador cerró el mapa
	protected void BH_OnMapClosed(MapConfiguration config)
	{
		if (System.IsConsoleApp())
			return;

		SCR_MapEntity.GetOnSelection().Remove(BH_OnMapClick);

		BH_DestroyPanel();
		BH_StopLoop();

		m_eState = BH_EFireSupportHUDState.CLOSED;
		m_pFireSupportComp = null;
		m_pMapEntity = null;
		m_bMapClickRegistered = false;

		BH_Log("BH_OnMapClosed: panel destruido.");
	}

	// -----------------------------------------------------------------------
	// BÚSQUEDA DE RADIO EQUIPADA
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Busca BH_FireSupportComponent en la radio de mochila del jugador local.
	// Solo devuelve el componente si la radio está en modo IN_SLOT (espalda).
	protected BH_FireSupportComponent BH_FindFireSupportComp()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return null;

		IEntity controlledEnt = pc.GetControlledEntity();
		if (!controlledEnt)
			return null;

		SCR_GadgetManagerComponent gadgetMgr = SCR_GadgetManagerComponent.Cast(
			controlledEnt.FindComponent(SCR_GadgetManagerComponent));
		if (!gadgetMgr)
			return null;

		array<SCR_GadgetComponent> radios = gadgetMgr.GetGadgetsByType(EGadgetType.RADIO_BACKPACK);
		if (!radios)
			return null;

		foreach (SCR_GadgetComponent gadget : radios)
		{
			if (gadget.GetMode() != EGadgetMode.IN_SLOT)
				continue;

			IEntity radioEnt = gadget.GetOwner();
			if (!radioEnt)
				continue;

			BH_FireSupportComponent fsComp = BH_FireSupportComponent.Cast(
				radioEnt.FindComponent(BH_FireSupportComponent));
			if (fsComp)
				return fsComp;
		}

		return null;
	}

	// -----------------------------------------------------------------------
	// CREACIÓN DEL PANEL
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_CreatePanel(Widget mapRootWidget)
	{
		if (!mapRootWidget)
			return;

		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return;

		int panelH = HEADER_HEIGHT + BTN_MARGIN
			+ (BTN_HEIGHT + BTN_MARGIN) * 4
			+ STATUS_HEIGHT + BTN_MARGIN * 2;

		// Panel raíz — hijo del root widget del mapa, anclado a la izquierda
		m_wPanelRoot = ws.CreateWidget(WidgetType.FrameWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(5, 5, 5, 245),
			200,
			mapRootWidget);

		if (!m_wPanelRoot)
			return;

		FrameSlot.SetAnchorMin(m_wPanelRoot, 0.0, 0.5);
		FrameSlot.SetAnchorMax(m_wPanelRoot, 0.0, 0.5);
		FrameSlot.SetPos(m_wPanelRoot, PANEL_MARGIN, -(panelH / 2));
		FrameSlot.SetSize(m_wPanelRoot, PANEL_WIDTH, panelH);
		FrameSlot.SetSizeToContent(m_wPanelRoot, false);

		// Cabecera
		Widget header = ws.CreateWidget(WidgetType.FrameWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(20, 70, 20, 255),
			0, m_wPanelRoot);
		if (header)
		{
			FrameSlot.SetPos(header, 0, 0);
			FrameSlot.SetSize(header, PANEL_WIDTH, HEADER_HEIGHT);

			m_wTitleText = TextWidget.Cast(ws.CreateWidget(WidgetType.TextWidgetTypeID,
				WidgetFlags.VISIBLE | WidgetFlags.BLEND,
				Color.FromRGBA(180, 255, 180, 255),
				0, header));
			if (m_wTitleText)
			{
				m_wTitleText.SetText("[ SOPORTE DE FUEGO ]");
				m_wTitleText.SetExactFontSize(14);
				FrameSlot.SetPos(m_wTitleText, 8, 12);
				FrameSlot.SetSize(m_wTitleText, PANEL_WIDTH - 16, 20);
			}
		}

		// Botones
		int btnY = HEADER_HEIGHT + BTN_MARGIN;
		BH_CreateButton(ws, btnY, "ARTILLERÍA",  "Bombardeo en área marcada",     m_wBtnArtillery,   m_wCooldownArtillery);
		btnY += BTN_HEIGHT + BTN_MARGIN;
		BH_CreateButton(ws, btnY, "HELI ATAQUE", "Helicóptero armado al objetivo",m_wBtnHeliAttack,  m_wCooldownHeliAttack);
		btnY += BTN_HEIGHT + BTN_MARGIN;
		BH_CreateButton(ws, btnY, "SUMINISTRO",  "Caja de munición + humo morado",m_wBtnHeliSupply,  m_wCooldownHeliSupply);
		btnY += BTN_HEIGHT + BTN_MARGIN;
		BH_CreateButton(ws, btnY, "EXTRACCIÓN",  "Helicóptero de evacuación",     m_wBtnHeliExtract, m_wCooldownHeliExtract);
		btnY += BTN_HEIGHT + BTN_MARGIN;

		// Línea de estado
		m_wStatusText = TextWidget.Cast(ws.CreateWidget(WidgetType.TextWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(160, 160, 160, 255),
			0, m_wPanelRoot));
		if (m_wStatusText)
		{
			m_wStatusText.SetText("Haz clic en una opción");
			m_wStatusText.SetExactFontSize(12);
			FrameSlot.SetPos(m_wStatusText, 8, btnY);
			FrameSlot.SetSize(m_wStatusText, PANEL_WIDTH - 16, STATUS_HEIGHT);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_CreateButton(WorkspaceWidget ws, int posY, string label, string desc,
		out Widget btnOut, out TextWidget cooldownOut)
	{
		if (!ws || !m_wPanelRoot)
			return;

		btnOut = ws.CreateWidget(WidgetType.FrameWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(30, 76, 30, 242),
			0, m_wPanelRoot);
		if (!btnOut)
			return;

		FrameSlot.SetPos(btnOut, BTN_MARGIN, posY);
		FrameSlot.SetSize(btnOut, PANEL_WIDTH - BTN_MARGIN * 2, BTN_HEIGHT);

		// Texto principal
		TextWidget lblTxt = TextWidget.Cast(ws.CreateWidget(WidgetType.TextWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(230, 255, 230, 255),
			0, btnOut));
		if (lblTxt)
		{
			lblTxt.SetText(label);
			lblTxt.SetExactFontSize(13);
			FrameSlot.SetPos(lblTxt, 8, 6);
			FrameSlot.SetSize(lblTxt, PANEL_WIDTH - 100, 18);
		}

		// Descripción
		TextWidget descTxt = TextWidget.Cast(ws.CreateWidget(WidgetType.TextWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(150, 204, 150, 255),
			0, btnOut));
		if (descTxt)
		{
			descTxt.SetText(desc);
			descTxt.SetExactFontSize(11);
			FrameSlot.SetPos(descTxt, 8, 26);
			FrameSlot.SetSize(descTxt, PANEL_WIDTH - 100, 16);
		}

		// Cooldown
		cooldownOut = TextWidget.Cast(ws.CreateWidget(WidgetType.TextWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(255, 153, 51, 255),
			0, btnOut));
		if (cooldownOut)
		{
			cooldownOut.SetText("LISTO");
			cooldownOut.SetExactFontSize(12);
			FrameSlot.SetPos(cooldownOut, PANEL_WIDTH - 95, 16);
			FrameSlot.SetSize(cooldownOut, 85, 20);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_DestroyPanel()
	{
		if (m_wPanelRoot)
		{
			m_wPanelRoot.RemoveFromHierarchy();
			m_wPanelRoot = null;
		}

		m_wTitleText       = null;
		m_wStatusText      = null;
		m_wBtnArtillery    = null;
		m_wBtnHeliAttack   = null;
		m_wBtnHeliSupply   = null;
		m_wBtnHeliExtract  = null;
		m_wCooldownArtillery   = null;
		m_wCooldownHeliAttack  = null;
		m_wCooldownHeliSupply  = null;
		m_wCooldownHeliExtract = null;
		m_wSelectedBtn         = null;
	}

	// -----------------------------------------------------------------------
	// LOOP DE ACTUALIZACIÓN
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_StartLoop()
	{
		if (m_bLoopActive)
			return;
		m_bLoopActive = true;
		GetGame().GetCallqueue().CallLater(BH_Tick, 250, true);

		// Registrar listener directo para clics en el mapa
		InputManager inputMgr = GetGame().GetInputManager();
		if (inputMgr)
			inputMgr.AddActionListener("MapSelect", EActionTrigger.UP, BH_OnMapSelectAction);
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_StopLoop()
	{
		if (!m_bLoopActive)
			return;
		m_bLoopActive = false;
		GetGame().GetCallqueue().Remove(BH_Tick);

		InputManager inputMgr = GetGame().GetInputManager();
		if (inputMgr)
			inputMgr.RemoveActionListener("MapSelect", EActionTrigger.UP, BH_OnMapSelectAction);
	}

	//------------------------------------------------------------------------------------------------
	// Callback directo de clic en el mapa — se ejecuta inmediatamente, no cada 250ms
	protected void BH_OnMapSelectAction(float value, EActionTrigger reason)
	{
		if (m_eState == BH_EFireSupportHUDState.MENU_OPEN)
			BH_CheckButtonClick();
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_Tick()
	{
		// Notificación
		if (m_bNotifActive)
		{
			m_fNotifTimer -= 0.25;
			if (m_fNotifTimer <= 0)
				BH_HideNotification();
		}

		if (m_eState != BH_EFireSupportHUDState.MENU_OPEN)
			return;

		if (!m_pFireSupportComp)
			return;

		BH_UpdateButtonStates();
	}

	// -----------------------------------------------------------------------
	// ACTUALIZACIÓN DE BOTONES
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_UpdateButtonStates()
	{
		if (!m_pFireSupportComp)
			return;

		bool globalLocked = m_pFireSupportComp.BH_IsGlobalLocked();

		BH_UpdateSingleButton(m_wBtnArtillery,   m_wCooldownArtillery,   BH_EFireSupportType.ARTILLERY,    globalLocked);
		BH_UpdateSingleButton(m_wBtnHeliAttack,  m_wCooldownHeliAttack,  BH_EFireSupportType.HELI_ATTACK,  globalLocked);
		BH_UpdateSingleButton(m_wBtnHeliSupply,  m_wCooldownHeliSupply,  BH_EFireSupportType.HELI_SUPPLY,  globalLocked);
		BH_UpdateSingleButton(m_wBtnHeliExtract, m_wCooldownHeliExtract, BH_EFireSupportType.HELI_EXTRACT, globalLocked);

		if (m_wStatusText)
		{
			if (globalLocked)
				m_wStatusText.SetText("Soporte activo — espera");
			else
				m_wStatusText.SetText("Haz clic en una opción");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_UpdateSingleButton(Widget btn, TextWidget cooldownTxt, BH_EFireSupportType type, bool globalLocked)
	{
		if (!btn)
			return;

		bool onCooldown = false;
		if (m_pFireSupportComp)
			onCooldown = m_pFireSupportComp.BH_IsOnCooldown(type);

		bool blocked = globalLocked || onCooldown;

		// Color según estado: bloqueado / seleccionado / disponible
		if (blocked)
			btn.SetColor(Color.FromRGBA(56, 30, 20, 242));
		else if (btn == m_wSelectedBtn)
			btn.SetColor(Color.FromRGBA(25, 140, 25, 255)); // verde brillante = seleccionado
		else
			btn.SetColor(Color.FromRGBA(30, 76, 30, 242));

		if (!cooldownTxt)
			return;

		if (onCooldown && m_pFireSupportComp)
		{
			float secs = m_pFireSupportComp.BH_GetCooldownRemaining(type);
			int mins = (int)(secs / 60);
			int secsRest = (int)(secs) - mins * 60;
			string secStr = secsRest.ToString();
			if (secsRest < 10)
				secStr = "0" + secStr;
			cooldownTxt.SetText(mins.ToString() + ":" + secStr);
		}
		else if (globalLocked)
		{
			cooldownTxt.SetText("OCUPADO");
		}
		else
		{
			cooldownTxt.SetText("LISTO");
		}
	}

	// -----------------------------------------------------------------------
	// DETECCIÓN DE CLIC EN BOTONES
	// El cursor del mapa está libre — usamos SCR_MapCursorInfo para la posición
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_CheckButtonClick()
	{
		if (!m_pFireSupportComp)
			return;

		if (m_pFireSupportComp.BH_IsGlobalLocked())
			return;

		// Obtener los widgets bajo el cursor usando el sistema nativo del mapa
		array<Widget> widgetsUnderCursor = SCR_MapCursorModule.GetMapWidgetsUnderCursor();
		if (!widgetsUnderCursor || widgetsUnderCursor.IsEmpty())
			return;

		// Comprobar si alguno de los widgets bajo el cursor es uno de nuestros botones
		// o un hijo de ellos (el texto encima del botón también cuenta)
		foreach (Widget w : widgetsUnderCursor)
		{
			Widget target = BH_FindButtonParent(w);
			if (!target)
				continue;

			if (target == m_wBtnArtillery && !m_pFireSupportComp.BH_IsOnCooldown(BH_EFireSupportType.ARTILLERY))
			{
				BH_SelectType(BH_EFireSupportType.ARTILLERY);
				return;
			}

			if (target == m_wBtnHeliAttack && !m_pFireSupportComp.BH_IsOnCooldown(BH_EFireSupportType.HELI_ATTACK))
			{
				BH_SelectType(BH_EFireSupportType.HELI_ATTACK);
				return;
			}

			if (target == m_wBtnHeliSupply && !m_pFireSupportComp.BH_IsOnCooldown(BH_EFireSupportType.HELI_SUPPLY))
			{
				BH_SelectType(BH_EFireSupportType.HELI_SUPPLY);
				return;
			}

			if (target == m_wBtnHeliExtract && !m_pFireSupportComp.BH_IsOnCooldown(BH_EFireSupportType.HELI_EXTRACT))
			{
				BH_SelectType(BH_EFireSupportType.HELI_EXTRACT);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Sube por la jerarquía de widgets buscando un widget que sea uno de nuestros botones
	protected Widget BH_FindButtonParent(Widget w)
	{
		Widget current = w;
		int maxDepth = 4; // máximo 4 niveles hacia arriba
		int depth = 0;

		while (current && depth < maxDepth)
		{
			if (current == m_wBtnArtillery ||
				current == m_wBtnHeliAttack ||
				current == m_wBtnHeliSupply ||
				current == m_wBtnHeliExtract)
				return current;

			current = current.GetParent();
			depth++;
		}

		return null;
	}

	// -----------------------------------------------------------------------
	// SELECCIÓN DE TIPO Y MARCADO EN MAPA
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_SelectType(BH_EFireSupportType type)
	{
		m_ePendingType = type;
		m_eState = BH_EFireSupportHUDState.WAITING_MAP;
		m_bMapClickRegistered = false;

		// Marcar el botón seleccionado visualmente
		if (type == BH_EFireSupportType.ARTILLERY)
			m_wSelectedBtn = m_wBtnArtillery;
		else if (type == BH_EFireSupportType.HELI_ATTACK)
			m_wSelectedBtn = m_wBtnHeliAttack;
		else if (type == BH_EFireSupportType.HELI_SUPPLY)
			m_wSelectedBtn = m_wBtnHeliSupply;
		else if (type == BH_EFireSupportType.HELI_EXTRACT)
			m_wSelectedBtn = m_wBtnHeliExtract;

		BH_UpdateButtonStates();

		// Mostrar mensaje de instrucción en el panel
		if (m_wStatusText)
			m_wStatusText.SetText(">>> HAZ CLIC EN EL MAPA para marcar el objetivo <<<");

		// Registrar el listener con delay para que el clic del botón ya haya pasado
		GetGame().GetCallqueue().CallLater(BH_RegisterMapClickListener, 300, false);

		BH_Log("BH_SelectType: tipo=" + type + " esperando clic en mapa.");
	}

	//------------------------------------------------------------------------------------------------
	// Se llama con delay para evitar que el clic del botón sea capturado como marcado
	protected void BH_RegisterMapClickListener()
	{
		if (m_eState != BH_EFireSupportHUDState.WAITING_MAP)
			return;

		SCR_MapEntity.GetOnSelection().Insert(BH_OnMapClick);
		BH_Log("BH_RegisterMapClickListener: listener registrado.");
	}

	//------------------------------------------------------------------------------------------------
	// Callback: el jugador hizo clic en el mapa para marcar la posición
	protected void BH_OnMapClick(vector worldPos)
	{
		if (m_bMapClickRegistered)
			return;

		if (m_eState != BH_EFireSupportHUDState.WAITING_MAP)
			return;

		m_bMapClickRegistered = true;
		SCR_MapEntity.GetOnSelection().Remove(BH_OnMapClick);

		BH_Log("BH_OnMapClick: posición = " + worldPos);

		// Colocar marcador en el mapa en la posición seleccionada
		BH_PlaceMapMarker(worldPos);

		// Enviar solicitud al servidor
		if (m_pFireSupportComp)
			m_pFireSupportComp.BH_RequestSupport(m_ePendingType, worldPos);

		// Confirmación
		BH_ShowNotification("Soporte: " + BH_GetTypeName(m_ePendingType) + " — En camino");

		// Restaurar estado del menú
		m_eState = BH_EFireSupportHUDState.MENU_OPEN;
		m_bMapClickRegistered = false;
		m_wSelectedBtn = null;

		if (m_wStatusText)
			m_wStatusText.SetText("Haz clic en una opción");

		BH_UpdateButtonStates();
	}

	// -----------------------------------------------------------------------
	// MARCA EN EL MAPA
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_PlaceMapMarker(vector worldPos)
	{
		// Usar la instancia estática del manager
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
		{
			BH_Log("BH_PlaceMapMarker: no se encontró SCR_MapMarkerManagerComponent.");
			return;
		}

		SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
		marker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		marker.SetWorldPos((int)worldPos[0], (int)worldPos[2]);
		marker.SetCustomText(BH_GetTypeName(m_ePendingType));

		// isLocal=true: solo visible para este jugador (no sincronizado)
		markerMgr.InsertStaticMarker(marker, true);

		BH_Log("BH_PlaceMapMarker: marcador colocado en " + worldPos);
	}

	// -----------------------------------------------------------------------
	// API PÚBLICA
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Para sistemas externos (artillería, helis)
	void BH_ShowExternalNotification(string msg)
	{
		BH_ShowNotification(msg);
	}

	//------------------------------------------------------------------------------------------------
	// Compatibilidad con BH_FireSupportComponent (stub del componente los llama)
	void BH_Toggle(BH_FireSupportComponent comp) {}
	void BH_ForceClose() {}

	// -----------------------------------------------------------------------
	// NOTIFICACIÓN
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_ShowNotification(string msg)
	{
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws)
			return;

		BH_HideNotification();

		int notifW = 460;
		int notifH = 40;

		m_wNotifRoot = ws.CreateWidget(WidgetType.FrameWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			new Color(0.05, 0.15, 0.05, 0.92),
			0, null);

		if (!m_wNotifRoot)
			return;

		FrameSlot.SetAnchorMin(m_wNotifRoot, 0.5, 0.0);
		FrameSlot.SetAnchorMax(m_wNotifRoot, 0.5, 0.0);
		FrameSlot.SetPos(m_wNotifRoot, -(notifW / 2), 80);
		FrameSlot.SetSize(m_wNotifRoot, notifW, notifH);
		FrameSlot.SetSizeToContent(m_wNotifRoot, false);

		TextWidget notifText = TextWidget.Cast(ws.CreateWidget(WidgetType.TextWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			new Color(0.5, 1.0, 0.5, 1.0),
			0, m_wNotifRoot));
		if (notifText)
		{
			notifText.SetText(msg);
			notifText.SetExactFontSize(15);
			FrameSlot.SetPos(notifText, 10, 10);
			FrameSlot.SetSize(notifText, notifW - 20, 22);
		}

		m_fNotifTimer = m_fNotificationDuration;
		m_bNotifActive = true;

		BH_Log("BH_ShowNotification: " + msg);
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_HideNotification()
	{
		if (m_wNotifRoot)
		{
			m_wNotifRoot.RemoveFromHierarchy();
			m_wNotifRoot = null;
		}
		m_bNotifActive = false;
		m_fNotifTimer = 0;
	}

	// -----------------------------------------------------------------------
	// UTILIDADES
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected string BH_GetTypeName(BH_EFireSupportType type)
	{
		if (type == BH_EFireSupportType.ARTILLERY)
			return "Artillería";

		if (type == BH_EFireSupportType.HELI_ATTACK)
			return "Heli Ataque";

		if (type == BH_EFireSupportType.HELI_SUPPLY)
			return "Suministro";

		if (type == BH_EFireSupportType.HELI_EXTRACT)
			return "Extracción";

		return "Desconocido";
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_Log(string msg)
	{
		if (!m_bBH_Debug)
			return;

		Print("[BH_FireSupportHUD] " + msg, LogLevel.NORMAL);
	}
}