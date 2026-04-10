// ============================================================
//  BH_VehicleJammer.c
//  Autor: BH Mod
//  Descripción: Componente que se añade a vehículos para
//               inhibir la señal de detonación de IEDs.
//               El jugador lo activa/desactiva manualmente.
//
//  El estado del beep de proximidad se replica a clientes
//  para que suene en servidor dedicado.
//  Patrón dual: servidor ejecuta sonido (host local) +
//  callback replica a clientes remotos.
// ============================================================

class BH_VehicleJammerClass : ScriptComponentClass {}

class BH_VehicleJammer : ScriptComponent
{
	// --------------------------------------------------------
	// Variables replicadas
	// --------------------------------------------------------

	[RplProp(onRplName: "BH_OnJammerStateChanged")]
	protected bool m_bActive;

	[RplProp(onRplName: "BH_OnBeepStateChanged")]
	protected bool m_bBeeping;

	// --------------------------------------------------------
	// Atributos configurables desde Workbench
	// --------------------------------------------------------

	[Attribute("30", UIWidgets.Slider, "Radio de inhibición (metros)", "5 100 5")]
	protected float m_fJammerRadius;

	[Attribute("BH_Activated_Sound", UIWidgets.EditBox, "Nombre del evento de sonido al activar (nodo Sound del .acp)")]
	protected string m_sSoundOn;

	[Attribute("BH_Desactivated_Sound", UIWidgets.EditBox, "Nombre del evento de sonido al desactivar (nodo Sound del .acp)")]
	protected string m_sSoundOff;

	[Attribute("BH_Sound_Beep", UIWidgets.EditBox, "Nombre del evento de sonido de alerta por IED cercana (nodo Sound del .acp)")]
	protected string m_sSoundBeep;

	[Attribute("1.5", UIWidgets.Slider, "Intervalo de repetición del beep (segundos)", "0.5 5 0.1")]
	protected float m_fBeepInterval;

	// --------------------------------------------------------
	// Init
	// --------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_bActive  = false;
		m_bBeeping = false;
	}

	// --------------------------------------------------------
	// Cleanup
	// --------------------------------------------------------

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(BH_RepeatBeep);
		GetGame().GetCallqueue().Remove(BH_RepeatBeepClient);
		super.OnDelete(owner);
	}

	// --------------------------------------------------------
	// API pública — jammer on/off
	// --------------------------------------------------------

	void BH_SetActive(bool active)
	{
		if (!BH_IED_Utils.BH_IsServer()) return;
		if (m_bActive == active) return;

		m_bActive = active;
		Replication.BumpMe();

		// Sonido en servidor (host local)
		if (active)
		{
			BH_IED_Utils.BH_Log("[BH_Jammer] Inhibidor ACTIVADO.");
			BH_PlaySoundEvent(m_sSoundOn);
		}
		else
		{
			BH_IED_Utils.BH_Log("[BH_Jammer] Inhibidor DESACTIVADO.");
			BH_PlaySoundEvent(m_sSoundOff);

			// Si se apaga el jammer, parar el beep también
			if (m_bBeeping)
				BH_SetBeeping(false);
		}
	}

	void BH_Toggle()
	{
		BH_SetActive(!m_bActive);
	}

	// --------------------------------------------------------
	// API pública — beep de proximidad a IED
	// Llamado por BH_IED_ProximityTrigger desde el servidor
	// --------------------------------------------------------

	void BH_SetBeeping(bool beeping)
	{
		if (!BH_IED_Utils.BH_IsServer()) return;
		if (m_bBeeping == beeping) return;

		m_bBeeping = beeping;
		Replication.BumpMe();

		// Gestionar timer de repetición en servidor (host local)
		if (beeping)
		{
			BH_IED_Utils.BH_Log("[BH_Jammer] Beep de proximidad ACTIVADO.");
			BH_PlaySoundEvent(m_sSoundBeep);
			// Iniciar repetición
			GetGame().GetCallqueue().CallLater(BH_RepeatBeep, m_fBeepInterval * 1000, true);
		}
		else
		{
			BH_IED_Utils.BH_Log("[BH_Jammer] Beep de proximidad DESACTIVADO.");
			GetGame().GetCallqueue().Remove(BH_RepeatBeep);
		}
	}

	// Timer que repite el beep en servidor (host local)
	protected void BH_RepeatBeep()
	{
		if (!m_bBeeping)
		{
			GetGame().GetCallqueue().Remove(BH_RepeatBeep);
			return;
		}

		BH_PlaySoundEvent(m_sSoundBeep);
	}

	// --------------------------------------------------------
	// Getters
	// --------------------------------------------------------

	bool  BH_IsActive()  { return m_bActive; }
	bool  BH_IsBeeping() { return m_bBeeping; }
	float BH_GetRadius() { return m_fJammerRadius; }

	// --------------------------------------------------------
	// Callback de replicación — jammer on/off (clientes remotos)
	// --------------------------------------------------------

	protected void BH_OnJammerStateChanged()
	{
		if (m_bActive)
			BH_PlaySoundEvent(m_sSoundOn);
		else
			BH_PlaySoundEvent(m_sSoundOff);

		BH_IED_Utils.BH_Log(string.Format("[BH_Jammer][CLIENT] Estado inhibidor: %1", m_bActive));
	}

	// --------------------------------------------------------
	// Callback de replicación — beep (clientes remotos)
	// --------------------------------------------------------

	protected void BH_OnBeepStateChanged()
	{
		if (m_bBeeping)
		{
			BH_IED_Utils.BH_Log("[BH_Jammer][CLIENT] Beep de proximidad ACTIVADO.");
			BH_PlaySoundEvent(m_sSoundBeep);
			// Iniciar repetición en cliente
			GetGame().GetCallqueue().Remove(BH_RepeatBeepClient);
			GetGame().GetCallqueue().CallLater(BH_RepeatBeepClient, m_fBeepInterval * 1000, true);
		}
		else
		{
			BH_IED_Utils.BH_Log("[BH_Jammer][CLIENT] Beep de proximidad DESACTIVADO.");
			GetGame().GetCallqueue().Remove(BH_RepeatBeepClient);
		}
	}

	// Timer que repite el beep en clientes remotos
	protected void BH_RepeatBeepClient()
	{
		if (!m_bBeeping)
		{
			GetGame().GetCallqueue().Remove(BH_RepeatBeepClient);
			return;
		}

		BH_PlaySoundEvent(m_sSoundBeep);
	}

	// --------------------------------------------------------
	// Helper de sonido
	// --------------------------------------------------------

	protected void BH_PlaySoundEvent(string eventName)
	{
		if (eventName.IsEmpty()) return;

		IEntity owner = GetOwner();
		if (!owner) return;

		SoundComponent soundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if (soundComp)
			soundComp.SoundEvent(eventName);
	}
}