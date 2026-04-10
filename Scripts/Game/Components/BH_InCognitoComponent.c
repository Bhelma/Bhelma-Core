/*
=============================================================================
	BH_InCognitoComponent.c
	
	Componente de incógnito para el GameMode de Arma Reforger.
	
	Funcionalidad:
	- Detecta si un jugador va de incógnito (ropa civil + sin armas equipadas)
	- Muestra un indicador HUD solo al jugador local vía RPC
	- Modifica la percepción de la IA asignando facción CIV temporal
	- Configurable completamente desde el editor de misiones (WorldEditor)
	
	Uso:
	- Añadir este componente al GameMode de la misión para activarlo
	- Configurar parámetros en el panel de propiedades del editor
	
	Multijugador:
	- El servidor ejecuta toda la lógica de detección
	- Notifica a cada cliente via RPC para actualizar su HUD local
	- Compatible con modo cooperativo (jugadores vs IA)
	
	Autor: BH
=============================================================================
*/

//! Enumeración de los posibles estados de incógnito
enum BH_EInCognitoEstado
{
	INCOGNITO    = 0,	//! El jugador pasa desapercibido
	COMPROMETIDO = 1	//! El jugador ha sido identificado como amenaza
};

//! Clase de configuración para items con clasificación forzada manualmente.
//! Se pueden añadir entradas directamente desde el panel de propiedades del editor.
[BaseContainerProps()]
class BH_InCognitoItemForzado
{
	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab del item a clasificar manualmente", "et")]
	ResourceName m_sPrefab;
	
	[Attribute("0", UIWidgets.CheckBox, "Marcar como MILITAR (desmarcado = civil)")]
	bool m_bEsMilitar;
};

//! Clase de configuración para facciones militares editables desde el editor.
//! Permite añadir/quitar facciones personalizadas sin tocar código.
[BaseContainerProps()]
class BH_InCognitoFaccionMilitar
{
	[Attribute("US", UIWidgets.EditBox, "Clave de la facción (debe coincidir exactamente con el FactionManager de la misión)")]
	string m_sClaveFaccion;
};

// Firma del invoker: recibe el ID del jugador y su nuevo estado
void BH_InCognitoEstadoCambiadoMethod(int playerId, BH_EInCognitoEstado nuevoEstado);
typedef func BH_InCognitoEstadoCambiadoMethod;
typedef ScriptInvokerBase<BH_InCognitoEstadoCambiadoMethod> BH_InCognitoEstadoCambiadoInvoker;

//! Componente principal que se añade al GameMode
[ComponentEditorProps(category: "BH/Incognito", description: "Sistema de incógnito. Detecta ropa civil y ausencia de armas. Añadir al GameMode para activarlo.")]
class BH_InCognitoComponentClass : ScriptComponentClass {}

class BH_InCognitoComponent : ScriptComponent
{
	//---------------------------------------------------------------------
	// ATRIBUTOS CONFIGURABLES DESDE EL EDITOR
	//---------------------------------------------------------------------
	
	[Attribute("0.5", UIWidgets.Slider, "Intervalo de comprobación del estado (segundos). Valores bajos son más reactivos pero costosos.", "0.1 5.0 0.1")]
	protected float m_fIntervaloComprobacion;
	
	[Attribute("0", UIWidgets.Slider, "Tiempo de gracia antes de pasar a COMPROMETIDO (segundos). Da margen al jugador para reaccionar.", "0 30 0.5")]
	protected float m_fTiempoGracia;
	
	[Attribute("1", UIWidgets.CheckBox, "¿Activar efecto sobre la percepción de la IA? (cambia facción a CIV cuando va de incógnito)")]
	protected bool m_bAfectarIA;
	
	[Attribute("CIV", UIWidgets.EditBox, "Clave de la facción civil que se asignará al jugador en incógnito (debe existir en el FactionManager de la misión)")]
	protected string m_sClaveFaccionCivil;
	
	[Attribute("", UIWidgets.Object, "Facciones consideradas militares. La ropa asociada a estas facciones compromete al jugador.")]
	protected ref array<ref BH_InCognitoFaccionMilitar> m_aFaccionesMilitaresConfig;
	
	[Attribute("", UIWidgets.Object, "Items con clasificación forzada (manual). Tienen prioridad sobre la detección automática por facción.")]
	protected ref array<ref BH_InCognitoItemForzado> m_aItemsForzados;
	
	[Attribute("{4639923085ED3278}UI/Textures/Icono_Espia.edds", UIWidgets.ResourceNamePicker, "Icono HUD cuando el jugador está en INCÓGNITO (.edds). Si está vacío usa color sólido verde.", "edds")]
	protected ResourceName m_sImagenIncognito;
	
	//---------------------------------------------------------------------
	// VARIABLES INTERNAS
	//---------------------------------------------------------------------
	
	//! Mapa con el estado actual de cada jugador (playerID -> estado)
	protected ref map<int, BH_EInCognitoEstado> m_mEstadosJugadores = new map<int, BH_EInCognitoEstado>();
	
	//! Referencia al widget HUD del jugador local
	protected ref BH_InCognitoHUD m_HUD;
	
	//! Mapa de lookup rápido para items forzados (prefab -> esMilitar)
	protected ref map<ResourceName, bool> m_mItemsForzadosMap = new map<ResourceName, bool>();
	
	//! Invoker público: se dispara cada vez que un jugador cambia de estado.
	//! Otros scripts pueden suscribirse con: incognito.m_OnEstadoCambiado.Insert(MiFuncion)
	ref BH_InCognitoEstadoCambiadoInvoker m_OnEstadoCambiado = new BH_InCognitoEstadoCambiadoInvoker();
	
	//! Facción original de cada jugador antes de modificarla (playerID -> clave de facción)
	protected ref map<int, string> m_mFaccionesOriginales = new map<int, string>();
	
	//! Lista de claves de facciones militares construida en OnPostInit (para búsqueda rápida)
	protected ref array<string> m_aFaccionesMilitaresCache = new array<string>();
	
	//! Override de estado forzado: playerID -> estado forzado
	protected ref map<int, BH_EInCognitoEstado> m_mOverrideEstado = new map<int, BH_EInCognitoEstado>();
	
	//! Override permanente: playerID -> true si es permanente
	protected ref map<int, bool> m_mOverridePermanente = new map<int, bool>();
	
	//! Jugadores actualmente en periodo de gracia (esperando pasar a COMPROMETIDO)
	protected ref array<int> m_aEnGracia = new array<int>();
	
	//---------------------------------------------------------------------
	// CICLO DE VIDA DEL COMPONENTE
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		// Construimos estructuras de búsqueda rápida
		BH_ConstruirCacheFacciones();
		BH_ConstruirMapaItemsForzados();
		
		// El servidor gestiona la lógica de detección y replicación
		if (Replication.IsServer())
			BH_IniciarServidor();
		
		// El HUD solo se crea en el cliente local
		// En partida local (sin servidor dedicado) también creamos el HUD
		if (!Replication.IsRunning() || !Replication.IsServer())
			BH_IniciarCliente();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		BH_Limpiar();
	}
	
	//---------------------------------------------------------------------
	// INICIALIZACIÓN
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Inicialización del lado servidor
	protected void BH_IniciarServidor()
	{
		GetGame().GetCallqueue().CallLater(BH_ComprobarTodosLosJugadores, m_fIntervaloComprobacion * 1000, true);
		Print("[BH_Incognito] Servidor iniciado. Intervalo: " + m_fIntervaloComprobacion + "s | IA: " + m_bAfectarIA + " | Facción CIV: " + m_sClaveFaccionCivil, LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Inicialización del lado cliente: crea el HUD local
	protected void BH_IniciarCliente()
	{
		m_HUD = new BH_InCognitoHUD(m_sImagenIncognito);
		Print("[BH_Incognito] Cliente iniciado. HUD creado.", LogLevel.NORMAL);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Construye la caché de facciones militares desde la configuración del editor.
	//! Si no se configuró ninguna en el editor, usa las facciones estándar de Reforger.
	protected void BH_ConstruirCacheFacciones()
	{
		m_aFaccionesMilitaresCache.Clear();
		
		if (m_aFaccionesMilitaresConfig && !m_aFaccionesMilitaresConfig.IsEmpty())
		{
			// Usar la configuración del editor
			foreach (BH_InCognitoFaccionMilitar faccion : m_aFaccionesMilitaresConfig)
			{
				if (faccion && !faccion.m_sClaveFaccion.IsEmpty())
					m_aFaccionesMilitaresCache.Insert(faccion.m_sClaveFaccion);
			}
			Print("[BH_Incognito] Facciones militares desde editor: " + m_aFaccionesMilitaresCache.Count(), LogLevel.NORMAL);
		}
		else
		{
			// Valores por defecto (facciones estándar de Arma Reforger)
			m_aFaccionesMilitaresCache.Insert("US");
			m_aFaccionesMilitaresCache.Insert("USSR");
			m_aFaccionesMilitaresCache.Insert("FIA");
			m_aFaccionesMilitaresCache.Insert("BLUFOR");
			m_aFaccionesMilitaresCache.Insert("OPFOR");
			m_aFaccionesMilitaresCache.Insert("INDFOR");
			Print("[BH_Incognito] Facciones militares: usando valores por defecto (6 facciones)", LogLevel.NORMAL);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Construye el mapa de lookup rápido a partir de la lista de items forzados del editor
	protected void BH_ConstruirMapaItemsForzados()
	{
		if (!m_aItemsForzados)
			return;
			
		foreach (BH_InCognitoItemForzado item : m_aItemsForzados)
		{
			if (item && item.m_sPrefab != ResourceName.Empty)
				m_mItemsForzadosMap.Insert(item.m_sPrefab, item.m_bEsMilitar);
		}
		
		Print("[BH_Incognito] Items forzados cargados: " + m_mItemsForzadosMap.Count(), LogLevel.NORMAL);
	}
	
	//---------------------------------------------------------------------
	// LÓGICA PRINCIPAL — DETECCIÓN DE ESTADO (solo servidor)
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Comprueba el estado de todos los jugadores conectados.
	//! Se ejecuta periódicamente via CallLater en el servidor.
	protected void BH_ComprobarTodosLosJugadores()
	{
		array<int> jugadoresIds = {};
		GetGame().GetPlayerManager().GetAllPlayers(jugadoresIds);
		
		foreach (int playerId : jugadoresIds)
		{
			IEntity jugador = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (!jugador)
				continue;
			
			// Primera vez que vemos a este jugador: guardamos su facción original
			if (!m_mEstadosJugadores.Contains(playerId))
				BH_GuardarFaccionOriginal(jugador, playerId);
			
			BH_EInCognitoEstado estadoAnterior = BH_EInCognitoEstado.INCOGNITO;
			if (m_mEstadosJugadores.Contains(playerId))
				estadoAnterior = m_mEstadosJugadores.Get(playerId);
			
			BH_EInCognitoEstado estadoActual;
			
			// Si hay un override activo, usamos ese estado en lugar del calculado por ropa
			if (m_mOverrideEstado.Contains(playerId))
			{
				estadoActual = m_mOverrideEstado.Get(playerId);
			}
			else
			{
				estadoActual = BH_CalcularEstadoJugador(jugador);
			}
			
			// Solo actuamos si el estado ha cambiado
			if (estadoActual != estadoAnterior)
			{
				// INCOGNITO → COMPROMETIDO con tiempo de gracia
				if (estadoActual == BH_EInCognitoEstado.COMPROMETIDO && m_fTiempoGracia > 0 && !m_aEnGracia.Contains(playerId))
				{
					m_aEnGracia.Insert(playerId);
					GetGame().GetCallqueue().CallLater(BH_AplicarComprobado, m_fTiempoGracia * 1000, false, playerId);
				}
				// COMPROMETIDO → INCOGNITO: inmediato, y cancelamos gracia si estaba pendiente
				else if (estadoActual == BH_EInCognitoEstado.INCOGNITO)
				{
					m_aEnGracia.RemoveItem(playerId);
					m_mEstadosJugadores.Set(playerId, estadoActual);
					BH_AplicarCambioEstado(jugador, playerId, estadoActual);
				}
				// Sin tiempo de gracia configurado: aplicar inmediatamente
				else if (!m_aEnGracia.Contains(playerId))
				{
					m_mEstadosJugadores.Set(playerId, estadoActual);
					BH_AplicarCambioEstado(jugador, playerId, estadoActual);
				}
			}
			else
			{
				// El estado natural vuelve a INCOGNITO durante el periodo de gracia: cancelamos
				if (estadoActual == BH_EInCognitoEstado.INCOGNITO && m_aEnGracia.Contains(playerId))
				{
					m_aEnGracia.RemoveItem(playerId);
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Calcula el estado de incógnito de un jugador concreto
	protected BH_EInCognitoEstado BH_CalcularEstadoJugador(IEntity jugador)
	{
		if (BH_TieneArmaActiva(jugador))
			return BH_EInCognitoEstado.COMPROMETIDO;
		
		if (BH_LlevaRopaMilitar(jugador))
			return BH_EInCognitoEstado.COMPROMETIDO;
		
		return BH_EInCognitoEstado.INCOGNITO;
	}
	
	//---------------------------------------------------------------------
	// DETECCIÓN DE ARMAS
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Comprueba si el jugador tiene un arma en mano o equipada en algún slot de arma.
	//! Las armas dentro de mochilas o bolsillos de ropa NO se detectan.
	protected bool BH_TieneArmaActiva(IEntity jugador)
	{
		// 1. Comprobar arma actualmente en la mano
		SCR_CharacterControllerComponent controlador = SCR_CharacterControllerComponent.Cast(
			jugador.FindComponent(SCR_CharacterControllerComponent)
		);
		if (controlador)
		{
			IEntity itemMano = controlador.GetItemInHandSlot();
			if (itemMano && BH_EsArma(itemMano))
				return true;
		}
		
		// 2. Comprobar todos los slots de arma equipada (primaria, secundaria, etc.)
		BaseWeaponManagerComponent gestorArmas = BaseWeaponManagerComponent.Cast(
			jugador.FindComponent(BaseWeaponManagerComponent)
		);
		if (gestorArmas)
		{
			array<WeaponSlotComponent> slotsArmas = {};
			gestorArmas.GetWeaponsSlots(slotsArmas);
			
			foreach (WeaponSlotComponent slot : slotsArmas)
			{
				if (!slot)
					continue;
				
				// Ignoramos el slot de mano (ya comprobado arriba)
				if (CharacterHandWeaponSlotComponent.Cast(slot))
					continue;
				
				if (slot.GetWeaponEntity())
					return true;
			}
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool BH_EsArma(IEntity entidad)
	{
		if (!entidad)
			return false;
		return (entidad.FindComponent(BaseWeaponComponent) != null);
	}
	
	//---------------------------------------------------------------------
	// DETECCIÓN DE ROPA
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Comprueba si el jugador lleva ropa militar en los slots de equipamiento del personaje.
	protected bool BH_LlevaRopaMilitar(IEntity jugador)
	{
		CharacterInventoryStorageComponent charStorage = CharacterInventoryStorageComponent.Cast(
			jugador.FindComponent(CharacterInventoryStorageComponent)
		);
		if (!charStorage)
			return false;
		
		int numSlots = charStorage.GetSlotsCount();
		for (int i = 0; i < numSlots; i++)
		{
			InventoryStorageSlot slot = charStorage.GetSlot(i);
			if (!slot)
				continue;
			
			IEntity prenda = slot.GetAttachedEntity();
			if (!prenda)
				continue;
			
			// Solo evaluamos prendas de ropa (BaseLoadoutClothComponent)
			if (!prenda.FindComponent(BaseLoadoutClothComponent))
				continue;
			
			if (BH_EsPrendaMilitar(prenda))
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Determina si una prenda es militar.
	//! Prioridad: 1) Items forzados del editor, 2) SCR_ItemOutfitFactionComponent
	protected bool BH_EsPrendaMilitar(IEntity prenda)
	{
		if (!prenda)
			return false;
		
		// 1. Comprobar si está en la lista de items con clasificación forzada (editor)
		EntityPrefabData prefabData = prenda.GetPrefabData();
		if (prefabData)
		{
			ResourceName prefabRN = prefabData.GetPrefab();
			if (m_mItemsForzadosMap.Contains(prefabRN))
				return m_mItemsForzadosMap.Get(prefabRN);
		}
		
		// 2. Detección automática via SCR_ItemOutfitFactionComponent
		SCR_ItemOutfitFactionComponent outfitComp = SCR_ItemOutfitFactionComponent.Cast(
			prenda.FindComponent(SCR_ItemOutfitFactionComponent)
		);
		if (!outfitComp || !outfitComp.IsValid())
			return false;
		
		array<SCR_OutfitFactionData> outfitValues = {};
		outfitComp.GetOutfitFactionDataArray(outfitValues);
		
		foreach (SCR_OutfitFactionData data : outfitValues)
		{
			string factionKey = data.GetAffiliatedFactionKey();
			foreach (string faccionMilitar : m_aFaccionesMilitaresCache)
			{
				if (factionKey == faccionMilitar)
					return true;
			}
		}
		
		return false;
	}
	
	//---------------------------------------------------------------------
	// APLICACIÓN DE CAMBIOS DE ESTADO
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Aplica todos los efectos derivados de un cambio de estado de un jugador
	protected void BH_AplicarCambioEstado(IEntity jugador, int playerId, BH_EInCognitoEstado nuevoEstado)
	{
		// Efecto sobre la IA (solo servidor)
		if (m_bAfectarIA)
			BH_AplicarEfectoIA(jugador, playerId, nuevoEstado);
		
		// Notificamos al cliente correspondiente via RPC para que actualice su HUD
		BH_RPC_ActualizarHUD(playerId, nuevoEstado);
		
		// Disparamos el invoker para que otros scripts suscritos sean notificados
		if (m_OnEstadoCambiado)
			m_OnEstadoCambiado.Invoke(playerId, nuevoEstado);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Guarda la facción original de un jugador antes de modificarla
	protected void BH_GuardarFaccionOriginal(IEntity jugador, int playerId)
	{
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(
			jugador.FindComponent(FactionAffiliationComponent)
		);
		if (!factionComp)
			return;
		
		Faction faccionActual = factionComp.GetAffiliatedFaction();
		if (faccionActual)
			m_mFaccionesOriginales.Set(playerId, faccionActual.GetFactionKey());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Modifica la facción del jugador para afectar cómo la IA lo percibe
	protected void BH_AplicarEfectoIA(IEntity jugador, int playerId, BH_EInCognitoEstado estado)
	{
		FactionAffiliationComponent factionComp = FactionAffiliationComponent.Cast(
			jugador.FindComponent(FactionAffiliationComponent)
		);
		if (!factionComp)
			return;
		
		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;
		
		if (estado == BH_EInCognitoEstado.INCOGNITO)
		{
			Faction faccionCivil = factionManager.GetFactionByKey(m_sClaveFaccionCivil);
			if (faccionCivil)
			{
				factionComp.SetAffiliatedFaction(faccionCivil);
				Print("[BH_Incognito] Jugador " + playerId + " -> facción " + m_sClaveFaccionCivil + " (incógnito)", LogLevel.NORMAL);
			}
			else
			{
				Print("[BH_Incognito] AVISO: No se encontró la facción '" + m_sClaveFaccionCivil + "'. Revisa el FactionManager.", LogLevel.WARNING);
			}
		}
		else
		{
			// Restaurar facción original
			if (m_mFaccionesOriginales.Contains(playerId))
			{
				string claveOriginal = m_mFaccionesOriginales.Get(playerId);
				Faction faccionOriginal = factionManager.GetFactionByKey(claveOriginal);
				if (faccionOriginal)
				{
					factionComp.SetAffiliatedFaction(faccionOriginal);
					Print("[BH_Incognito] Jugador " + playerId + " -> facción " + claveOriginal + " (comprometido)", LogLevel.NORMAL);
				}
			}
		}
	}
	
	//---------------------------------------------------------------------
	// SISTEMA RPC — COMUNICACIÓN SERVIDOR -> CLIENTE
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Envía el nuevo estado a todos los clientes via broadcast.
	//! Cada cliente filtra en BH_RPC_Recibir si el mensaje es para él.
	//! En partida local (sin replicación activa) actualiza directamente.
	protected void BH_RPC_ActualizarHUD(int playerId, BH_EInCognitoEstado nuevoEstado)
	{
		int rpcEstado = nuevoEstado;
		
		if (Replication.IsRunning())
		{
			// Multijugador: broadcast, el cliente filtra por playerId en BH_RPC_Recibir
			Rpc(BH_RPC_Recibir, playerId, rpcEstado);
		}
		else
		{
			// Partida local sin replicación: actualizamos el HUD directamente
			BH_AplicarEstadoEnCliente(rpcEstado);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Receptor RPC que se ejecuta en cada cliente.
	//! Cada cliente comprueba si el playerId es el suyo antes de actualizar el HUD.
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void BH_RPC_Recibir(int playerId, int estado)
	{
		// Comprobamos si este RPC es para el jugador local
		PlayerController localController = GetGame().GetPlayerController();
		if (!localController)
			return;
		
		IEntity localEntity = localController.GetControlledEntity();
		if (!localEntity)
			return;
		
		int localPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(localEntity);
		
		// Solo actualizamos el HUD si el mensaje es para este jugador
		if (localPlayerId != playerId)
			return;
		
		BH_AplicarEstadoEnCliente(estado);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Actualiza el HUD local con el estado recibido del servidor
	protected void BH_AplicarEstadoEnCliente(int estado)
	{
		if (!m_HUD)
			return;
		
		m_HUD.BH_MostrarEstado(estado);
	}
	
	//---------------------------------------------------------------------
	// LIMPIEZA
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	protected void BH_Limpiar()
	{
		GetGame().GetCallqueue().Remove(BH_ComprobarTodosLosJugadores);
		
		if (m_HUD)
		{
			m_HUD.BH_Destruir();
			m_HUD = null;
		}
		
		m_mEstadosJugadores.Clear();
		m_mFaccionesOriginales.Clear();
		m_mOverrideEstado.Clear();
		m_mOverridePermanente.Clear();
		m_aEnGracia.Clear();
		
		Print("[BH_Incognito] Componente destruido y recursos liberados", LogLevel.NORMAL);
	}
	
	//---------------------------------------------------------------------
	// API PÚBLICA — para uso desde otros scripts de misión
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Devuelve el estado actual de incógnito de un jugador
	BH_EInCognitoEstado BH_GetEstadoJugador(int playerId)
	{
		if (m_mEstadosJugadores.Contains(playerId))
			return m_mEstadosJugadores.Get(playerId);
		return BH_EInCognitoEstado.INCOGNITO;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Devuelve true si el jugador indicado está actualmente en modo incógnito
	bool BH_EstaEnIncognito(int playerId)
	{
		return BH_GetEstadoJugador(playerId) == BH_EInCognitoEstado.INCOGNITO;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Devuelve la lista de facciones militares activa (para depuración o scripts externos)
	array<string> BH_GetFaccionesMilitares()
	{
		return m_aFaccionesMilitaresCache;
	}
	
	//---------------------------------------------------------------------
	// TIEMPO DE GRACIA
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Callback que se ejecuta al terminar el tiempo de gracia.
	//! Comprueba si el jugador sigue siendo COMPROMETIDO antes de aplicar el cambio.
	protected void BH_AplicarComprobado(int playerId)
	{
		m_aEnGracia.RemoveItem(playerId);
		
		IEntity jugador = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!jugador)
			return;
		
		// Recalculamos por si el jugador reaccionó a tiempo durante la gracia
		BH_EInCognitoEstado estadoActual;
		if (m_mOverrideEstado.Contains(playerId))
			estadoActual = m_mOverrideEstado.Get(playerId);
		else
			estadoActual = BH_CalcularEstadoJugador(jugador);
		
		// Solo aplicamos si sigue siendo COMPROMETIDO
		if (estadoActual == BH_EInCognitoEstado.COMPROMETIDO)
		{
			m_mEstadosJugadores.Set(playerId, estadoActual);
			BH_AplicarCambioEstado(jugador, playerId, estadoActual);
		}
	}
	
	//---------------------------------------------------------------------
	// SISTEMA DE OVERRIDE DE ESTADO
	//---------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	//! Fuerza el estado de un jugador ignorando la detección automática por ropa.
	//! \param playerId		ID del jugador a afectar
	//! \param estado		Estado a forzar (INCOGNITO o COMPROMETIDO)
	//! \param permanente	Si true, el override no expira nunca
	//! \param duracion		Segundos hasta que el override expira (0 = permanente)
	void BH_SetOverrideEstado(int playerId, BH_EInCognitoEstado estado, bool permanente, float duracion)
	{
		m_mOverrideEstado.Set(playerId, estado);
		m_mOverridePermanente.Set(playerId, permanente);
		
		// Aplicamos el cambio inmediatamente sin esperar al próximo tick
		IEntity jugador = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (jugador)
		{
			m_mEstadosJugadores.Set(playerId, estado);
			BH_AplicarCambioEstado(jugador, playerId, estado);
		}
		
		// Si no es permanente, programamos la expiración
		if (!permanente && duracion > 0)
		{
			GetGame().GetCallqueue().CallLater(BH_ExpirarOverride, duracion * 1000, false, playerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	//! Elimina el override de estado de un jugador, volviendo a la detección automática por ropa.
	void BH_ClearOverrideEstado(int playerId)
	{
		m_mOverrideEstado.Remove(playerId);
		m_mOverridePermanente.Remove(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Elimina el override de todos los jugadores.
	void BH_ClearTodosLosOverrides()
	{
		m_mOverrideEstado.Clear();
		m_mOverridePermanente.Clear();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Devuelve true si el jugador tiene un override de estado activo
	bool BH_TieneOverride(int playerId)
	{
		return m_mOverrideEstado.Contains(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Callback que se ejecuta cuando expira un override temporal.
	//! Elimina el override y deja que la detección automática vuelva a funcionar.
	protected void BH_ExpirarOverride(int playerId)
	{
		if (Replication.IsRunning() && !Replication.IsServer())
			return;

		if (!m_mOverrideEstado.Contains(playerId))
			return;
		
		// Si es permanente no expiramos (no debería llegar aquí, pero por seguridad)
		if (m_mOverridePermanente.Contains(playerId) && m_mOverridePermanente.Get(playerId))
			return;
		
		Print("[BH_Incognito] Override expirado para jugador " + playerId + ". Volviendo a detección automática.", LogLevel.NORMAL);
		
		BH_ClearOverrideEstado(playerId);
		
		// Cancelamos cualquier gracia pendiente
		m_aEnGracia.RemoveItem(playerId);
		
		// Forzamos una comprobación inmediata para actualizar el estado por ropa
		IEntity jugador = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!jugador)
			return;
		
		BH_EInCognitoEstado estadoNatural = BH_CalcularEstadoJugador(jugador);
		
		// Borramos el estado guardado para forzar que la próxima comprobación lo aplique
		m_mEstadosJugadores.Remove(playerId);
		
		// Aplicamos directamente sin pasar por el tiempo de gracia
		m_mEstadosJugadores.Set(playerId, estadoNatural);
		BH_AplicarCambioEstado(jugador, playerId, estadoNatural);
	}
}