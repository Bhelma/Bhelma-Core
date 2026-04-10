// ============================================================================
// BH_TrafficExclusionZone.c
//
// Componente de zona de exclusión para el sistema de tráfico BH_Traffic.
//
// USO:
//   1. Crear un Entity vacío en el editor de Reforger (clic derecho > Create Entity).
//   2. Añadir este componente al Entity.
//   3. Configurar m_fRadius con el radio deseado en metros.
//   4. Colocar el Entity sobre la zona que quieres excluir (base militar, aeropuerto, etc.).
//   5. Repetir para cada zona de exclusión que necesites.
//
// El componente se registra automáticamente en BH_TrafficExclusionManager
// al inicializarse, y se desregistra al destruirse.
//
// Funciona en local y en servidor dedicado (solo el servidor lo procesa).
//
// Autor: Bhelma
// ============================================================================

[ComponentEditorProps(category: "GameScripted/Traffic", description: "BH - Zona de exclusión de tráfico. Los vehículos civiles no spawnearan ni circularan dentro de este radio.", icon: "WBData")]
class BH_TrafficExclusionZoneClass : ScriptComponentClass
{
};

class BH_TrafficExclusionZone : ScriptComponent
{
	// ========================================================================
	// ATRIBUTOS CONFIGURABLES EN EL EDITOR
	// ========================================================================

	[Attribute(defvalue: "200", uiwidget: UIWidgets.Slider, desc: "Radio de exclusion en metros. Ningun vehiculo civil spawnea ni circula dentro de este area.", params: "10 5000 10", category: "BH Exclusion Zone")]
	protected float m_fRadius;

	[Attribute(defvalue: "true", desc: "Activar esta zona de exclusion", category: "BH Exclusion Zone")]
	protected bool m_bEnabled;

	[Attribute(defvalue: "true", desc: "Mostrar info de debug en consola al registrar/desregistrar la zona", category: "BH Exclusion Zone")]
	protected bool m_bDebugMode;

	// ========================================================================
	// INICIALIZACIÓN
	// ========================================================================

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!m_bEnabled)
			return;

		// Solo registrar en servidor (o SP)
		if (!BH_ExclusionIsServer(owner))
			return;

		// Registrar en el manager global
		BH_TrafficExclusionManager.BH_RegisterZone(this);

		if (m_bDebugMode)
		{
			Print("[BH_ExclusionZone] Zona registrada en " + owner.GetOrigin().ToString() + " | Radio: " + m_fRadius.ToString() + "m", LogLevel.NORMAL);
		}
	}

	// ========================================================================
	// DESTRUCCIÓN
	// ========================================================================

	override void OnDelete(IEntity owner)
	{
		// Desregistrar del manager al destruirse el entity
		BH_TrafficExclusionManager.BH_UnregisterZone(this);

		if (m_bDebugMode)
		{
			Print("[BH_ExclusionZone] Zona desregistrada.", LogLevel.NORMAL);
		}

		super.OnDelete(owner);
	}

	// ========================================================================
	// API PÚBLICA
	// ========================================================================

	//! Devuelve el radio de exclusión en metros
	float BH_GetRadius()
	{
		return m_fRadius;
	}

	//! Devuelve si la zona está activa
	bool BH_IsEnabled()
	{
		return m_bEnabled;
	}

	//! Activa o desactiva la zona en tiempo de ejecución
	void BH_SetEnabled(bool enabled)
	{
		m_bEnabled = enabled;
	}

	// ========================================================================
	// UTILIDAD INTERNA
	// ========================================================================

	//! Comprueba si estamos en servidor (igual que BH_TrafficComponent)
	protected bool BH_ExclusionIsServer(IEntity owner)
	{
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (rpl)
			return rpl.IsMaster();

		// Sin RplComponent = singleplayer
		return true;
	}
};