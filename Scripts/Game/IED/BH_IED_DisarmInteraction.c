// ============================================================
//  BH_IED_DisarmInteraction.c
//  Autor: BH Mod
//  Descripción: Gestiona la desactivación de la IED mediante
//               acciones nativas de Reforger. Genera entre
//               2 y 4 acciones de corte de cable, solo una
//               es la correcta. El jugador debe llevar los
//               alicates en la mano para que aparezcan.
//  [FIX] BUG 4 — Feedback replicado a clientes remotos
//  [FIX] BUG 7 — Detección de alicates por componente
//  [FIX] Hints y sonidos se ejecutan en servidor (host local)
//        Y TAMBIÉN se replican a clientes remotos via RplProp
// ============================================================

// ============================================================
//  Acción: Cortar cable ROJO
// ============================================================

class BH_DisarmAction_Red : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		BH_IED_DisarmInteraction disarm = BH_GetDisarm();
		if (!disarm) return false;
		return disarm.BH_CanShowActions(user) && disarm.BH_IsWireEnabled(BH_EWireColor.BH_WIRE_RED);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_IED_DisarmInteraction disarm = BH_GetDisarm();
		if (disarm) disarm.BH_TryCutWire(BH_EWireColor.BH_WIRE_RED, pUserEntity);
	}

	protected BH_IED_DisarmInteraction BH_GetDisarm()
	{
		IEntity owner = GetOwner();
		if (!owner) return null;
		return BH_IED_DisarmInteraction.Cast(owner.FindComponent(BH_IED_DisarmInteraction));
	}
}

// ============================================================
//  Acción: Cortar cable VERDE
// ============================================================

class BH_DisarmAction_Green : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		BH_IED_DisarmInteraction disarm = BH_GetDisarm();
		if (!disarm) return false;
		return disarm.BH_CanShowActions(user) && disarm.BH_IsWireEnabled(BH_EWireColor.BH_WIRE_GREEN);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_IED_DisarmInteraction disarm = BH_GetDisarm();
		if (disarm) disarm.BH_TryCutWire(BH_EWireColor.BH_WIRE_GREEN, pUserEntity);
	}

	protected BH_IED_DisarmInteraction BH_GetDisarm()
	{
		IEntity owner = GetOwner();
		if (!owner) return null;
		return BH_IED_DisarmInteraction.Cast(owner.FindComponent(BH_IED_DisarmInteraction));
	}
}

// ============================================================
//  Acción: Cortar cable AZUL
// ============================================================

class BH_DisarmAction_Blue : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		BH_IED_DisarmInteraction disarm = BH_GetDisarm();
		if (!disarm) return false;
		return disarm.BH_CanShowActions(user) && disarm.BH_IsWireEnabled(BH_EWireColor.BH_WIRE_BLUE);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_IED_DisarmInteraction disarm = BH_GetDisarm();
		if (disarm) disarm.BH_TryCutWire(BH_EWireColor.BH_WIRE_BLUE, pUserEntity);
	}

	protected BH_IED_DisarmInteraction BH_GetDisarm()
	{
		IEntity owner = GetOwner();
		if (!owner) return null;
		return BH_IED_DisarmInteraction.Cast(owner.FindComponent(BH_IED_DisarmInteraction));
	}
}

// ============================================================
//  Acción: Cortar cable AMARILLO
// ============================================================

class BH_DisarmAction_Yellow : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		BH_IED_DisarmInteraction disarm = BH_GetDisarm();
		if (!disarm) return false;
		return disarm.BH_CanShowActions(user) && disarm.BH_IsWireEnabled(BH_EWireColor.BH_WIRE_YELLOW);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		BH_IED_DisarmInteraction disarm = BH_GetDisarm();
		if (disarm) disarm.BH_TryCutWire(BH_EWireColor.BH_WIRE_YELLOW, pUserEntity);
	}

	protected BH_IED_DisarmInteraction BH_GetDisarm()
	{
		IEntity owner = GetOwner();
		if (!owner) return null;
		return BH_IED_DisarmInteraction.Cast(owner.FindComponent(BH_IED_DisarmInteraction));
	}
}

// ============================================================
//  BH_IED_DisarmInteraction — componente principal
// ============================================================

class BH_IED_DisarmInteractionClass : ScriptComponentClass {}

class BH_IED_DisarmInteraction : ScriptComponent
{
	// --------------------------------------------------------
	// Atributos configurables desde Workbench
	// --------------------------------------------------------

	[Attribute("2", UIWidgets.Slider, "Número de cables visibles (2-4)", "2 4 1")]
	protected int m_iWireCount;

	[Attribute("sounds/BH_IED/BH_ied_click_correct.wav", UIWidgets.EditBox, "Sonido cable correcto")]
	protected string m_sSoundCorrect;

	[Attribute("sounds/BH_IED/BH_ied_click_wrong.wav", UIWidgets.EditBox, "Sonido cable incorrecto")]
	protected string m_sSoundWrong;

	// --------------------------------------------------------
	// [FIX] BUG 4 — Variable replicada para feedback en clientes remotos
	// -1 = sin resultado, 0 = fallo, 1 = éxito
	// --------------------------------------------------------

	[RplProp(onRplName: "BH_OnDisarmResultChanged")]
	protected int m_iLastDisarmResult;

	// --------------------------------------------------------
	// Variables internas
	// --------------------------------------------------------

	protected BH_IED_Controller m_Controller;

	// --------------------------------------------------------
	// Init
	// --------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_iLastDisarmResult = -1;

		m_Controller = BH_IED_Controller.Cast(owner.FindComponent(BH_IED_Controller));
		if (!m_Controller)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Disarm] ERROR: No se encontró BH_IED_Controller.");
			return;
		}
	}

	// --------------------------------------------------------
	// Comprueba si las acciones deben mostrarse
	// --------------------------------------------------------

	bool BH_CanShowActions(IEntity user)
	{
		if (!m_Controller) return false;
		if (m_Controller.BH_GetState() != BH_EIEDState.BH_IED_ARMED) return false;
		return BH_PlayerHasWireCutter(user);
	}

	// --------------------------------------------------------
	// Comprueba si un cable concreto está habilitado
	// según el número de cables configurado
	// --------------------------------------------------------

	bool BH_IsWireEnabled(int wireColor)
	{
		// Siempre mostrar ROJO y VERDE (mínimo 2)
		if (wireColor == BH_EWireColor.BH_WIRE_RED)    return true;
		if (wireColor == BH_EWireColor.BH_WIRE_GREEN)  return true;

		// AZUL solo si hay 3 o más cables
		if (wireColor == BH_EWireColor.BH_WIRE_BLUE)   return m_iWireCount >= 3;

		// AMARILLO solo si hay 4 cables
		if (wireColor == BH_EWireColor.BH_WIRE_YELLOW) return m_iWireCount >= 4;

		return false;
	}

	// --------------------------------------------------------
	// Lógica de corte de cable — llamado desde las acciones
	// (PerformAction se ejecuta en servidor en Reforger)
	// --------------------------------------------------------

	void BH_TryCutWire(int wireColor, IEntity user)
	{
		if (!m_Controller) return;
		if (m_Controller.BH_GetState() != BH_EIEDState.BH_IED_ARMED) return;

		BH_IED_Utils.BH_Log(string.Format("[BH_IED_Disarm] Jugador cortó cable: %1", BH_IED_Utils.BH_WireColorToString(wireColor)));

		bool success = m_Controller.BH_TryDisarm(wireColor);

		// Feedback INMEDIATO en servidor (funciona para host local y listen server)
		if (success)
		{
			BH_PlaySound(m_sSoundCorrect);
			BH_ShowHint("IED desactivada correctamente.");
		}
		else
		{
			BH_PlaySound(m_sSoundWrong);
			BH_ShowHint("¡Cable incorrecto! ¡ALEJATE!");
		}

		// [FIX] BUG 4 — Replicar resultado a clientes remotos (dedicado)
		if (success)
			m_iLastDisarmResult = 1;
		else
			m_iLastDisarmResult = 0;

		Replication.BumpMe();
	}

	// --------------------------------------------------------
	// [FIX] BUG 4 — Callback de replicación del resultado
	// Se ejecuta SOLO en clientes remotos (dedicado)
	// El host local ya recibió feedback arriba en BH_TryCutWire
	// --------------------------------------------------------

	protected void BH_OnDisarmResultChanged()
	{
		if (m_iLastDisarmResult == 1)
		{
			BH_PlaySound(m_sSoundCorrect);
			BH_ShowHint("IED desactivada correctamente.");
		}
		else if (m_iLastDisarmResult == 0)
		{
			BH_PlaySound(m_sSoundWrong);
			BH_ShowHint("¡Cable incorrecto! ¡ALEJATE!");
		}
	}

	// --------------------------------------------------------
	// [FIX] BUG 7 — Comprueba si el jugador lleva WireCutter
	// Ahora busca por componente BH_WireCutter, no por nombre
	// --------------------------------------------------------

	protected bool BH_PlayerHasWireCutter(IEntity playerEntity)
	{
		CharacterControllerComponent charCtrl = CharacterControllerComponent.Cast(
			playerEntity.FindComponent(CharacterControllerComponent)
		);
		if (!charCtrl) return false;

		// Comprobar mano izquierda (gadget slot)
		IEntity heldItem = charCtrl.GetAttachedGadgetAtLeftHandSlot();
		if (heldItem)
		{
			BH_WireCutter wireCutter = BH_WireCutter.Cast(heldItem.FindComponent(BH_WireCutter));
			if (wireCutter && wireCutter.BH_CanDisarm())
				return true;
		}

		// Comprobar mano derecha
		heldItem = charCtrl.GetRightHandItem();
		if (heldItem)
		{
			BH_WireCutter wireCutter = BH_WireCutter.Cast(heldItem.FindComponent(BH_WireCutter));
			if (wireCutter && wireCutter.BH_CanDisarm())
				return true;
		}

		return false;
	}

	// --------------------------------------------------------
	// Helpers
	// --------------------------------------------------------

	protected void BH_ShowHint(string message)
	{
		SCR_HintManagerComponent hintMgr = SCR_HintManagerComponent.GetInstance();
		if (hintMgr)
			hintMgr.ShowCustomHint(message, "IED", 3.0);
	}

	protected void BH_PlaySound(string soundPath)
	{
		IEntity owner = GetOwner();
		if (!owner) return;

		SoundComponent soundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if (soundComp)
			soundComp.SoundEvent(soundPath);
	}
}