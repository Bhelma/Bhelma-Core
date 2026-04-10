// ============================================================
//  BH_WireCutter.c
//  Autor: BH Mod
//  Descripción: Componente que se añade al prefab del ítem
//               "Alicates" (WireCutter). Su presencia en el
//               inventario del jugador es suficiente para
//               habilitar la interacción de desactivación
//               de IEDs.
//  [SIN CAMBIOS] — Este archivo no tenía bugs propios
// ============================================================

class BH_WireCutterClass : ScriptComponentClass {}

class BH_WireCutter : ScriptComponent
{
	// --------------------------------------------------------
	// Atributos configurables desde Workbench
	// --------------------------------------------------------

	[Attribute("1", UIWidgets.CheckBox, "Puede desactivar IEDs")]
	protected bool m_bCanDisarm;

	[Attribute("Alicates de desactivación", UIWidgets.EditBox, "Nombre mostrado en inventario")]
	protected string m_sDisplayName;

	// --------------------------------------------------------
	// Init
	// --------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		BH_IED_Utils.BH_Log(string.Format("[BH_WireCutter] Ítem inicializado: %1", m_sDisplayName));
	}

	// --------------------------------------------------------
	// Getter
	// --------------------------------------------------------

	bool BH_CanDisarm() { return m_bCanDisarm; }
}