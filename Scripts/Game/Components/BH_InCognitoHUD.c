/*
=============================================================================
	BH_InCognitoHUD.c
	
	Gestiona el indicador visual (HUD) del estado de incógnito.
	
	Icono:
	- Si se configura una imagen .edds en el editor, la usa
	- Si no, dibuja un placeholder procedural (círculo verde) usando
	  widgets de color sólido, sin dependencias externas
	
	El HUD se muestra SOLO cuando el jugador está en incógnito.
	Se oculta completamente al estar comprometido.
	
	Autor: BH
=============================================================================
*/

class BH_InCognitoHUD
{
	//---------------------------------------------------------------------
	// CONSTANTES DE POSICIÓN Y TAMAÑO
	//---------------------------------------------------------------------
	
	//! Margen desde el borde derecho y superior (px, resolución de referencia 1920x1080)
	protected static const int MARGEN_X = 24;
	protected static const int MARGEN_Y = 24;
	
	//! Tamaño del contenedor del icono (px)
	protected static const int ANCHO_ICONO = 256;
	protected static const int ALTO_ICONO  = 256;
	
	//! Tamaño del círculo interno del placeholder (px, debe ser <= tamaño del icono)
	protected static const int ANCHO_CIRCULO = 56;
	protected static const int ALTO_CIRCULO  = 56;
	
	//---------------------------------------------------------------------
	// VARIABLES
	//---------------------------------------------------------------------
	
	//! Widget contenedor raíz
	protected Widget m_wRaiz;
	
	//! Widget de imagen (si se configura .edds desde el editor)
	protected ImageWidget m_wImagen;
	
	//! Widget del placeholder visual (círculo de color)
	protected Widget m_wPlaceholder;
	
	//! Estado que se está mostrando actualmente
	protected int m_iEstadoActual = -1; // -1 = sin inicializar
	
	//! Recurso de imagen configurado desde el editor
	protected ResourceName m_sRecursoIncognito;
	
	//---------------------------------------------------------------------
	// CONSTRUCTOR / DESTRUCTOR
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void BH_InCognitoHUD(ResourceName imagenIncognito)
	{
		m_sRecursoIncognito = imagenIncognito;
		BH_CrearWidgets();
	}
	
	//------------------------------------------------------------------------------------------------
	void ~BH_InCognitoHUD()
	{
		BH_Destruir();
	}
	
	//---------------------------------------------------------------------
	// CREACIÓN DE WIDGETS
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Crea y posiciona todos los widgets del HUD en la esquina superior derecha
	protected void BH_CrearWidgets()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			Print("[BH_Incognito] ERROR: No se pudo obtener el workspace", LogLevel.ERROR);
			return;
		}
		
		// Contenedor raíz: frame anclado en esquina superior derecha
		m_wRaiz = workspace.CreateWidget(
			WidgetType.FrameWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromInt(Color.WHITE),
			100,  // orden de renderizado
			null  // sin padre: se adjunta al workspace raíz
		);
		
		if (!m_wRaiz)
		{
			Print("[BH_Incognito] ERROR: No se pudo crear el widget contenedor", LogLevel.ERROR);
			return;
		}
		
		// Posición del icono en pantalla.
		// POS_X: distancia en px desde el borde DERECHO (positivo = más hacia la izquierda)
		// POS_Y: distancia en px desde el borde SUPERIOR (positivo = más hacia abajo)
		// El sistema de layout de Reforger escala automáticamente según la resolución,
		// por lo que estos valores se ven igual en 1080p, 2K y 4K.
		int posX = 200; // <-- AJUSTAR
		int posY = 8;  // <-- AJUSTAR
		
		FrameSlot.SetAnchorMin(m_wRaiz, 1, 0);
		FrameSlot.SetAnchorMax(m_wRaiz, 1, 0);
		FrameSlot.SetPos(m_wRaiz, -posX, posY);
		FrameSlot.SetSize(m_wRaiz, ANCHO_ICONO, ALTO_ICONO);
		FrameSlot.SetSizeToContent(m_wRaiz, false);
		
		// Crear el icono visual: imagen real o placeholder procedural
		if (m_sRecursoIncognito != ResourceName.Empty)
			BH_CrearIconoImagen(workspace);
		else
			BH_CrearIconoPlaceholder(workspace);
		
		// Estado inicial: oculto
		m_wRaiz.SetVisible(false);
		
		Print("[BH_Incognito] HUD creado correctamente", LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Crea el icono usando la imagen .edds configurada en el editor
	protected void BH_CrearIconoImagen(WorkspaceWidget workspace)
	{
		Widget widgetBase = workspace.CreateWidget(
			WidgetType.ImageWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND | WidgetFlags.STRETCH,
			Color.FromInt(Color.WHITE),
			0,
			m_wRaiz
		);
		
		m_wImagen = ImageWidget.Cast(widgetBase);
		if (!m_wImagen)
		{
			Print("[BH_Incognito] ERROR: No se pudo crear el widget de imagen. Usando placeholder.", LogLevel.WARNING);
			BH_CrearIconoPlaceholder(workspace);
			return;
		}
		
		// Ocupar todo el contenedor padre con stretch absoluto
		FrameSlot.SetAnchorMin(m_wImagen, 0, 0);
		FrameSlot.SetAnchorMax(m_wImagen, 0, 0);
		FrameSlot.SetPos(m_wImagen, 0, 0);
		FrameSlot.SetSize(m_wImagen, ANCHO_ICONO, ALTO_ICONO);
		
		// Cargar la textura
		m_wImagen.LoadImageTexture(0, m_sRecursoIncognito);
		m_wImagen.SetImage(0);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Crea un placeholder visual procedural: círculo verde semitransparente con borde.
	//! No requiere ningún archivo de imagen externo.
	//! Para sustituirlo por un icono real, configura m_sImagenIncognito en el editor.
	protected void BH_CrearIconoPlaceholder(WorkspaceWidget workspace)
	{
		// Fondo exterior: cuadrado semitransparente oscuro (simula el borde)
		Widget fondo = workspace.CreateWidget(
			WidgetType.FrameWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(0, 0, 0, 160),  // negro semitransparente
			0,
			m_wRaiz
		);
		if (fondo)
		{
			FrameSlot.SetAnchorMin(fondo, 0.5, 0.5);
			FrameSlot.SetAnchorMax(fondo, 0.5, 0.5);
			int offset = ANCHO_CIRCULO / 2;
			FrameSlot.SetPos(fondo, -offset, -offset);
			FrameSlot.SetSize(fondo, ANCHO_CIRCULO, ALTO_CIRCULO);
		}
		
		// Círculo interior verde: indica incógnito activo
		m_wPlaceholder = workspace.CreateWidget(
			WidgetType.FrameWidgetTypeID,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND,
			Color.FromRGBA(40, 200, 80, 220),  // verde opaco
			1,
			m_wRaiz
		);
		if (m_wPlaceholder)
		{
			// Centrado dentro del contenedor, ligeramente más pequeño que el fondo
			int innerSize = ANCHO_CIRCULO - 8;
			int innerOffset = (ANCHO_ICONO - innerSize) / 2;
			FrameSlot.SetAnchorMin(m_wPlaceholder, 0, 0);
			FrameSlot.SetAnchorMax(m_wPlaceholder, 0, 0);
			FrameSlot.SetPos(m_wPlaceholder, innerOffset, innerOffset);
			FrameSlot.SetSize(m_wPlaceholder, innerSize, innerSize);
		}
	}
	
	//---------------------------------------------------------------------
	// CONTROL DE ESTADO
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Actualiza el HUD para mostrar el estado indicado.
	//! Acepta int para compatibilidad con el sistema RPC.
	//! \\param estado Valor de BH_EInCognitoEstado (0=INCOGNITO, 1=COMPROMETIDO)
	void BH_MostrarEstado(int estado)
	{
		if (m_iEstadoActual == estado)
			return;
		
		m_iEstadoActual = estado;
		
		if (estado == BH_EInCognitoEstado.INCOGNITO)
			BH_MostrarIncognito();
		else
			BH_OcultarHUD();
		
		string nombreEstado = "COMPROMETIDO";
		if (estado == BH_EInCognitoEstado.INCOGNITO)
			nombreEstado = "INCOGNITO";
		Print("[BH_Incognito] Estado cambiado: " + nombreEstado, LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Hace visible el HUD (estado incógnito)
	protected void BH_MostrarIncognito()
	{
		if (m_wRaiz)
			m_wRaiz.SetVisible(true);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Oculta el HUD completo (estado comprometido o sin estado)
	protected void BH_OcultarHUD()
	{
		if (m_wRaiz)
			m_wRaiz.SetVisible(false);
	}
	
	//---------------------------------------------------------------------
	// LIMPIEZA
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	void BH_Destruir()
	{
		if (m_wRaiz)
		{
			m_wRaiz.RemoveFromHierarchy();
			m_wRaiz = null;
		}
		
		m_wImagen      = null;
		m_wPlaceholder = null;
		
		Print("[BH_Incognito] HUD destruido", LogLevel.NORMAL);
	}
	
	//---------------------------------------------------------------------
	// GETTERS
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	int BH_GetEstadoActual()
	{
		return m_iEstadoActual;
	}
}