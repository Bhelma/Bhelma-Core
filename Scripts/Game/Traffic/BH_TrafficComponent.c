// ============================================================================
// BH_TrafficComponent.c
// 
// Sistema de generación automática de tráfico para Arma Reforger.
// Detecta carreteras y poblaciones del mapa y genera vehículos estáticos
// y dinámicos (con IA) mediante streaming por proximidad al jugador.
//
// Se añade como componente al GameMode de la misión.
//
// Autor: Bhelma
// ============================================================================

//! Datos precalculados de un punto candidato a spawn
class BH_TrafficSpawnPoint
{
	vector Position;       // Posición en el mundo
	vector Direction;      // Dirección/orientación de la carretera en ese punto
	bool IsUrban;          // Está dentro de una zona poblada
	bool IsOccupied;       // Tiene un vehículo spawneado actualmente
	int AssignedVehicleID; // ID de la entidad spawneada (-1 si libre)
	bool IsStatic;         // true = vehículo estacionado, false = circulando
	
	void BH_TrafficSpawnPoint()
	{
		Position = vector.Zero;
		Direction = vector.Zero;
		IsUrban = false;
		IsOccupied = false;
		AssignedVehicleID = -1;
		IsStatic = false;
	}
};

//! Datos de un vehículo activo gestionado por el sistema
class BH_TrafficVehicleData
{
	IEntity VehicleEntity;           // Referencia a la entidad del vehículo
	BH_TrafficSpawnPoint SpawnPoint; // Punto de spawn asociado
	bool HasAI;                      // Tiene conductor IA
	IEntity DriverEntity;            // Referencia al conductor
	IEntity GroupEntity;             // Referencia al grupo IA
	IEntity WaypointEntity;          // Referencia al waypoint
	
	// Estado de comportamiento urbano
	int UrbanState;                  // Ver BH_EUrbanState
	float UrbanTimer;                // Temporizador para el estado actual
	IEntity TargetBuilding;          // Edificio destino
	vector ParkPosition;             // Posición donde aparcó
	IEntity WalkGroupEntity;         // Grupo IA temporal para caminata (separado del de conducción)
	
	void BH_TrafficVehicleData()
	{
		VehicleEntity = null;
		SpawnPoint = null;
		HasAI = false;
		DriverEntity = null;
		GroupEntity = null;
		WaypointEntity = null;
		UrbanState = 0;
		UrbanTimer = 0;
		TargetBuilding = null;
		ParkPosition = vector.Zero;
		WalkGroupEntity = null;
	}
};

// ============================================================================
// Componente principal - Añadir al GameMode
// ============================================================================
[ComponentEditorProps(category: "GameScripted/Traffic", description: "BH - Sistema de tráfico automático por proximidad. Detecta carreteras y poblaciones y genera vehículos estáticos y con IA.", icon: "WBData")]
class BH_TrafficComponentClass : ScriptComponentClass
{
};

class BH_TrafficComponent : ScriptComponent
{
	// ========================================================================
	// ATRIBUTOS CONFIGURABLES EN EL EDITOR
	// ========================================================================
	
	[Attribute(defvalue: "500", uiwidget: UIWidgets.Slider, desc: "Radio en metros alrededor de cada jugador donde se generan vehiculos. AVISO: en MP cada jugador genera trafico en su radio, valores altos con muchos jugadores pueden sobrecargar el servidor", params: "100 2000 50", category: "BH Traffic - Streaming")]
	protected float m_fActivationRadius;
	
	[Attribute(defvalue: "200", uiwidget: UIWidgets.Slider, desc: "Radio minimo en metros. Los vehiculos NO aparecen dentro de este radio (evita spawn delante del jugador)", params: "50 500 25", category: "BH Traffic - Streaming")]
	protected float m_fMinSpawnRadius;
	
	[Attribute(defvalue: "600", uiwidget: UIWidgets.Slider, desc: "Radio en metros para eliminar vehiculos (debe ser mayor que activacion para histeresis)", params: "150 2500 50", category: "BH Traffic - Streaming")]
	protected float m_fDeactivationRadius;
	
	[Attribute(defvalue: "2", uiwidget: UIWidgets.Slider, desc: "Intervalo en segundos entre chequeos de proximidad. Valores bajos = mas reactivo pero mas carga de servidor", params: "0.5 10 0.5", category: "BH Traffic - Streaming")]
	protected float m_fCheckInterval;
	
	[Attribute(defvalue: "40", uiwidget: UIWidgets.Slider, desc: "Maximo de vehiculos simultaneos en el mundo (estaticos + dinamicos). AVISO: cada vehiculo dinamico con IA consume recursos. En MP con muchos jugadores, recomendado 5-10 por jugador esperado. Ej: 50 jugadores con 100 vehiculos = carga alta de servidor. Jugadores en vehiculos aereos tambien generan trafico debajo de ellos", params: "5 200 5", category: "BH Traffic - Limits")]
	protected int m_iMaxSimultaneousVehicles;
	
	[Attribute(defvalue: "1.5", uiwidget: UIWidgets.Slider, desc: "Densidad de puntos de spawn por kilómetro de carretera", params: "0.5 10 0.5", category: "BH Traffic - Density")]
	protected float m_fDensityPerKm;
	
	[Attribute(defvalue: "50", uiwidget: UIWidgets.Slider, desc: "Porcentaje de vehículos que serán estáticos (aparcados)", params: "0 100 5", category: "BH Traffic - Distribution")]
	protected int m_iStaticPercentage;
	
	[Attribute(defvalue: "300", uiwidget: UIWidgets.Slider, desc: "Radio en metros para detectar clusters de edificios como zona urbana", params: "50 1000 50", category: "BH Traffic - Urban Detection")]
	protected float m_fUrbanDetectionRadius;
	
	[Attribute(defvalue: "5", uiwidget: UIWidgets.Slider, desc: "Número mínimo de edificios en un radio para considerar zona urbana", params: "2 20 1", category: "BH Traffic - Urban Detection")]
	protected int m_iMinBuildingsForUrban;
	
	[Attribute(desc: "Lista de prefabs de vehículos civiles para spawn", category: "BH Traffic - Prefabs")]
	protected ref array<ResourceName> m_aPrefabsVehicles;
	
	[Attribute(desc: "Lista de prefabs de personajes conductores IA", category: "BH Traffic - Prefabs")]
	protected ref array<ResourceName> m_aPrefabsDrivers;
	
	[Attribute(defvalue: "true", desc: "Activar sistema de tráfico", category: "BH Traffic - General")]
	protected bool m_bEnabled;
	
	[Attribute(defvalue: "true", desc: "Activar comportamiento urbano (conductores bajan, caminan, entran en edificios)", category: "BH Traffic - Urban Behavior")]
	protected bool m_bUrbanBehaviorEnabled;
	
	[Attribute(defvalue: "100", uiwidget: UIWidgets.Slider, desc: "Radio en metros del jugador para activar comportamiento urbano de los conductores", params: "50 300 10", category: "BH Traffic - Urban Behavior")]
	protected float m_fUrbanBehaviorRadius;
	
	[Attribute(defvalue: "30", uiwidget: UIWidgets.Slider, desc: "Tiempo mínimo en segundos que el conductor pasa dentro del edificio", params: "10 120 5", category: "BH Traffic - Urban Behavior")]
	protected float m_fBuildingStayMin;
	
	[Attribute(defvalue: "120", uiwidget: UIWidgets.Slider, desc: "Tiempo máximo en segundos que el conductor pasa dentro del edificio", params: "30 300 10", category: "BH Traffic - Urban Behavior")]
	protected float m_fBuildingStayMax;
	
	[Attribute(defvalue: "true", desc: "Mostrar mensajes de debug en consola", category: "BH Traffic - Debug")]
	protected bool m_bDebugMode;
	
	[Attribute(defvalue: "50", uiwidget: UIWidgets.Slider, desc: "Espaciado en metros del grid de muestreo para detectar carreteras. Menor = más preciso pero más lento al inicio", params: "20 200 10", category: "BH Traffic - Scanning")]
	protected float m_fScanGridSpacing;
	
	// ========================================================================
	// VARIABLES INTERNAS
	// ========================================================================
	
	// Todos los puntos candidatos precalculados
	protected ref array<ref BH_TrafficSpawnPoint> m_aSpawnPoints;
	
	// Vehículos activos actualmente en el mundo
	protected ref array<ref BH_TrafficVehicleData> m_aActiveVehicles;
	
	// Control de tiempo para chequeo periódico
	protected float m_fTimeSinceLastCheck;
	
	// Flag de inicialización completada
	protected bool m_bInitialized;
	
	// Gestor de comportamiento urbano
	protected ref BH_TrafficUrbanBehavior m_UrbanBehavior;
	
	// Variables de debug para log de materiales
	protected ref array<string> m_aDebugLoggedMaterials;
	protected int m_iDebugMaterialLogCount;
	
	// Variables para escaneo asíncrono
	protected float m_fScanCurrentX;
	protected float m_fScanCurrentZ;
	protected float m_fScanMaxX;
	protected float m_fScanMaxZ;
	protected float m_fScanMinX;
	protected float m_fScanMinZ;
	protected bool m_bScanInProgress;
	protected int m_iScanRoadPointsFound;
	protected int m_iScanTotalSamples;
	
	// ========================================================================
	// INICIALIZACIÓN
	// ========================================================================
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!m_bEnabled)
			return;
		
		// Solo ejecutar en servidor (o en SP)
		if (!BH_TrafficIsServer())
			return;
		
		m_aSpawnPoints = new array<ref BH_TrafficSpawnPoint>();
		m_aActiveVehicles = new array<ref BH_TrafficVehicleData>();
		m_fTimeSinceLastCheck = 0;
		m_bInitialized = false;
		m_UrbanBehavior = new BH_TrafficUrbanBehavior(this);
		m_aDebugLoggedMaterials = new array<string>();
		m_iDebugMaterialLogCount = 0;
		m_bScanInProgress = false;
		m_iScanRoadPointsFound = 0;
		m_iScanTotalSamples = 0;
		
		// Validar histéresis
		if (m_fDeactivationRadius <= m_fActivationRadius)
		{
			m_fDeactivationRadius = m_fActivationRadius + 100;
			BH_TrafficLog("[BH_Traffic] AVISO: Radio de desactivación ajustado a " + m_fDeactivationRadius.ToString() + "m para mantener histéresis.");
		}
		
		// Registrar para recibir EOnFrame
		SetEventMask(owner, EntityEvent.FRAME);
		
		BH_TrafficLog("[BH_Traffic] Componente inicializado. Iniciando precálculo...");
		
		// Lanzar precálculo de puntos de spawn
		GetGame().GetCallqueue().CallLater(BH_InitializeSpawnPoints, 1000, false);
	}
	
	// ========================================================================
	// PRECÁLCULO DE PUNTOS DE SPAWN (Fase 1 - Se ejecuta una vez)
	// ========================================================================
	
	//! Escanea carreteras y poblaciones para generar puntos candidatos
	protected void BH_InitializeSpawnPoints()
	{
		BH_TrafficLog("[BH_Traffic] Fase 1: Escaneando red de carreteras (asíncrono)...");
		
		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			BH_TrafficLog("[BH_Traffic] ERROR: No se pudo obtener el mundo.");
			return;
		}
		
		// Obtener límites del mapa
		vector mins, maxs;
		world.GetBoundBox(mins, maxs);
		
		m_fScanMinX = mins[0];
		m_fScanMinZ = mins[2];
		m_fScanMaxX = maxs[0];
		m_fScanMaxZ = maxs[2];
		m_fScanCurrentX = m_fScanMinX + m_fScanGridSpacing;
		m_fScanCurrentZ = m_fScanMinZ + m_fScanGridSpacing;
		
		vector center = (mins + maxs) * 0.5;
		float mapRadius = vector.Distance(mins, maxs) * 0.5;
		
		BH_TrafficLog("[BH_Traffic] Centro del mapa: " + center.ToString() + " | Radio: " + mapRadius.ToString());
		BH_TrafficLog("[BH_Traffic] Espaciado de muestreo: " + m_fScanGridSpacing.ToString() + "m");
		
		// Diagnóstico de entidades
		if (m_bDebugMode)
		{
			BH_DiagnosticEntityScan(world, center);
		}
		
		// Iniciar escaneo asíncrono
		m_bScanInProgress = true;
		m_iScanRoadPointsFound = 0;
		m_iScanTotalSamples = 0;
		
		// Procesar primer chunk
		BH_ScanNextChunk();
	}
	
	//! Procesa un chunk del grid de muestreo y programa el siguiente
	protected void BH_ScanNextChunk()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		
		int samplesThisChunk = 0;
		int maxSamplesPerChunk = 2000; // Procesar 2000 puntos por tick
		
		while (m_fScanCurrentX < m_fScanMaxX && samplesThisChunk < maxSamplesPerChunk)
		{
			while (m_fScanCurrentZ < m_fScanMaxZ && samplesThisChunk < maxSamplesPerChunk)
			{
				m_iScanTotalSamples++;
				samplesThisChunk++;
				
				if (BH_IsRoadSurface(world, m_fScanCurrentX, m_fScanCurrentZ))
				{
					float groundY = world.GetSurfaceY(m_fScanCurrentX, m_fScanCurrentZ);
					
					BH_TrafficSpawnPoint point = new BH_TrafficSpawnPoint();
					point.Position = Vector(m_fScanCurrentX, groundY, m_fScanCurrentZ);
					point.Direction = Vector(0, 0, 0);
					
					m_aSpawnPoints.Insert(point);
					m_iScanRoadPointsFound++;
				}
				
				m_fScanCurrentZ += m_fScanGridSpacing;
			}
			
			// Siguiente fila
			if (m_fScanCurrentZ >= m_fScanMaxZ)
			{
				m_fScanCurrentZ = m_fScanMinZ + m_fScanGridSpacing;
				m_fScanCurrentX += m_fScanGridSpacing;
			}
		}
		
		// ¿Terminamos?
		if (m_fScanCurrentX >= m_fScanMaxX)
		{
			// Escaneo completado
			m_bScanInProgress = false;
			BH_TrafficLog("[BH_Traffic] Grid muestreado: " + m_iScanTotalSamples.ToString() + " puntos | Carretera encontrada: " + m_iScanRoadPointsFound.ToString());
			
			// Continuar con las fases 2 y 3
			BH_FinalizeScanAndInit();
		}
		else
		{
			// Programar siguiente chunk en 50ms
			GetGame().GetCallqueue().CallLater(BH_ScanNextChunk, 50, false);
		}
	}
	
	//! Finaliza el escaneo y ejecuta las fases 2 y 3
	protected void BH_FinalizeScanAndInit()
	{
		// Refinar orientaciones
		if (m_iScanRoadPointsFound > 1)
		{
			BH_RefineSpawnPointDirections();
		}
		
		// Fase 2: Detectar zonas urbanas
		BH_TrafficLog("[BH_Traffic] Fase 2: Detectando zonas urbanas...");
		BH_DetectUrbanAreas();
		
		// Fase 3: Asignar tipos
		BH_TrafficLog("[BH_Traffic] Fase 3: Asignando tipos de vehículo...");
		BH_AssignVehicleTypes();
		
		m_bInitialized = true;
		BH_TrafficLog("[BH_Traffic] Precálculo completado. " + m_aSpawnPoints.Count().ToString() + " puntos de spawn generados.");
	}
	
	// ========================================================================
	// ESCANEO DE RED DE CARRETERAS
	// ========================================================================
	
	//! Comprueba si la superficie en una coordenada XZ es carretera (asfalto, grava, etc.)
	protected bool BH_IsRoadSurface(BaseWorld world, float x, float z)
	{
		// Hacer un trace vertical para detectar el tipo de superficie
		float surfaceY = world.GetSurfaceY(x, z);
		
		vector start = Vector(x, surfaceY + 5.0, z);
		vector end = Vector(x, surfaceY - 1.0, z);
		
		TraceParam trace = new TraceParam();
		trace.Start = start;
		trace.End = end;
		trace.Flags = TraceFlags.WORLD;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;
		
		float result = world.TraceMove(trace, null);
		
		if (result >= 1.0)
			return false;
		
		// Obtener el material de la superficie impactada
		// En Enfusion, TraceParam contiene info del material tras el trace
		GameMaterial material = trace.SurfaceProps;
		if (!material)
			return false;
		
		// Comprobar el nombre del material para detectar carreteras
		string matName = material.GetName();
		
		// En modo debug, loguear los primeros materiales únicos encontrados
		// para calibrar los filtros
		if (m_bDebugMode && m_iDebugMaterialLogCount < 30)
		{
			bool alreadyLogged = false;
			foreach (string logged : m_aDebugLoggedMaterials)
			{
				if (logged == matName)
				{
					alreadyLogged = true;
					break;
				}
			}
			if (!alreadyLogged)
			{
				m_aDebugLoggedMaterials.Insert(matName);
				m_iDebugMaterialLogCount++;
				BH_TrafficLog("[BH_Traffic] Material superficie: '" + matName + "' en " + x.ToString() + ", " + z.ToString());
			}
		}
		
		// Los materiales de carretera en Reforger suelen contener estas palabras
		string matNameLower = matName;
		matNameLower.ToLower();
		
		if (matNameLower.Contains("asphalt") ||
			matNameLower.Contains("dirt_road") ||
			matNameLower.Contains("road") ||
			matNameLower.Contains("concrete") ||
			matNameLower.Contains("gravel") ||
			matNameLower.Contains("cobble") ||
			matNameLower.Contains("pavement") ||
			matNameLower.Contains("tarmac"))
		{
			return true;
		}
		
		return false;
	}
	
	//! Refina las orientaciones de los spawn points buscando la dirección de la carretera
	protected void BH_RefineSpawnPointDirections()
	{
		int count = m_aSpawnPoints.Count();
		BaseWorld world = GetGame().GetWorld();
		
		for (int i = 0; i < count; i++)
		{
			BH_TrafficSpawnPoint current = m_aSpawnPoints[i];
			
			// Buscar el vecino más cercano que NO esté exactamente al lado en el grid
			// (para evitar orientar en diagonal por el patrón cuadrado del grid)
			// Buscamos los 2 vecinos más cercanos y calculamos la dirección entre ellos
			
			float bestDist1 = 999999;
			float bestDist2 = 999999;
			BH_TrafficSpawnPoint nearest1 = null;
			BH_TrafficSpawnPoint nearest2 = null;
			
			for (int j = 0; j < count; j++)
			{
				if (i == j)
					continue;
				
				float dist = vector.DistanceSq(current.Position, m_aSpawnPoints[j].Position);
				
				if (dist < bestDist1)
				{
					bestDist2 = bestDist1;
					nearest2 = nearest1;
					bestDist1 = dist;
					nearest1 = m_aSpawnPoints[j];
				}
				else if (dist < bestDist2)
				{
					bestDist2 = dist;
					nearest2 = m_aSpawnPoints[j];
				}
			}
			
			// Si tenemos 2 vecinos, la dirección de la carretera va de uno a otro pasando por nosotros
			vector dir;
			if (nearest1 && nearest2)
			{
				dir = nearest1.Position - nearest2.Position;
			}
			else if (nearest1)
			{
				dir = nearest1.Position - current.Position;
			}
			else
			{
				continue;
			}
			
			dir[1] = 0; // Ignorar componente vertical
			dir.Normalize();
			
			// Convertir dirección a ángulos (yaw)
			float yaw = Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG;
			current.Direction = Vector(yaw, 0, 0);
			
			// Desplazar ligeramente la posición hacia un lado de la carretera
			// para que los vehículos no estén en el centro exacto
			vector right = Vector(-dir[2], 0, dir[0]); // Perpendicular derecha
			current.Position = current.Position + right * 2.5; // 2.5m a la derecha
			
			// Ajustar Y al suelo tras el desplazamiento
			if (world)
			{
				float groundY = world.GetSurfaceY(current.Position[0], current.Position[2]);
				current.Position[1] = groundY;
			}
		}
	}
	
	//! Diagnóstico: loguea las clases de entidades cercanas al centro para análisis
	protected void BH_DiagnosticEntityScan(BaseWorld world, vector center)
	{
		BH_TrafficLog("[BH_Traffic] DIAGNÓSTICO: Escaneando entidades en radio 200m del centro...");
		m_aDiagClassNames = new array<string>();
		world.QueryEntitiesBySphere(center, 200, BH_DiagnosticCallback, null, EQueryEntitiesFlags.ALL);
		
		// Loguear clases únicas encontradas
		array<string> unique = {};
		foreach (string cn : m_aDiagClassNames)
		{
			bool found = false;
			foreach (string u : unique)
			{
				if (u == cn)
				{
					found = true;
					break;
				}
			}
			if (!found)
				unique.Insert(cn);
		}
		
		BH_TrafficLog("[BH_Traffic] DIAGNÓSTICO: " + m_aDiagClassNames.Count().ToString() + " entidades, " + unique.Count().ToString() + " clases únicas:");
		foreach (string className : unique)
		{
			BH_TrafficLog("[BH_Traffic]   - " + className);
		}
	}
	
	// Array temporal para el diagnóstico
	protected ref array<string> m_aDiagClassNames;
	
	//! Callback de diagnóstico
	protected bool BH_DiagnosticCallback(IEntity entity)
	{
		if (entity && m_aDiagClassNames)
		{
			m_aDiagClassNames.Insert(entity.ClassName());
		}
		return true;
	}
	
	// ========================================================================
	// DETECCIÓN DE ZONAS URBANAS
	// ========================================================================
	
	//! Marca los spawn points que están dentro de zonas con alta densidad de edificios
	protected void BH_DetectUrbanAreas()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		
		int urbanCount = 0;
		
		foreach (BH_TrafficSpawnPoint point : m_aSpawnPoints)
		{
			int buildingCount = BH_CountBuildingsInRadius(world, point.Position, m_fUrbanDetectionRadius);
			
			if (buildingCount >= m_iMinBuildingsForUrban)
			{
				point.IsUrban = true;
				urbanCount++;
			}
		}
		
		BH_TrafficLog("[BH_Traffic] Puntos en zona urbana: " + urbanCount.ToString() + " de " + m_aSpawnPoints.Count().ToString());
	}
	
	//! Cuenta edificios en un radio alrededor de una posición
	protected int BH_CountBuildingsInRadius(BaseWorld world, vector center, float radius)
	{
		int count = 0;
		
		// QueryEntitiesBySphere con callback que filtra edificios
		world.QueryEntitiesBySphere(center, radius, BH_BuildingCountCallback, null, EQueryEntitiesFlags.STATIC);
		
		return m_iTempBuildingCount;
	}
	
	// Variable temporal para el callback de conteo (los callbacks no pueden devolver valores)
	protected int m_iTempBuildingCount;
	
	//! Callback para contar edificios
	protected bool BH_BuildingCountCallback(IEntity entity)
	{
		if (!entity)
			return true;
		
		string className = entity.ClassName();
		
		// Detectar edificios por su clase
		if (className.Contains("Building") || className.Contains("House") || className.Contains("Structure"))
		{
			m_iTempBuildingCount++;
		}
		
		return true; // Continuar iterando
	}
	
	// ========================================================================
	// ASIGNACIÓN DE TIPOS
	// ========================================================================
	
	//! Log de información sobre los puntos generados (ya no pre-asigna tipo, se decide al spawn)
	protected void BH_AssignVehicleTypes()
	{
		int totalPoints = m_aSpawnPoints.Count();
		if (totalPoints == 0)
			return;
		
		int urbanPoints = 0;
		foreach (BH_TrafficSpawnPoint point : m_aSpawnPoints)
		{
			if (point.IsUrban)
				urbanPoints++;
		}
		
		BH_TrafficLog("[BH_Traffic] Puntos totales: " + totalPoints.ToString() + " | Urbanos (aptos para estáticos): " + urbanPoints.ToString());
	}
	
	// ========================================================================
	// LOOP PRINCIPAL - STREAMING POR PROXIMIDAD
	// ========================================================================
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bEnabled || !m_bInitialized)
			return;
		
		// Acumular tiempo
		m_fTimeSinceLastCheck += timeSlice;
		
		if (m_fTimeSinceLastCheck < m_fCheckInterval)
			return;
		
		m_fTimeSinceLastCheck = 0;
		
		// Ejecutar ciclo de streaming
		BH_UpdateTrafficStreaming();
	}
	
	//! Ciclo principal de streaming: spawn/despawn según proximidad a jugadores
	protected void BH_UpdateTrafficStreaming()
	{
		// Obtener posiciones de todos los jugadores
		array<vector> playerPositions = {};
		BH_GetAllPlayerPositions(playerPositions);
		
		if (playerPositions.IsEmpty())
			return;
		
		// --- FASE DESPAWN: Eliminar vehículos fuera de rango ---
		BH_DespawnOutOfRange(playerPositions);
		
		// --- FASE SPAWN: Activar puntos dentro de rango ---
		BH_SpawnInRange(playerPositions);
		
		// --- FASE STUCK: Detectar vehículos dinámicos parados y reasignar waypoint ---
		BH_CheckStuckVehicles();
		
		// --- FASE URBAN: Comportamiento urbano ---
		if (m_bUrbanBehaviorEnabled && m_UrbanBehavior)
		{
			m_UrbanBehavior.BH_UpdateUrbanBehavior(m_aActiveVehicles, playerPositions, m_fUrbanBehaviorRadius, m_fBuildingStayMin, m_fBuildingStayMax, m_fCheckInterval, m_bDebugMode);
		}
	}
	
	//! Detecta vehículos dinámicos que llevan parados mucho tiempo y les reasigna waypoint
	protected void BH_CheckStuckVehicles()
	{
		foreach (BH_TrafficVehicleData data : m_aActiveVehicles)
		{
			if (!data || !data.HasAI || !data.VehicleEntity)
				continue;
			
			// Solo chequear vehículos en estado DRIVING (no en comportamiento urbano)
			if (data.UrbanState != 0)
				continue;
			
			// Comprobar velocidad del vehículo
			Physics physics = data.VehicleEntity.GetPhysics();
			if (!physics)
				continue;
			
			vector velocity = physics.GetVelocity();
			float speed = velocity.Length();
			
			// Si está casi parado (< 0.5 m/s), incrementar contador
			if (speed < 0.5)
			{
				data.UrbanTimer += m_fCheckInterval;
				
				// Si lleva parado más de 15 segundos, reasignar waypoint
				if (data.UrbanTimer > 15.0)
				{
					data.UrbanTimer = 0;
					BH_AssignNewDrivingWaypoint(data);
					
					if (m_bDebugMode)
						BH_TrafficLog("[BH_Traffic] Vehículo parado detectado. Nuevo waypoint asignado.");
				}
			}
			else
			{
				// Se está moviendo, resetear contador
				data.UrbanTimer = 0;
			}
		}
	}
	
	//! Obtiene las posiciones de todos los jugadores conectados
	protected void BH_GetAllPlayerPositions(out array<vector> positions)
	{
		// Iterar sobre los player controllers del GameMode
		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if (playerEntity)
			{
				positions.Insert(playerEntity.GetOrigin());
			}
		}
	}
	
	//! Comprueba si una posición está dentro del radio de algún jugador
	protected bool BH_IsInPlayerRadius(vector position, array<vector> playerPositions, float radius)
	{
		foreach (vector playerPos : playerPositions)
		{
			if (vector.DistanceSq(position, playerPos) <= radius * radius)
				return true;
		}
		return false;
	}
	
	// ========================================================================
	// SPAWN DE VEHÍCULOS
	// ========================================================================
	
	//! Spawnea vehículos en puntos que han entrado en rango (aleatorio, con control de ratio)
	protected void BH_SpawnInRange(array<vector> playerPositions)
	{
		// Comprobar límite global antes de buscar
		int currentCount = m_aActiveVehicles.Count();
		if (currentCount >= m_iMaxSimultaneousVehicles)
			return;
		
		int slotsAvailable = m_iMaxSimultaneousVehicles - currentCount;
		
		// Contar estáticos y dinámicos activos para controlar el ratio
		int activeStatic = 0;
		int activeDynamic = 0;
		foreach (BH_TrafficVehicleData vd : m_aActiveVehicles)
		{
			if (vd && vd.SpawnPoint)
			{
				if (vd.SpawnPoint.IsStatic)
					activeStatic++;
				else
					activeDynamic++;
			}
		}
		
		// Calcular cuántos de cada tipo deberíamos tener según el porcentaje
		int targetTotal = m_iMaxSimultaneousVehicles;
		int targetStatic = (targetTotal * m_iStaticPercentage) / 100;
		int targetDynamic = targetTotal - targetStatic;
		
		// Separar candidatos en urbanos (aptos para estático) y todos (aptos para dinámico)
		array<int> urbanCandidates = {};
		array<int> allCandidates = {};
		
		for (int idx = 0; idx < m_aSpawnPoints.Count(); idx++)
		{
			BH_TrafficSpawnPoint point = m_aSpawnPoints[idx];
			
			if (point.IsOccupied)
				continue;
			
			// Comprobar que está en el anillo
			bool inRange = false;
			bool tooClose = false;
			
			foreach (vector playerPos : playerPositions)
			{
				float distSq = vector.DistanceSq(point.Position, playerPos);
				
				if (distSq <= m_fActivationRadius * m_fActivationRadius)
					inRange = true;
				if (distSq < m_fMinSpawnRadius * m_fMinSpawnRadius)
					tooClose = true;
			}
			
			if (inRange && !tooClose && !BH_TrafficExclusionManager.BH_IsPositionExcluded(point.Position))
			{
				allCandidates.Insert(idx);
				if (point.IsUrban)
					urbanCandidates.Insert(idx);
			}
		}
		
		if (allCandidates.IsEmpty())
			return;
		
		// Barajar ambos arrays
		BH_ShuffleIntArray(allCandidates);
		BH_ShuffleIntArray(urbanCandidates);
		
		// Spawnear respetando el ratio
		int spawned = 0;
		int usedUrbanIdx = 0;
		int usedAllIdx = 0;
		
		while (spawned < slotsAvailable)
		{
			// Decidir qué tipo necesitamos
			bool needStatic = (activeStatic < targetStatic);
			bool needDynamic = (activeDynamic < targetDynamic);
			
			// Si ambos necesitan, alternar; si ninguno necesita, hacer dinámico
			bool makeStatic = false;
			if (needStatic && !needDynamic)
				makeStatic = true;
			else if (!needStatic && needDynamic)
				makeStatic = false;
			else if (needStatic && needDynamic)
				makeStatic = (Math.RandomInt(0, 100) < m_iStaticPercentage);
			else
				makeStatic = (Math.RandomInt(0, 100) < m_iStaticPercentage);
			
			BH_TrafficSpawnPoint candidate = null;
			
			if (makeStatic)
			{
				// Buscar punto urbano disponible
				while (usedUrbanIdx < urbanCandidates.Count())
				{
					BH_TrafficSpawnPoint pt = m_aSpawnPoints[urbanCandidates[usedUrbanIdx]];
					usedUrbanIdx++;
					if (!pt.IsOccupied)
					{
						candidate = pt;
						break;
					}
				}
				
				// Si no hay urbanos disponibles, forzar dinámico
				if (!candidate)
					makeStatic = false;
			}
			
			if (!makeStatic && !candidate)
			{
				// Buscar cualquier punto disponible para dinámico
				while (usedAllIdx < allCandidates.Count())
				{
					BH_TrafficSpawnPoint pt = m_aSpawnPoints[allCandidates[usedAllIdx]];
					usedAllIdx++;
					if (!pt.IsOccupied)
					{
						candidate = pt;
						break;
					}
				}
			}
			
			// Sin candidatos disponibles, salir
			if (!candidate)
				break;
			
			candidate.IsStatic = makeStatic;
			BH_SpawnVehicleAtPoint(candidate);
			spawned++;
			
			if (makeStatic)
				activeStatic++;
			else
				activeDynamic++;
		}
		
		if (m_bDebugMode && spawned > 0)
		{
			int newCount = m_aActiveVehicles.Count();
			BH_TrafficLog("[BH_Traffic] Spawneados " + spawned.ToString() + " | Estáticos: " + activeStatic.ToString() + "/" + targetStatic.ToString() + " | Dinámicos: " + activeDynamic.ToString() + "/" + targetDynamic.ToString());
		}
	}
	
	//! Baraja un array de enteros (Fisher-Yates)
	protected void BH_ShuffleIntArray(array<int> arr)
	{
		int n = arr.Count();
		for (int i = n - 1; i > 0; i--)
		{
			int j = Math.RandomInt(0, i + 1);
			int temp = arr[i];
			arr[i] = arr[j];
			arr[j] = temp;
		}
	}
	
	//! Crea un vehículo en un punto de spawn concreto
	protected void BH_SpawnVehicleAtPoint(BH_TrafficSpawnPoint point)
	{
		if (!m_aPrefabsVehicles || m_aPrefabsVehicles.IsEmpty())
		{
			BH_TrafficLog("[BH_Traffic] ERROR: No hay prefabs de vehículos configurados.");
			return;
		}
		
		// Elegir prefab aleatorio
		int randomIndex = Math.RandomInt(0, m_aPrefabsVehicles.Count());
		ResourceName prefab = m_aPrefabsVehicles[randomIndex];
		
		// Calcular posición final
		vector spawnPos = point.Position;
		vector spawnAngles = point.Direction;
		
		if (point.IsStatic)
		{
			// Vehículos estáticos: desplazar más al lateral (5-8m) como aparcado
			float yawRad = spawnAngles[0] * Math.DEG2RAD;
			vector roadDir = Vector(Math.Sin(yawRad), 0, Math.Cos(yawRad));
			vector rightDir = Vector(-roadDir[2], 0, roadDir[0]);
			
			// Desplazar 5-8m a la derecha de la carretera
			float lateralOffset = 5.0 + Math.RandomFloat01() * 3.0;
			spawnPos = spawnPos + rightDir * lateralOffset;
			
			// Ajustar Y al suelo tras el desplazamiento
			BaseWorld world = GetGame().GetWorld();
			if (world)
			{
				spawnPos[1] = world.GetSurfaceY(spawnPos[0], spawnPos[2]);
			}
			
			// Rotar ligeramente el vehículo aparcado (no perfectamente alineado)
			float parkAngleOffset = -10.0 + Math.RandomFloat01() * 20.0;
			spawnAngles[0] = spawnAngles[0] + parkAngleOffset;
		}
		
		// Preparar parámetros de spawn
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		
		vector mat[4];
		Math3D.AnglesToMatrix(spawnAngles, mat);
		mat[3] = spawnPos;
		params.Transform = mat;
		
		// Spawn de la entidad
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			BH_TrafficLog("[BH_Traffic] ERROR: No se pudo cargar prefab: " + prefab);
			return;
		}
		
		IEntity vehicle = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		
		if (!vehicle)
		{
			BH_TrafficLog("[BH_Traffic] ERROR: Falló el spawn del vehículo.");
			return;
		}
		
		// Registrar en datos
		BH_TrafficVehicleData data = new BH_TrafficVehicleData();
		data.VehicleEntity = vehicle;
		data.SpawnPoint = point;
		data.HasAI = !point.IsStatic;
		
		point.IsOccupied = true;
		point.AssignedVehicleID = m_aActiveVehicles.Count();
		
		m_aActiveVehicles.Insert(data);
		
		// Si es dinámico, asignar IA conductora
		if (!point.IsStatic)
		{
			BH_AssignDriverAI(vehicle, point, data);
		}
		
		if (m_bDebugMode)
		{
			string type = "DINÁMICO";
			if (point.IsStatic)
				type = "ESTÁTICO";
			
			BH_TrafficLog("[BH_Traffic] Spawn " + type + " en " + point.Position.ToString());
		}
	}
	
	// ========================================================================
	// DESPAWN DE VEHÍCULOS
	// ========================================================================
	
	//! Elimina vehículos que han salido del radio de todos los jugadores
	protected void BH_DespawnOutOfRange(array<vector> playerPositions)
	{
		// Recorrer de atrás hacia adelante para poder eliminar mientras iteramos
		for (int i = m_aActiveVehicles.Count() - 1; i >= 0; i--)
		{
			BH_TrafficVehicleData data = m_aActiveVehicles[i];
			
			if (!data || !data.VehicleEntity)
			{
				// Vehículo ya no existe, limpiar
				BH_CleanupVehicleData(data, i);
				continue;
			}
			
			vector vehiclePos = data.VehicleEntity.GetOrigin();
			
			// Si está dentro del radio de desactivación de algún jugador, mantener
			if (BH_IsInPlayerRadius(vehiclePos, playerPositions, m_fDeactivationRadius))
				continue;
			
			// Fuera de rango → despawn
			BH_DespawnVehicle(data, i);
		}
	}
	
	//! Elimina un vehículo y limpia sus datos
	protected void BH_DespawnVehicle(BH_TrafficVehicleData data, int index)
	{
		if (m_bDebugMode)
		{
			BH_TrafficLog("[BH_Traffic] Despawn vehículo en " + data.VehicleEntity.GetOrigin().ToString());
		}
		
		// Limpiar waypoint
		if (data.WaypointEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(data.WaypointEntity);
			data.WaypointEntity = null;
		}
		
		// Limpiar conductor (sacarlo del vehículo primero)
		if (data.DriverEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(data.DriverEntity);
			data.DriverEntity = null;
		}
		
		// Limpiar grupo IA
		if (data.GroupEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(data.GroupEntity);
			data.GroupEntity = null;
		}
		
		// Liberar el spawn point
		if (data.SpawnPoint)
		{
			data.SpawnPoint.IsOccupied = false;
			data.SpawnPoint.AssignedVehicleID = -1;
		}
		
		// Eliminar el vehículo
		SCR_EntityHelper.DeleteEntityAndChildren(data.VehicleEntity);
		
		// Eliminar del array
		m_aActiveVehicles.Remove(index);
	}
	
	//! Limpia datos de un vehículo que ya no existe
	protected void BH_CleanupVehicleData(BH_TrafficVehicleData data, int index)
	{
		if (data && data.SpawnPoint)
		{
			data.SpawnPoint.IsOccupied = false;
			data.SpawnPoint.AssignedVehicleID = -1;
		}
		m_aActiveVehicles.Remove(index);
	}
	
	// ========================================================================
	// IA DE CONDUCCIÓN
	// ========================================================================
	
	//! Asigna un conductor IA a un vehículo para que circule
	protected void BH_AssignDriverAI(IEntity vehicle, BH_TrafficSpawnPoint originPoint, BH_TrafficVehicleData vehicleData)
	{
		if (!m_aPrefabsDrivers || m_aPrefabsDrivers.IsEmpty())
		{
			BH_TrafficLog("[BH_Traffic] AVISO: No hay prefabs de conductores configurados.");
			return;
		}
		
		// --- Paso 1: Crear un grupo IA ---
		Resource groupRes = Resource.Load("{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et");
		if (!groupRes || !groupRes.IsValid())
		{
			BH_TrafficLog("[BH_Traffic] AVISO: No se pudo cargar prefab de grupo IA.");
			return;
		}
		
		EntitySpawnParams groupParams = new EntitySpawnParams();
		groupParams.TransformMode = ETransformMode.WORLD;
		vector groupTransform[4];
		vehicle.GetTransform(groupTransform);
		groupParams.Transform = groupTransform;
		
		IEntity groupEntity = GetGame().SpawnEntityPrefab(groupRes, GetGame().GetWorld(), groupParams);
		if (!groupEntity)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Falló spawn del grupo IA.");
			return;
		}
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(groupEntity);
		if (!aiGroup)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Entidad no es SCR_AIGroup.");
			SCR_EntityHelper.DeleteEntityAndChildren(groupEntity);
			return;
		}
		
		// Guardar referencia al grupo
		vehicleData.GroupEntity = groupEntity;
		
		// --- Paso 2: Crear el conductor ---
		int randomIndex = Math.RandomInt(0, m_aPrefabsDrivers.Count());
		ResourceName driverPrefab = m_aPrefabsDrivers[randomIndex];
		
		Resource driverRes = Resource.Load(driverPrefab);
		if (!driverRes || !driverRes.IsValid())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(groupEntity);
			vehicleData.GroupEntity = null;
			return;
		}
		
		EntitySpawnParams driverParams = new EntitySpawnParams();
		driverParams.TransformMode = ETransformMode.WORLD;
		vector driverTransform[4];
		vehicle.GetTransform(driverTransform);
		driverParams.Transform = driverTransform;
		
		IEntity driver = GetGame().SpawnEntityPrefab(driverRes, GetGame().GetWorld(), driverParams);
		if (!driver)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(groupEntity);
			vehicleData.GroupEntity = null;
			return;
		}
		
		// Guardar referencia al conductor
		vehicleData.DriverEntity = driver;
		
		// --- Paso 3: Añadir conductor al grupo ---
		AIControlComponent aiControl = AIControlComponent.Cast(driver.FindComponent(AIControlComponent));
		if (aiControl)
		{
			aiGroup.AddAIEntityToGroup(driver);
			BH_TrafficLog("[BH_Traffic] Conductor añadido al grupo IA.");
		}
		else
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Conductor sin AIControlComponent.");
		}
		
		// --- Paso 4: Montar y conducir con delay ---
		GetGame().GetCallqueue().CallLater(BH_DelayedMountAndDrive, 500, false, driver, vehicle, originPoint, aiGroup, vehicleData);
	}
	
	//! Monta el conductor y asigna waypoint con delay
	protected void BH_DelayedMountAndDrive(IEntity driver, IEntity vehicle, BH_TrafficSpawnPoint originPoint, SCR_AIGroup aiGroup, BH_TrafficVehicleData vehicleData)
	{
		if (!driver || !vehicle || !aiGroup)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Entidades perdidas en DelayedMountAndDrive.");
			return;
		}
		
		// Montar en vehículo
		BH_MoveCharacterIntoVehicle(driver, vehicle);
		
		// Asignar waypoint de conducción al grupo
		BH_AssignDrivingWaypoint(driver, vehicle, originPoint, aiGroup, vehicleData);
	}
	
	//! Mueve un personaje al asiento de conductor del vehículo
	protected void BH_MoveCharacterIntoVehicle(IEntity character, IEntity vehicle)
	{
		// Obtener el CompartmentAccessComponent del personaje
		CompartmentAccessComponent accessComp = CompartmentAccessComponent.Cast(character.FindComponent(CompartmentAccessComponent));
		if (!accessComp)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Personaje sin CompartmentAccessComponent.");
			return;
		}
		
		// Buscar el slot de piloto manualmente en el vehículo
		BaseCompartmentManagerComponent compManager = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		if (!compManager)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Vehículo sin CompartmentManager.");
			return;
		}
		
		array<BaseCompartmentSlot> compartments = {};
		compManager.GetCompartments(compartments);
		
		BaseCompartmentSlot pilotSlot = null;
		foreach (BaseCompartmentSlot slot : compartments)
		{
			// Verificar si es un PilotCompartmentSlot libre
			if (PilotCompartmentSlot.Cast(slot) && !slot.GetOccupant())
			{
				pilotSlot = slot;
				break;
			}
		}
		
		if (!pilotSlot)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: No se encontró slot de piloto libre.");
			return;
		}
		
		// Forzar teleport al compartimento usando GetInVehicle del engine
		// forceTeleport=true teletransporta sin animación
		// doorInfoIndex=-1 (ignorado con teleport)
		// closeDoor=0 (no interactuar con puertas)
		// performWhenPaused=false
		bool success = accessComp.GetInVehicle(vehicle, pilotSlot, true, -1, 0, false);
		
		if (success)
			BH_TrafficLog("[BH_Traffic] Conductor montado en vehículo.");
		else
			BH_TrafficLog("[BH_Traffic] AVISO: No se pudo montar conductor.");
	}
	
	//! Asigna un waypoint de conducción al grupo IA
	protected void BH_AssignDrivingWaypoint(IEntity driver, IEntity vehicle, BH_TrafficSpawnPoint originPoint, SCR_AIGroup aiGroup, BH_TrafficVehicleData vehicleData)
	{
		if (!aiGroup)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Sin grupo IA para waypoint.");
			return;
		}
		
		// Buscar un destino aleatorio variando el rango cada vez
		// para que las rutas no sean siempre iguales
		float randomMinDist = 200 + Math.RandomFloat01() * 500;  // 200-700m mínimo
		float randomMaxDist = 1000 + Math.RandomFloat01() * 4000; // 1000-5000m máximo
		
		BH_TrafficSpawnPoint destination = BH_FindDistantSpawnPoint(originPoint, randomMinDist, randomMaxDist);
		
		if (!destination)
		{
			// Ampliar búsqueda si no hay resultado
			destination = BH_FindDistantSpawnPoint(originPoint, 50, 8000);
		}
		
		if (!destination)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: No se encontró destino para conducción IA.");
			return;
		}
		
		// Spawn de un waypoint de movimiento
		EntitySpawnParams wpParams = new EntitySpawnParams();
		wpParams.TransformMode = ETransformMode.WORLD;
		
		vector wpTransform[4];
		Math3D.MatrixIdentity4(wpTransform);
		wpTransform[3] = destination.Position;
		wpParams.Transform = wpTransform;
		
		// Intentar cargar waypoint de movimiento
		// Probamos varios prefabs conocidos en orden de preferencia
		Resource wpResource;
		
		wpResource = Resource.Load("{22A875E30470BD4F}Prefabs/AI/Waypoints/AIWaypoint_Move.et");
		if (!wpResource || !wpResource.IsValid())
		{
			wpResource = Resource.Load("{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint.et");
		}
		
		if (!wpResource || !wpResource.IsValid())
		{
			BH_TrafficLog("[BH_Traffic] AVISO: No se pudo cargar prefab de waypoint de movimiento.");
			return;
		}
		
		IEntity waypoint = GetGame().SpawnEntityPrefab(wpResource, GetGame().GetWorld(), wpParams);
		if (!waypoint)
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Falló spawn del waypoint.");
			return;
		}
		
		AIWaypoint wp = AIWaypoint.Cast(waypoint);
		if (wp)
		{
			aiGroup.AddWaypoint(wp);
			if (vehicleData)
				vehicleData.WaypointEntity = waypoint;
			BH_TrafficLog("[BH_Traffic] Waypoint asignado. Destino: " + destination.Position.ToString());
		}
		else
		{
			BH_TrafficLog("[BH_Traffic] AVISO: Entidad no es AIWaypoint.");
			SCR_EntityHelper.DeleteEntityAndChildren(waypoint);
		}
	}
	
	//! Función pública para asignar nuevo waypoint de conducción (usada por BH_TrafficUrbanBehavior)
	void BH_AssignNewDrivingWaypoint(BH_TrafficVehicleData vehicleData)
	{
		if (!vehicleData || !vehicleData.DriverEntity || !vehicleData.VehicleEntity || !vehicleData.GroupEntity || !vehicleData.SpawnPoint)
			return;
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(vehicleData.GroupEntity);
		if (!aiGroup)
			return;
		
		BH_AssignDrivingWaypoint(vehicleData.DriverEntity, vehicleData.VehicleEntity, vehicleData.SpawnPoint, aiGroup, vehicleData);
	}
	
	//! Busca un spawn point a una distancia determinada del origen
	protected BH_TrafficSpawnPoint BH_FindDistantSpawnPoint(BH_TrafficSpawnPoint origin, float minDist, float maxDist)
	{
		float minDistSq = minDist * minDist;
		float maxDistSq = maxDist * maxDist;
		
		// Buscar puntos candidatos a la distancia adecuada
		// Cualquier punto de carretera vale como destino
		array<ref BH_TrafficSpawnPoint> candidates = {};
		
		foreach (BH_TrafficSpawnPoint point : m_aSpawnPoints)
		{
			float distSq = vector.DistanceSq(origin.Position, point.Position);
			if (distSq >= minDistSq && distSq <= maxDistSq)
			{
				candidates.Insert(point);
			}
		}
		
		if (candidates.IsEmpty())
			return null;
		
		// Elegir uno aleatorio
		return candidates[Math.RandomInt(0, candidates.Count())];
	}
	
	//! Elimina la IA de un vehículo antes del despawn
	protected void BH_RemoveDriverAI(IEntity vehicle)
	{
		// Obtener compartimentos y buscar el conductor
		BaseCompartmentManagerComponent compManager = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		if (!compManager)
			return;
		
		array<BaseCompartmentSlot> compartments = {};
		compManager.GetCompartments(compartments);
		
		foreach (BaseCompartmentSlot slot : compartments)
		{
			IEntity occupant = slot.GetOccupant();
			if (occupant)
			{
				// Eliminar el ocupante
				SCR_EntityHelper.DeleteEntityAndChildren(occupant);
			}
		}
	}
	
	// ========================================================================
	// UTILIDADES
	// ========================================================================
	
	//! Comprueba si estamos en servidor
	protected bool BH_TrafficIsServer()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (rpl)
			return rpl.IsMaster();
		
		// Sin RplComponent = singleplayer
		return true;
	}
	
	//! Log de debug
	protected void BH_TrafficLog(string message)
	{
		if (m_bDebugMode)
			Print(message, LogLevel.NORMAL);
	}
	
	// ========================================================================
	// LIMPIEZA
	// ========================================================================
	
	override void OnDelete(IEntity owner)
	{
		// Eliminar todos los vehículos activos al cerrar
		if (m_aActiveVehicles)
		{
			for (int i = m_aActiveVehicles.Count() - 1; i >= 0; i--)
			{
				BH_TrafficVehicleData data = m_aActiveVehicles[i];
				if (data && data.VehicleEntity)
				{
					if (data.HasAI)
						BH_RemoveDriverAI(data.VehicleEntity);
					
					SCR_EntityHelper.DeleteEntityAndChildren(data.VehicleEntity);
				}
			}
			m_aActiveVehicles.Clear();
		}
		
		super.OnDelete(owner);
	}
};
