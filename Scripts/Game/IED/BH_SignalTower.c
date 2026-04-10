// ============================================================
//  BH_SignalTower.c
//  Autor: BH Mod
//  Descripción: Antena de señal móvil + acción de sabotaje.
//
//  SONIDO: Mismo patrón que BH_CuttTool_UserAction.
//  SoundEvent para arrancar, SOUND_STOP_PLAYING para parar.
//  Se ejecuta directamente sobre el SoundComponent del owner
//  (la antena), no del jugador.
// ============================================================

// ============================================================
//  Acción: Sabotear antena
//  Mismo patrón de sonido que BH_CuttTool_UserAction
// ============================================================

class BH_SabotageTowerAction : ScriptedUserAction
{
	// --------------------------------------------------------
	// Visibilidad y condiciones
	// --------------------------------------------------------

	override bool CanBeShownScript(IEntity user)
	{
		BH_SignalTower tower = BH_GetTower();
		if (!tower) return false;

		if (!tower.BH_IsOperational()) return false;

		return BH_PlayerHasWireCutter(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	// --------------------------------------------------------
	// Completado — sabotear la antena
	// --------------------------------------------------------

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_SignalTower tower = BH_GetTower();
		if (!tower) return;

		tower.BH_Sabotage();
	}

	// --------------------------------------------------------
	// Helpers
	// --------------------------------------------------------

	protected BH_SignalTower BH_GetTower()
	{
		IEntity owner = GetOwner();
		if (!owner) return null;
		return BH_SignalTower.Cast(owner.FindComponent(BH_SignalTower));
	}

	protected bool BH_PlayerHasWireCutter(IEntity playerEntity)
	{
		CharacterControllerComponent charCtrl = CharacterControllerComponent.Cast(
			playerEntity.FindComponent(CharacterControllerComponent)
		);
		if (!charCtrl) return false;

		IEntity heldItem = charCtrl.GetAttachedGadgetAtLeftHandSlot();
		if (heldItem)
		{
			BH_WireCutter wireCutter = BH_WireCutter.Cast(heldItem.FindComponent(BH_WireCutter));
			if (wireCutter && wireCutter.BH_CanDisarm())
				return true;
		}

		heldItem = charCtrl.GetRightHandItem();
		if (heldItem)
		{
			BH_WireCutter wireCutter = BH_WireCutter.Cast(heldItem.FindComponent(BH_WireCutter));
			if (wireCutter && wireCutter.BH_CanDisarm())
				return true;
		}

		return false;
	}
}

// ============================================================
//  BH_SignalTower — componente principal
// ============================================================

class BH_SignalTowerClass : ScriptComponentClass {}

class BH_SignalTower : ScriptComponent
{
	// --------------------------------------------------------
	// Registro estático de todas las antenas en el mundo
	// --------------------------------------------------------

	protected static ref array<BH_SignalTower> s_aTowers = new array<BH_SignalTower>();

	// --------------------------------------------------------
	// Variables replicadas
	// --------------------------------------------------------

	[RplProp(onRplName: "BH_OnTowerStateChanged")]
	protected bool m_bOperational;

	// --------------------------------------------------------
	// Atributos configurables desde Workbench
	// --------------------------------------------------------

	[Attribute("500", UIWidgets.Slider, "Radio de cobertura de señal (metros)", "50 2000 50")]
	protected float m_fCoverageRadius;

	// --------------------------------------------------------
	// Init
	// --------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!s_aTowers.Contains(this))
			s_aTowers.Insert(this);

		if (!BH_IED_Utils.BH_IsServer())
			return;

		m_bOperational = true;
		Replication.BumpMe();

		BH_IED_Utils.BH_Log(string.Format("[BH_SignalTower] Antena inicializada. Radio: %1m. Total: %2", m_fCoverageRadius, s_aTowers.Count()));
	}

	// --------------------------------------------------------
	// Cleanup
	// --------------------------------------------------------

	override void OnDelete(IEntity owner)
	{
		s_aTowers.RemoveItem(this);
		super.OnDelete(owner);
	}

	// --------------------------------------------------------
	// API — sabotaje
	// --------------------------------------------------------

	void BH_Sabotage()
	{
		if (!BH_IED_Utils.BH_IsServer()) return;
		if (!m_bOperational) return;

		m_bOperational = false;
		Replication.BumpMe();

		BH_IED_Utils.BH_Log("[BH_SignalTower] Antena SABOTEADA.");
		BH_ShowHint("Antena saboteada. Señal móvil cortada en la zona.");
	}

	void BH_Repair()
	{
		if (!BH_IED_Utils.BH_IsServer()) return;
		if (m_bOperational) return;

		m_bOperational = true;
		Replication.BumpMe();

		BH_IED_Utils.BH_Log("[BH_SignalTower] Antena REPARADA.");
		BH_ShowHint("Antena reparada. Señal móvil restaurada en la zona.");
	}

	// --------------------------------------------------------
	// Getters
	// --------------------------------------------------------

	bool  BH_IsOperational()     { return m_bOperational; }
	float BH_GetCoverageRadius() { return m_fCoverageRadius; }

	// --------------------------------------------------------
	// Cobertura de señal — estático
	// --------------------------------------------------------

	static bool BH_HasSignalCoverage(vector pos)
	{
		if (s_aTowers.Count() == 0)
		{
			BH_IED_Utils.BH_Log("[BH_SignalTower] No hay antenas en el mapa — cobertura total.");
			return true;
		}

		BH_IED_Utils.BH_Log(string.Format("[BH_SignalTower] Comprobando cobertura. Antenas registradas: %1", s_aTowers.Count()));

		foreach (BH_SignalTower tower : s_aTowers)
		{
			if (!tower) continue;

			IEntity towerEnt = tower.GetOwner();
			if (!towerEnt) continue;

			float dist = vector.Distance(towerEnt.GetOrigin(), pos);
			bool operational = tower.BH_IsOperational();

			BH_IED_Utils.BH_Log(string.Format("[BH_SignalTower] Antena: operativa=%1, dist=%2m, radio=%3m", operational, dist, tower.BH_GetCoverageRadius()));

			if (!operational) continue;

			if (dist <= tower.BH_GetCoverageRadius())
			{
				BH_IED_Utils.BH_Log("[BH_SignalTower] Cobertura encontrada.");
				return true;
			}
		}

		BH_IED_Utils.BH_Log("[BH_SignalTower] SIN cobertura.");
		return false;
	}

	// --------------------------------------------------------
	// Callback de replicación — clientes remotos
	// --------------------------------------------------------

	protected void BH_OnTowerStateChanged()
	{
		if (!m_bOperational)
			BH_IED_Utils.BH_Log("[BH_SignalTower][CLIENT] Antena SABOTEADA.");
		else
			BH_IED_Utils.BH_Log("[BH_SignalTower][CLIENT] Antena OPERATIVA.");
	}

	// --------------------------------------------------------
	// Helpers
	// --------------------------------------------------------

	protected void BH_ShowHint(string message)
	{
		SCR_HintManagerComponent hintMgr = SCR_HintManagerComponent.GetInstance();
		if (hintMgr)
			hintMgr.ShowCustomHint(message, "Antena", 4.0);
	}
}