// ============================================================================
// BH_TrafficExclusionManager.c
//
// Manager estático (singleton) que gestiona todas las zonas de exclusión
// de tráfico registradas en el mapa.
//
// BH_TrafficComponent consulta este manager antes de spawnear cualquier
// vehículo. Si el punto de spawn cae dentro de alguna zona registrada,
// el punto es descartado silenciosamente.
//
// No necesita ser añadido a ningún Entity. Es una clase estática pura
// que vive en memoria mientras la misión está activa.
//
// Funciona en local y en servidor dedicado.
//
// Autor: Bhelma
// ============================================================================

class BH_TrafficExclusionManager
{
	// ========================================================================
	// ESTADO INTERNO (estático)
	// ========================================================================

	//! Lista de todas las zonas de exclusión activas registradas
	protected static ref array<BH_TrafficExclusionZone> s_aZones = new array<BH_TrafficExclusionZone>();

	// ========================================================================
	// REGISTRO DE ZONAS
	// ========================================================================

	//! Registra una zona de exclusión. Llamado automáticamente por BH_TrafficExclusionZone.OnPostInit
	static void BH_RegisterZone(BH_TrafficExclusionZone zone)
	{
		if (!zone)
			return;

		// Evitar duplicados
		if (s_aZones.Find(zone) != -1)
			return;

		s_aZones.Insert(zone);
	}

	//! Desregistra una zona de exclusión. Llamado automáticamente por BH_TrafficExclusionZone.OnDelete
	static void BH_UnregisterZone(BH_TrafficExclusionZone zone)
	{
		if (!zone)
			return;

		int idx = s_aZones.Find(zone);
		if (idx != -1)
			s_aZones.Remove(idx);
	}

	// ========================================================================
	// CONSULTA PRINCIPAL
	// ========================================================================

	//! Comprueba si una posición está dentro de alguna zona de exclusión.
	//! Devuelve true si la posición está EXCLUIDA (no se debe spawnear aquí).
	//! Esta es la única función que BH_TrafficComponent necesita llamar.
	static bool BH_IsPositionExcluded(vector position)
	{
		// Sin zonas registradas = nada excluido (coste cero)
		if (s_aZones.IsEmpty())
			return false;

		foreach (BH_TrafficExclusionZone zone : s_aZones)
		{
			// Ignorar zonas desactivadas
			if (!zone || !zone.BH_IsEnabled())
				continue;

			// Obtener el entity propietario de la zona para saber su posición
			IEntity zoneEntity = zone.GetOwner();
			if (!zoneEntity)
				continue;

			vector zoneCenter = zoneEntity.GetOrigin();
			float radius = zone.BH_GetRadius();

			// Comparación 2D (ignoramos diferencia de altura, igual que el resto del sistema)
			float dx = position[0] - zoneCenter[0];
			float dz = position[2] - zoneCenter[2];
			float distSq = dx * dx + dz * dz;

			if (distSq <= radius * radius)
				return true; // Dentro de zona de exclusión
		}

		return false; // Fuera de todas las zonas
	}

	// ========================================================================
	// UTILIDADES DE DEBUG
	// ========================================================================

	//! Devuelve el número de zonas de exclusión registradas
	static int BH_GetZoneCount()
	{
		return s_aZones.Count();
	}

	//! Loguea todas las zonas registradas (útil para debug)
	static void BH_DebugPrintZones()
	{
		Print("[BH_ExclusionManager] Zonas registradas: " + s_aZones.Count().ToString(), LogLevel.NORMAL);

		int i = 0;
		foreach (BH_TrafficExclusionZone zone : s_aZones)
		{
			if (!zone)
				continue;

			IEntity zoneEntity = zone.GetOwner();
			if (!zoneEntity)
				continue;

			string status = "INACTIVA";
			if (zone.BH_IsEnabled())
				status = "ACTIVA";
			Print("[BH_ExclusionManager]   [" + i.ToString() + "] Pos: " + zoneEntity.GetOrigin().ToString() + " | Radio: " + zone.BH_GetRadius().ToString() + "m | " + status, LogLevel.NORMAL);
			i++;
		}
	}
};