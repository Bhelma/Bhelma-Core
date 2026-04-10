// ============================================================================
// BH_TrafficUtils.c
//
// Utilidades, constantes y funciones auxiliares para el sistema de tráfico
// automático BH_TrafficComponent.
//
// Autor: Bhelma
// ============================================================================

// ============================================================================
// ENUMS
// ============================================================================

//! Tipos de carretera detectados
enum BH_ERoadType
{
	HIGHWAY,      // Autopista / carretera principal
	SECONDARY,    // Carretera secundaria
	RURAL,        // Camino rural / tierra
	URBAN_STREET, // Calle urbana
	UNKNOWN       // No identificado
};

//! Estado de un vehículo gestionado
enum BH_EVehicleState
{
	PARKED,       // Estacionado, sin IA
	DRIVING,      // Circulando con IA
	WAITING,      // En espera (pendiente de spawn)
	DESPAWNING    // Marcado para eliminación
};

// ============================================================================
// CLASE DE CONFIGURACIÓN EXTENDIDA
// ============================================================================

//! Configuración de un tipo de carretera para densidad variable
[BaseContainerProps()]
class BH_TrafficRoadConfig
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Tipo de carretera", enums: ParamEnumArray.FromEnum(BH_ERoadType), category: "Road Config")]
	BH_ERoadType m_eRoadType;
	
	[Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Multiplicador de densidad para este tipo de vía", params: "0.1 5 0.1", category: "Road Config")]
	float m_fDensityMultiplier;
	
	[Attribute(defvalue: "60", uiwidget: UIWidgets.Slider, desc: "Velocidad máxima de IA en km/h para este tipo de vía", params: "10 120 5", category: "Road Config")]
	float m_fMaxSpeedKmh;
	
	[Attribute(defvalue: "30", uiwidget: UIWidgets.Slider, desc: "Porcentaje de vehículos estáticos en este tipo de vía", params: "0 100 5", category: "Road Config")]
	int m_iStaticPercent;
};

// ============================================================================
// FUNCIONES ESTÁTICAS DE UTILIDAD
// ============================================================================

class BH_TrafficUtils
{
	// ========================================================================
	// POSICIONAMIENTO
	// ========================================================================
	
	//! Obtiene la posición en el suelo para una coordenada XZ
	static vector BH_GetGroundPosition(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return pos;
		
		float groundY = world.GetSurfaceY(pos[0], pos[2]);
		return Vector(pos[0], groundY, pos[2]);
	}
	
	//! Desplaza una posición lateralmente respecto a una dirección (para aparcar en cuneta)
	static vector BH_OffsetPositionLateral(vector position, vector direction, float offsetMeters)
	{
		// Calcular vector perpendicular (derecha)
		vector right = direction * Vector(0, 1, 0); // Cross product simplificado
		right.Normalize();
		
		return position + (right * offsetMeters);
	}
	
	//! Calcula la orientación (ángulos) para que un vehículo mire en la dirección de la carretera
	static vector BH_CalculateVehicleAngles(vector direction)
	{
		float yaw = Math.Atan2(direction[0], direction[2]) * Math.RAD2DEG;
		return Vector(yaw, 0, 0);
	}
	
	//! Verifica si una posición es válida para spawn (no está dentro de otro objeto)
	static bool BH_IsPositionClear(vector position, float checkRadius = 3.0)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;
		
		// Trazar una línea hacia abajo para verificar que hay suelo
		vector start = position + Vector(0, 5, 0);
		vector end = position - Vector(0, 1, 0);
		
		TraceParam trace = new TraceParam();
		trace.Start = start;
		trace.End = end;
		trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		trace.LayerMask = EPhysicsLayerPresets.Projectile;
		
		float traceResult = world.TraceMove(trace, null);
		
		// Si el trace no golpeó nada, posición no válida (sin suelo)
		if (traceResult >= 1.0)
			return false;
		
		// Verificar que no hay entidades muy cerca
		// Usar un query pequeño para detectar colisiones
		return true;
	}
	
	//! Calcula la pendiente del terreno en una posición
	static float BH_GetTerrainSlope(vector position, float sampleDistance = 2.0)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;
		
		// Muestrear 4 puntos alrededor
		float yN = world.GetSurfaceY(position[0], position[2] + sampleDistance);
		float yS = world.GetSurfaceY(position[0], position[2] - sampleDistance);
		float yE = world.GetSurfaceY(position[0] + sampleDistance, position[2]);
		float yW = world.GetSurfaceY(position[0] - sampleDistance, position[2]);
		
		float slopeNS = Math.AbsFloat(yN - yS) / (sampleDistance * 2);
		float slopeEW = Math.AbsFloat(yE - yW) / (sampleDistance * 2);
		
		// Devolver la pendiente máxima en grados
		float maxSlope = Math.Max(slopeNS, slopeEW);
		return Math.Atan2(maxSlope, 1.0) * Math.RAD2DEG;
	}
	
	// ========================================================================
	// ALEATORIZACIÓN
	// ========================================================================
	
	//! Devuelve un float aleatorio entre min y max
	static float BH_RandomFloat(float min, float max)
	{
		return min + (Math.RandomFloat01() * (max - min));
	}
	
	//! Baraja un array de spawn points (Fisher-Yates)
	static void BH_ShuffleSpawnPoints(array<ref BH_TrafficSpawnPoint> points)
	{
		int n = points.Count();
		for (int i = n - 1; i > 0; i--)
		{
			int j = Math.RandomInt(0, i + 1);
			ref BH_TrafficSpawnPoint temp = points[i];
			points[i] = points[j];
			points[j] = temp;
		}
	}
	
	// ========================================================================
	// DISTANCIAS Y GEOMETRÍA
	// ========================================================================
	
	//! Distancia 2D (ignora Y) entre dos posiciones
	static float BH_Distance2D(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return Math.Sqrt(dx * dx + dz * dz);
	}
	
	//! Distancia 2D al cuadrado (más eficiente para comparaciones)
	static float BH_Distance2DSq(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}
	
	//! Verifica si un punto está dentro de un polígono 2D (ray casting)
	static bool BH_IsPointInPolygon2D(vector point, array<vector> polygon)
	{
		int n = polygon.Count();
		bool inside = false;
		
		int j = n - 1;
		for (int i = 0; i < n; i++)
		{
			if ((polygon[i][2] > point[2]) != (polygon[j][2] > point[2]) &&
				point[0] < (polygon[j][0] - polygon[i][0]) * (point[2] - polygon[i][2]) / (polygon[j][2] - polygon[i][2]) + polygon[i][0])
			{
				inside = !inside;
			}
			j = i;
		}
		
		return inside;
	}
};
