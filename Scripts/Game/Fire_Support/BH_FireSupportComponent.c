// ============================================================================
// BH_FireSupportComponent.c
//
// Descripción:
//   Componente núcleo del sistema de Soporte de Fuego por Radio.
//   Se añade al prefab de la radio de mochila (largo alcance).
//
//   Gestiona en servidor:
//     - Bloqueo global (solo 1 solicitud activa simultánea)
//     - Cooldowns independientes por tipo de soporte
//     - Recepción de solicitudes vía RPC desde clientes
//     - Resolución de zona de spawn (la más lejana al combate)
//     - Dispatch al subsistema correspondiente (artillería / heli)
//
//   El HUD se gestiona en BH_FireSupportHUD, que se abre automáticamente
//   cuando el jugador abre el mapa y lleva la radio equipada.
//
// Autor: Bhelma
//
// Dónde se añade:
//   Prefab de la radio de mochila (largo alcance). Ejemplo:
//   Prefabs/Items/Equipment/Radios/Radio_AN_PRC_152_Backpack.et
//   NO se añade al GameMode ni al personaje.
//
// Dependencias:
//   - BH_FireSupportHUD.c
//   - BH_ArtillerySupportManager.c
//   - BH_FireSupportHelicopterManager.c
//   - BH_AmbientHelicopterControllerComponent.c
// ============================================================================

// ---------------------------------------------------------------------------
// Enumerado: tipos de soporte disponibles
// ---------------------------------------------------------------------------
enum BH_EFireSupportType
{
	ARTILLERY   = 0,    // Bombardeo de morteros
	HELI_ATTACK = 1,    // Helicóptero de ataque
	HELI_SUPPLY = 2,    // Helicóptero de suministro (caja + humo morado)
	HELI_EXTRACT = 3    // Helicóptero de extracción
}


// ---------------------------------------------------------------------------
// Clase de datos de una zona de spawn de helicóptero
// Se configura en el Workbench añadiendo entidades al array del GameMode.
// En tiempo de ejecución el servidor elige la zona más lejana al marcador.
// ---------------------------------------------------------------------------
[BaseContainerProps()]
class BH_FireSupportSpawnZone
{
	[Attribute("", UIWidgets.EditBox,
		desc: "Nombre de la entidad SCR_Position (o cualquier IEntity) colocada en el Workbench que representa esta zona de spawn/aeródromo.",
		category: "BH Fire Support - Spawn Zone")]
	string m_sEntityName;

	[Attribute("500", UIWidgets.EditBox,
		desc: "Radio en metros alrededor del punto de spawn donde puede aparecer el helicóptero.",
		params: "50 2000",
		category: "BH Fire Support - Spawn Zone")]
	float m_fSpawnRadius;
}

// ---------------------------------------------------------------------------
// Class descriptor
// ---------------------------------------------------------------------------
[EntityEditorProps(category: "BH/FireSupport/", description: "Fire Support Radio Component", color: "255 128 0 255")]
class BH_FireSupportComponentClass : ScriptComponentClass
{
}

// ---------------------------------------------------------------------------
// Componente principal
// ---------------------------------------------------------------------------
class BH_FireSupportComponent : ScriptComponent
{
	// -----------------------------------------------------------------------
	// ATRIBUTOS — Cooldowns por tipo
	// -----------------------------------------------------------------------

	[Attribute("300", UIWidgets.EditBox,
		desc: "Tiempo de enfriamiento en segundos para el soporte de ARTILLERÍA tras su uso.",
		params: "30 3600",
		category: "BH Fire Support - Cooldowns")]
	protected float m_fCooldownArtillery;

	[Attribute("600", UIWidgets.EditBox,
		desc: "Tiempo de enfriamiento en segundos para el soporte de HELICÓPTERO DE ATAQUE.",
		params: "30 3600",
		category: "BH Fire Support - Cooldowns")]
	protected float m_fCooldownHeliAttack;

	[Attribute("480", UIWidgets.EditBox,
		desc: "Tiempo de enfriamiento en segundos para el soporte de HELICÓPTERO DE SUMINISTRO.",
		params: "30 3600",
		category: "BH Fire Support - Cooldowns")]
	protected float m_fCooldownHeliSupply;

	[Attribute("600", UIWidgets.EditBox,
		desc: "Tiempo de enfriamiento en segundos para el soporte de HELICÓPTERO DE EXTRACCIÓN.",
		params: "30 3600",
		category: "BH Fire Support - Cooldowns")]
	protected float m_fCooldownHeliExtract;

	// -----------------------------------------------------------------------
	// ATRIBUTOS — Artillería
	// -----------------------------------------------------------------------

	[Attribute("45", UIWidgets.EditBox,
		desc: "Segundos de delay desde la solicitud hasta el primer impacto de artillería (simula tiempo de carga y desplazamiento).",
		params: "10 300",
		category: "BH Fire Support - Artillery")]
	protected int m_iArtilleryDelay;

	[Attribute("12", UIWidgets.EditBox,
		desc: "Número de impactos del bombardeo de artillería.",
		params: "1 50",
		category: "BH Fire Support - Artillery")]
	protected int m_iArtilleryRounds;

	[Attribute("60", UIWidgets.EditBox,
		desc: "Radio en metros del área bombardeada alrededor del marcador.",
		params: "10 300",
		category: "BH Fire Support - Artillery")]
	protected float m_fArtilleryRadius;

	[Attribute("3.5", UIWidgets.EditBox,
		desc: "Intervalo en segundos entre impactos consecutivos de artillería.",
		params: "0.5 15",
		category: "BH Fire Support - Artillery")]
	protected float m_fArtilleryImpactInterval;

	// -----------------------------------------------------------------------
	// ATRIBUTOS — Helicópteros
	// -----------------------------------------------------------------------

	[Attribute("{42A502E3BB727CEB}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_HeliPilot.et", UIWidgets.ResourcePickerThumbnail,
		desc: "Prefab del piloto de los helicópteros de soporte.",
		params: "et",
		category: "BH Fire Support - Helicopters")]
	protected ResourceName m_sPilotPrefab;

	[Attribute("{15CD521098748195}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_HeliCrew.et", UIWidgets.ResourcePickerThumbnail,
		desc: "Prefab del copiloto / artillero.",
		params: "et",
		category: "BH Fire Support - Helicopters")]
	protected ResourceName m_sCoPilotPrefab;

	[Attribute("", UIWidgets.ResourcePickerThumbnail,
		desc: "Prefab del helicóptero de ATAQUE (debe tener BH_AmbientHelicopterControllerComponent).",
		params: "et",
		category: "BH Fire Support - Helicopters")]
	protected ResourceName m_sHeliAttackPrefab;

	[Attribute("", UIWidgets.ResourcePickerThumbnail,
		desc: "Prefab del helicóptero de SUMINISTRO / EXTRACCIÓN (debe tener BH_AmbientHelicopterControllerComponent).",
		params: "et",
		category: "BH Fire Support - Helicopters")]
	protected ResourceName m_sHeliUtilityPrefab;

	[Attribute("", UIWidgets.ResourcePickerThumbnail,
		desc: "Prefab de la caja de suministros que suelta el helicóptero de suministro.",
		params: "et",
		category: "BH Fire Support - Helicopters")]
	protected ResourceName m_sSupplyBoxPrefab;

	[Attribute("3000", UIWidgets.EditBox,
		desc: "Distancia en metros a la que el helicóptero desaparece al alejarse del área de misión.",
		params: "500 10000",
		category: "BH Fire Support - Helicopters")]
	protected float m_fHeliDespawnDistance;

	// -----------------------------------------------------------------------
	// ATRIBUTOS — Zonas de spawn
	// -----------------------------------------------------------------------

	[Attribute(desc: "Lista de zonas de aeródromo/spawn disponibles para helicópteros. El servidor elegirá automáticamente la MÁS LEJANA al marcador de combate para simular tiempo de llegada realista. Añade al menos una.",
		category: "BH Fire Support - Spawn Zones")]
	protected ref array<ref BH_FireSupportSpawnZone> m_aSpawnZones;

	// -----------------------------------------------------------------------
	// ATRIBUTOS — Debug
	// -----------------------------------------------------------------------

	[Attribute(defvalue: "0",
		desc: "Activar logs de debug en consola.",
		category: "BH Debug")]
	bool m_bBH_Debug;

	// -----------------------------------------------------------------------
	// Estado interno
	// -----------------------------------------------------------------------

	// Referencia cacheada al GadgetComponent padre (la propia radio)
	protected SCR_GadgetComponent m_pGadgetComp;

	// -----------------------------------------------------------------------
	// Estado replicado — cooldowns (gestionados en servidor, leídos en cliente)
	// -----------------------------------------------------------------------

	// Tiempo Unix aproximado en que termina cada cooldown (0 = disponible)
	// Usamos float con tiempo de juego para evitar dependencia de reloj real.
	// Se replica mediante RPC de servidor a cliente al cambiar.
	[RplProp()]
	protected float m_fCooldownEndArtillery;

	[RplProp()]
	protected float m_fCooldownEndHeliAttack;

	[RplProp()]
	protected float m_fCooldownEndHeliSupply;

	[RplProp()]
	protected float m_fCooldownEndHeliExtract;

	// Bloqueo global: hay una solicitud activa en el servidor
	[RplProp()]
	protected bool m_bGlobalLocked;

	// -----------------------------------------------------------------------
	// LIFECYCLE
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Resolver GadgetComponent en el mismo prefab (la radio)
		m_pGadgetComp = SCR_GadgetComponent.Cast(owner.FindComponent(SCR_GadgetComponent));
		if (!m_pGadgetComp)
		{
			BH_Log("OnPostInit: ERROR - No se encontró SCR_GadgetComponent en la radio. Verifica el prefab.");
			return;
		}

		BH_Log("OnPostInit: componente inicializado correctamente.");
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
	}

	// -----------------------------------------------------------------------
	// API PÚBLICA — Consultas de estado (llamadas desde el HUD)
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Devuelve true si un tipo concreto está en cooldown
	bool BH_IsOnCooldown(BH_EFireSupportType type)
	{
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;

		if (type == BH_EFireSupportType.ARTILLERY)
			return (m_fCooldownEndArtillery > now);

		if (type == BH_EFireSupportType.HELI_ATTACK)
			return (m_fCooldownEndHeliAttack > now);

		if (type == BH_EFireSupportType.HELI_SUPPLY)
			return (m_fCooldownEndHeliSupply > now);

		if (type == BH_EFireSupportType.HELI_EXTRACT)
			return (m_fCooldownEndHeliExtract > now);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Devuelve los segundos restantes de cooldown para un tipo (0 si disponible)
	float BH_GetCooldownRemaining(BH_EFireSupportType type)
	{
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		float endTime = 0;

		if (type == BH_EFireSupportType.ARTILLERY)
			endTime = m_fCooldownEndArtillery;
		else if (type == BH_EFireSupportType.HELI_ATTACK)
			endTime = m_fCooldownEndHeliAttack;
		else if (type == BH_EFireSupportType.HELI_SUPPLY)
			endTime = m_fCooldownEndHeliSupply;
		else if (type == BH_EFireSupportType.HELI_EXTRACT)
			endTime = m_fCooldownEndHeliExtract;

		float remaining = endTime - now;
		if (remaining < 0)
			remaining = 0;

		return remaining;
	}

	//------------------------------------------------------------------------------------------------
	// Devuelve true si el sistema está bloqueado globalmente (hay solicitud activa)
	bool BH_IsGlobalLocked()
	{
		return m_bGlobalLocked;
	}

	// -----------------------------------------------------------------------
	// API PÚBLICA — Solicitar soporte (llamadas desde el HUD tras selección)
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// El cliente llama a este método con las coordenadas del marcador en el mapa.
	// Internamente envía RPC al servidor.
	void BH_RequestSupport(BH_EFireSupportType type, vector targetPos)
	{
		if (BH_IsGlobalLocked())
		{
			BH_Log("BH_RequestSupport: solicitud rechazada — sistema bloqueado globalmente.");
			return;
		}

		if (BH_IsOnCooldown(type))
		{
			BH_Log("BH_RequestSupport: solicitud rechazada — tipo en cooldown.");
			return;
		}

		BH_Log("BH_RequestSupport: enviando solicitud tipo=" + type + " pos=" + targetPos);

		// Obtener el ID de replicación del portador para que el servidor
		// sepa quién hace la solicitud
		RplComponent rplComp = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		RplId radioRplId;
		if (rplComp)
			radioRplId = rplComp.Id();

		Rpc(RPC_ServerRequestSupport, type, targetPos, radioRplId);
	}

	//------------------------------------------------------------------------------------------------
	// Para la extracción: el jugador ya está en el heli y da la orden de partir.
	// Se envía la posición destino al servidor.
	void BH_RequestExtractionDepart(vector destinationPos)
	{
		BH_Log("BH_RequestExtractionDepart: enviando destino=" + destinationPos);
		Rpc(RPC_ServerExtractionDepart, destinationPos);
	}

	// -----------------------------------------------------------------------
	// RPC — Cliente → Servidor
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_ServerRequestSupport(BH_EFireSupportType type, vector targetPos, RplId radioRplId)
	{
		BH_Log("RPC_ServerRequestSupport: recibido tipo=" + type + " pos=" + targetPos);

		// Validaciones en servidor
		if (m_bGlobalLocked)
		{
			BH_Log("RPC_ServerRequestSupport: rechazado — bloqueo global activo.");
			return;
		}

		if (BH_IsOnCooldown(type))
		{
			BH_Log("RPC_ServerRequestSupport: rechazado — tipo en cooldown.");
			return;
		}

		// Bloquear globalmente mientras se procesa
		BH_SetGlobalLocked(true);

		// Iniciar cooldown del tipo solicitado
		BH_StartCooldown(type);

		// Resolver zona de spawn (la más lejana al objetivo)
		vector spawnPos = BH_ResolveSpawnZone(targetPos);

		// Despachar al subsistema correcto
		if (type == BH_EFireSupportType.ARTILLERY)
		{
			BH_DispatchArtillery(targetPos, spawnPos);
		}
		else if (type == BH_EFireSupportType.HELI_ATTACK)
		{
			BH_DispatchHeliAttack(targetPos, spawnPos);
		}
		else if (type == BH_EFireSupportType.HELI_SUPPLY)
		{
			BH_DispatchHeliSupply(targetPos, spawnPos);
		}
		else if (type == BH_EFireSupportType.HELI_EXTRACT)
		{
			BH_DispatchHeliExtract(targetPos, spawnPos, radioRplId);
		}

		// Notificar a todos los clientes para que coloquen el marcador en el mapa
		Rpc(RPC_ClientPlaceMarker, type, targetPos);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_ServerExtractionDepart(vector destinationPos)
	{
		BH_Log("RPC_ServerExtractionDepart: destino=" + destinationPos);

		// Notificar al subsistema de helicópteros que el heli de extracción
		// debe partir hacia el destino indicado.
		// BH_HelicopterSupport lo gestiona (script 4).
		BH_FireSupportHelicopterManager heliMgr = BH_FireSupportHelicopterManager.GetInstance();
		if (heliMgr)
			heliMgr.BH_SetExtractionDestination(destinationPos);
	}

	// -----------------------------------------------------------------------
	// RPC — Servidor → Cliente (sincronización de cooldowns)
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Notifica a todos los clientes el estado actualizado de cooldowns
	protected void BH_BroadcastCooldownState()
	{
		Rpc(RPC_ClientUpdateCooldowns,
			m_fCooldownEndArtillery,
			m_fCooldownEndHeliAttack,
			m_fCooldownEndHeliSupply,
			m_fCooldownEndHeliExtract,
			m_bGlobalLocked);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ClientUpdateCooldowns(
		float endArtillery,
		float endHeliAttack,
		float endHeliSupply,
		float endHeliExtract,
		bool globalLocked)
	{
		m_fCooldownEndArtillery  = endArtillery;
		m_fCooldownEndHeliAttack = endHeliAttack;
		m_fCooldownEndHeliSupply = endHeliSupply;
		m_fCooldownEndHeliExtract = endHeliExtract;
		m_bGlobalLocked          = globalLocked;

		BH_Log("RPC_ClientUpdateCooldowns: estado de cooldowns actualizado.");
	}

	// -----------------------------------------------------------------------
	// LÓGICA DE SERVIDOR — Cooldowns y bloqueo
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_SetGlobalLocked(bool locked)
	{
		m_bGlobalLocked = locked;
		BH_BroadcastCooldownState();
	}

	//------------------------------------------------------------------------------------------------
	// Inicia el cooldown para el tipo indicado y programa su finalización
	protected void BH_StartCooldown(BH_EFireSupportType type)
	{
		float now = GetGame().GetWorld().GetWorldTime() * 0.001;
		float duration = 0;

		if (type == BH_EFireSupportType.ARTILLERY)
		{
			duration = m_fCooldownArtillery;
			m_fCooldownEndArtillery = now + duration;
		}
		else if (type == BH_EFireSupportType.HELI_ATTACK)
		{
			duration = m_fCooldownHeliAttack;
			m_fCooldownEndHeliAttack = now + duration;
		}
		else if (type == BH_EFireSupportType.HELI_SUPPLY)
		{
			duration = m_fCooldownHeliSupply;
			m_fCooldownEndHeliSupply = now + duration;
		}
		else if (type == BH_EFireSupportType.HELI_EXTRACT)
		{
			duration = m_fCooldownHeliExtract;
			m_fCooldownEndHeliExtract = now + duration;
		}

		BH_Log("BH_StartCooldown: tipo=" + type + " duración=" + duration + "s");
		BH_BroadcastCooldownState();
	}

	//------------------------------------------------------------------------------------------------
	// Libera el bloqueo global (llamado por los subsistemas cuando terminan)
	void BH_ReleaseGlobalLock()
	{
		BH_Log("BH_ReleaseGlobalLock: liberando bloqueo global.");
		BH_SetGlobalLocked(false);
	}

	// -----------------------------------------------------------------------
	// LÓGICA DE SERVIDOR — Resolución de zona de spawn
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	// Devuelve la posición de la zona de spawn más LEJANA al objetivo.
	// Simula que los refuerzos vienen de una base alejada del frente.
	protected vector BH_ResolveSpawnZone(vector targetPos)
	{
		if (!m_aSpawnZones || m_aSpawnZones.Count() == 0)
		{
			BH_Log("BH_ResolveSpawnZone: ADVERTENCIA — no hay zonas de spawn configuradas. Usando posición del objetivo + 2km norte.");
			return targetPos + Vector(0, 0, 2000);
		}

		float maxDist = -1;
		vector bestPos = targetPos + Vector(0, 0, 2000); // fallback

		foreach (BH_FireSupportSpawnZone zone : m_aSpawnZones)
		{
			if (!zone || zone.m_sEntityName == "")
				continue;

			IEntity zoneEnt = GetGame().GetWorld().FindEntityByName(zone.m_sEntityName);
			if (!zoneEnt)
			{
				BH_Log("BH_ResolveSpawnZone: no se encontró entidad '" + zone.m_sEntityName + "'.");
				continue;
			}

			vector zonePos = zoneEnt.GetOrigin();
			float dist = vector.DistanceXZ(zonePos, targetPos);

			if (dist > maxDist)
			{
				maxDist = dist;
				// Añadir un pequeño offset aleatorio dentro del radio configurado
				float angle = Math.RandomFloat(0, 360);
				float radius = Math.RandomFloat(0, zone.m_fSpawnRadius);
				float offsetX = Math.Sin(angle * Math.DEG2RAD) * radius;
				float offsetZ = Math.Cos(angle * Math.DEG2RAD) * radius;
				bestPos = Vector(zonePos[0] + offsetX, zonePos[1], zonePos[2] + offsetZ);
			}
		}

		BH_Log("BH_ResolveSpawnZone: zona elegida dist=" + maxDist + " pos=" + bestPos);
		return bestPos;
	}

	// -----------------------------------------------------------------------
	// DISPATCH — Llama al subsistema correspondiente
	// (los subsistemas se implementan en los próximos scripts)
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_DispatchArtillery(vector targetPos, vector spawnPos)
	{
		BH_Log("BH_DispatchArtillery: target=" + targetPos + " delay=" + m_iArtilleryDelay + "s");

		// BH_ArtillerySupport.c gestionará los impactos.
		// Pasamos todos los parámetros necesarios.
		BH_ArtillerySupportManager artMgr = BH_ArtillerySupportManager.GetInstance();
		if (!artMgr)
		{
			BH_Log("BH_DispatchArtillery: ERROR — BH_ArtillerySupportManager no disponible.");
			BH_ReleaseGlobalLock();
			return;
		}

		artMgr.BH_LaunchMission(
			targetPos,
			m_iArtilleryDelay,
			m_iArtilleryRounds,
			m_fArtilleryRadius,
			m_fArtilleryImpactInterval,
			this
		);
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_DispatchHeliAttack(vector targetPos, vector spawnPos)
	{
		BH_Log("BH_DispatchHeliAttack: target=" + targetPos + " spawn=" + spawnPos);

		BH_FireSupportHelicopterManager heliMgr = BH_FireSupportHelicopterManager.GetInstance();
		if (!heliMgr)
		{
			BH_Log("BH_DispatchHeliAttack: ERROR — BH_FireSupportHelicopterManager no disponible.");
			BH_ReleaseGlobalLock();
			return;
		}

		heliMgr.BH_LaunchAttack(
			targetPos,
			spawnPos,
			m_sHeliAttackPrefab,
			m_sPilotPrefab,
			m_sCoPilotPrefab,
			m_fHeliDespawnDistance,
			this
		);
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_DispatchHeliSupply(vector targetPos, vector spawnPos)
	{
		BH_Log("BH_DispatchHeliSupply: target=" + targetPos + " spawn=" + spawnPos);

		BH_FireSupportHelicopterManager heliMgr = BH_FireSupportHelicopterManager.GetInstance();
		if (!heliMgr)
		{
			BH_Log("BH_DispatchHeliSupply: ERROR — BH_FireSupportHelicopterManager no disponible.");
			BH_ReleaseGlobalLock();
			return;
		}

		heliMgr.BH_LaunchSupply(
			targetPos,
			spawnPos,
			m_sHeliUtilityPrefab,
			m_sPilotPrefab,
			m_sCoPilotPrefab,
			m_sSupplyBoxPrefab,
			m_fHeliDespawnDistance,
			this
		);
	}

	//------------------------------------------------------------------------------------------------
	protected void BH_DispatchHeliExtract(vector targetPos, vector spawnPos, RplId requesterRadioId)
	{
		BH_Log("BH_DispatchHeliExtract: target=" + targetPos + " spawn=" + spawnPos);

		BH_FireSupportHelicopterManager heliMgr = BH_FireSupportHelicopterManager.GetInstance();
		if (!heliMgr)
		{
			BH_Log("BH_DispatchHeliExtract: ERROR — BH_FireSupportHelicopterManager no disponible.");
			BH_ReleaseGlobalLock();
			return;
		}

		heliMgr.BH_LaunchExtraction(
			targetPos,
			spawnPos,
			m_sHeliUtilityPrefab,
			m_sPilotPrefab,
			m_sCoPilotPrefab,
			m_fHeliDespawnDistance,
			requesterRadioId,
			this
		);
	}

	// -----------------------------------------------------------------------
	// MARCADORES DE MAPA — Visibles para todos los jugadores
	// El servidor llama a RPC_ClientPlaceMarker tras confirmar la solicitud.
	// Cada cliente crea un marcador local con texto del tipo de soporte.
	// El marcador se borra automáticamente tras m_fMarkerDuration segundos.
	// -----------------------------------------------------------------------

	// Duración en segundos del marcador en el mapa (configurable en Workbench)
	[Attribute("300", UIWidgets.EditBox,
		desc: "Segundos que permanece visible el marcador de soporte en el mapa. 0 = permanente hasta borrado manual.",
		params: "0 3600",
		category: "BH Fire Support - Map Markers")]
	protected float m_fMarkerDuration;

	// Referencia al último marcador creado localmente (para borrarlo al expirar)
	protected ref SCR_MapMarkerBase m_pLastMarker;

	//------------------------------------------------------------------------------------------------
	// RPC Broadcast: el servidor pide a todos los clientes que coloquen el marcador
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RPC_ClientPlaceMarker(BH_EFireSupportType type, vector targetPos)
	{
		BH_Log("RPC_ClientPlaceMarker: tipo=" + type + " pos=" + targetPos);

		// En servidor dedicado no hay mapa ni UI
		if (System.IsConsoleApp())
			return;

		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
		{
			BH_Log("RPC_ClientPlaceMarker: SCR_MapMarkerManagerComponent no disponible.");
			return;
		}

		// Borrar marcador anterior si aún existía
		if (m_pLastMarker)
		{
			markerMgr.RemoveStaticMarker(m_pLastMarker);
			m_pLastMarker = null;
		}

		// Elegir el texto del marcador según el tipo de soporte
		string markerText = "";
		if (type == BH_EFireSupportType.ARTILLERY)
			markerText = "[ARTILLERIA]";
		else if (type == BH_EFireSupportType.HELI_ATTACK)
			markerText = "[HELI ATAQUE]";
		else if (type == BH_EFireSupportType.HELI_SUPPLY)
			markerText = "[SUMINISTRO]";
		else if (type == BH_EFireSupportType.HELI_EXTRACT)
			markerText = "[EXTRACCION LZ]";

		// Construir el marcador manualmente con tipo PLACED_CUSTOM
		// (marcador estático configurable colocado por el jugador)
		SCR_MapMarkerBase newMarker = new SCR_MapMarkerBase();
		newMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		newMarker.SetWorldPos((int)targetPos[0], (int)targetPos[2]);
		newMarker.SetCustomText(markerText);

		markerMgr.InsertStaticMarker(newMarker, true);

		// Recuperar la referencia al marcador recién insertado (es el último de la lista)
		array<SCR_MapMarkerBase> markers = markerMgr.GetStaticMarkers();
		if (!markers || markers.IsEmpty())
		{
			BH_Log("RPC_ClientPlaceMarker: ERROR — no se pudo recuperar el marcador creado.");
			return;
		}

		m_pLastMarker = markers[markers.Count() - 1];

		BH_Log("RPC_ClientPlaceMarker: marcador creado texto=" + markerText);

		// Programar borrado automático si la duración es mayor que 0
		if (m_fMarkerDuration > 0)
		{
			GetGame().GetCallqueue().CallLater(BH_RemoveLastMarker, (int)(m_fMarkerDuration * 1000), false);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Borra el último marcador creado (llamado por CallLater al expirar)
	protected void BH_RemoveLastMarker()
	{
		if (!m_pLastMarker)
			return;

		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerMgr)
			return;

		markerMgr.RemoveStaticMarker(m_pLastMarker);
		BH_Log("BH_RemoveLastMarker: marcador eliminado.");
		m_pLastMarker = null;
	}

	// -----------------------------------------------------------------------
	// GETTERS — Parámetros expuestos para los subsistemas
	// -----------------------------------------------------------------------

	ResourceName BH_GetHeliAttackPrefab()   { return m_sHeliAttackPrefab; }
	ResourceName BH_GetHeliUtilityPrefab()  { return m_sHeliUtilityPrefab; }
	ResourceName BH_GetPilotPrefab()        { return m_sPilotPrefab; }
	ResourceName BH_GetCoPilotPrefab()      { return m_sCoPilotPrefab; }
	ResourceName BH_GetSupplyBoxPrefab()    { return m_sSupplyBoxPrefab; }
	float        BH_GetDespawnDistance()    { return m_fHeliDespawnDistance; }
	int          BH_GetArtilleryDelay()     { return m_iArtilleryDelay; }
	int          BH_GetArtilleryRounds()    { return m_iArtilleryRounds; }
	float        BH_GetArtilleryRadius()    { return m_fArtilleryRadius; }
	float        BH_GetArtilleryInterval()  { return m_fArtilleryImpactInterval; }

	// -----------------------------------------------------------------------
	// UTILIDADES
	// -----------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void BH_Log(string msg)
	{
		if (!m_bBH_Debug)
			return;

		Print("[BH_FireSupportComponent] " + msg, LogLevel.NORMAL);
	}
}