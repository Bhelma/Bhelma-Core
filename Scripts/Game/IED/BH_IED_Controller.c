// ============================================================
//  BH_IED_Controller.c
//  Autor: BH Mod
//  Descripción: Controlador principal del sistema IED.
//
//  SONIDO DE EXPLOSIÓN:
//  Usa SoundComponent con el .acp configurado en Workbench.
//  El atributo m_sExplosionSoundEvent debe coincidir con el
//  nombre del nodo Sound dentro del .acp (ej: "SOUND_EXPLOSION").
// ============================================================

// --- Enums --------------------------------------------------

enum BH_EIEDState
{
	BH_IED_ARMED      = 0,
	BH_IED_TRIGGERED  = 1,
	BH_IED_DISARMED   = 2,
	BH_IED_EXPLODED   = 3
}

enum BH_EWireColor
{
	BH_WIRE_RED    = 0,
	BH_WIRE_GREEN  = 1,
	BH_WIRE_BLUE   = 2,
	BH_WIRE_YELLOW = 3
}

// --- Clase principal ----------------------------------------

class BH_IED_ControllerClass : ScriptComponentClass {}

class BH_IED_Controller : ScriptComponent
{
	// --------------------------------------------------------
	// Variables replicadas
	// --------------------------------------------------------

	[RplProp(onRplName: "BH_OnStateChanged")]
	protected BH_EIEDState m_eState;

	[RplProp()]
	protected int m_iCorrectWire;

	// Posición replicada para que clientes remotos sepan dónde
	// reproducir efectos si la entidad ya fue destruida
	[RplProp()]
	protected vector m_vExplosionPos;

	// --------------------------------------------------------
	// Variables configurables desde Workbench
	// --------------------------------------------------------

	[Attribute("2", UIWidgets.Slider, "Número de cables visibles (2-4)", "2 4 1")]
	protected int m_iWireCount;

	[Attribute("0", UIWidgets.CheckBox, "IED falsa (no explota, cualquier cable la desactiva)")]
	protected bool m_bIsFake;

	[Attribute("0", UIWidgets.CheckBox, "Activar logs de debug en consola")]
	protected bool m_bDebug;

	[Attribute("5", UIWidgets.Slider, "Segundos de cuenta atrás tras cable incorrecto", "1 15 1")]
	protected float m_fCountdownTime;

	[Attribute("3", UIWidgets.Slider, "Delay en segundos tras ring antes de explotar", "1 30 1")]
	protected float m_fRingDelay;

	[Attribute("8", UIWidgets.Slider, "Radio de daño de la explosión (metros)", "1 30 1")]
	protected float m_fExplosionRadius;

	[Attribute("1000", UIWidgets.Slider, "Daño máximo de la explosión", "100 5000 100")]
	protected float m_fExplosionDamage;

	// Nombre del evento de sonido dentro del .acp asignado al SoundComponent.
	// Debe coincidir EXACTAMENTE con el nombre del nodo Sound del .acp.
	[Attribute("SOUND_EXPLOSION", UIWidgets.EditBox, "Nombre del evento de sonido de explosión (nodo Sound del .acp)")]
	protected string m_sExplosionSoundEvent;

	[Attribute("BH_Ring_Sound", UIWidgets.EditBox, "Sonido del ring de móvil (nodo Sound del .acp)")]
	protected string m_sRingSound;

	// --------------------------------------------------------
	// Variables internas
	// --------------------------------------------------------

	protected float  m_fCountdownRemaining;
	protected bool   m_bCountdownActive;
	protected bool   m_bIsVBIED;

	// --------------------------------------------------------
	// Init
	// --------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!BH_IED_Utils.BH_IsServer())
			return;

		// Activar debug global si este componente lo tiene marcado
		if (m_bDebug)
			BH_IED_Utils.BH_SetDebug(true);

		m_eState = BH_EIEDState.BH_IED_ARMED;

		Math.Randomize(-1);
		m_iCorrectWire = Math.RandomInt(0, m_iWireCount);

		Replication.BumpMe();

		BH_IED_Utils.BH_Log(string.Format("[BH_IED] IED inicializada. Cable correcto: %1 (de %2 cables)", BH_IED_Utils.BH_WireColorToString(m_iCorrectWire), m_iWireCount));
	}

	// --------------------------------------------------------
	// API pública — llamada desde ProximityTrigger
	// --------------------------------------------------------

	void BH_TriggerByProximity()
	{
		if (!BH_IED_Utils.BH_IsServer()) return;
		if (m_eState != BH_EIEDState.BH_IED_ARMED) return;

		// IED falsa — nunca detona por proximidad
		if (m_bIsFake) return;

		BH_IED_Utils.BH_Log("[BH_IED] Trigger por proximidad activado.");
		BH_SetState(BH_EIEDState.BH_IED_TRIGGERED);
		BH_PlaySoundGlobal(m_sRingSound);

		GetGame().GetCallqueue().CallLater(BH_Explode, (m_fRingDelay * 1000), false);
	}

	bool BH_TryDisarm(int wireColorChosen)
	{
		if (!BH_IED_Utils.BH_IsServer()) return false;
		if (m_eState != BH_EIEDState.BH_IED_ARMED) return false;

		// IED falsa — cualquier cable la desactiva
		if (m_bIsFake)
		{
			BH_SetState(BH_EIEDState.BH_IED_DISARMED);
			BH_IED_Utils.BH_Log("[BH_IED] IED FALSA desactivada.");
			if (!m_bIsVBIED)
				GetGame().GetCallqueue().CallLater(BH_DestroyOwner, 500, false);
			return true;
		}

		if (wireColorChosen == m_iCorrectWire)
		{
			BH_SetState(BH_EIEDState.BH_IED_DISARMED);
			BH_IED_Utils.BH_Log("[BH_IED] IED desactivada correctamente.");
			if (!m_bIsVBIED)
				GetGame().GetCallqueue().CallLater(BH_DestroyOwner, 500, false);
			// VBIED: el vehículo queda intacto y desactivado (no se destruye)
			return true;
		}
		else
		{
			BH_IED_Utils.BH_Log("[BH_IED] Cable incorrecto. Iniciando cuenta atrás.");
			BH_SetState(BH_EIEDState.BH_IED_TRIGGERED);
			BH_PlaySoundGlobal(m_sRingSound);
			GetGame().GetCallqueue().CallLater(BH_Explode, (m_fCountdownTime * 1000), false);
			return false;
		}
	}

	// --------------------------------------------------------
	// Explosión
	// --------------------------------------------------------

	void BH_Explode()
	{
		if (!BH_IED_Utils.BH_IsServer()) return;
		if (m_eState == BH_EIEDState.BH_IED_EXPLODED) return;

		IEntity owner = GetOwner();
		if (!owner) return;

		vector explosionPos = owner.GetOrigin();
		m_vExplosionPos = explosionPos;

		BH_IED_Utils.BH_Log(string.Format("[BH_IED] EXPLOSIÓN en posición: %1", explosionPos.ToString()));

		// 1) Daño radial — solo servidor
		BH_ApplyExplosionDamage(explosionPos);

		// 2) Efectos — para host local
		BH_PlayExplosionEffects(explosionPos);

		// 3) Cambiar estado — dispara BH_OnClientExploded en clientes remotos
		BH_SetState(BH_EIEDState.BH_IED_EXPLODED);

		// 4) Destruir según tipo
		if (m_bIsVBIED)
		{
			// VBIED: destruir el vehículo via DamageManager (chasis quemado)
			BH_VBIED vbied = BH_VBIED.Cast(owner.FindComponent(BH_VBIED));
			if (vbied)
			{
				GetGame().GetCallqueue().CallLater(vbied.BH_DestroyVehicle, 500, false);
				BH_IED_Utils.BH_Log("[BH_IED] VBIED: destruyendo vehículo via DamageManager.");
			}
		}
		else
		{
			// IED normal: eliminar entidad con delay para replicación
			GetGame().GetCallqueue().CallLater(BH_DestroyOwner, 2000, false);
		}
	}

	// --------------------------------------------------------
	// Efectos de explosión (partículas + sonido)
	// --------------------------------------------------------

	protected void BH_PlayExplosionEffects(vector pos)
	{
		// --- Partículas ---
		ParticleEffectEntitySpawnParams spawnParams = new ParticleEffectEntitySpawnParams();
		Math3D.MatrixIdentity4(spawnParams.Transform);
		spawnParams.Transform[3] = pos;
		ParticleEffectEntity.SpawnParticleEffect("{8105B9A5EA395C54}Particles/Weapon/Explosion_M15_AT_Mine.ptc", spawnParams);

		// --- Sonido via SCR_SoundManagerModule (Reforger 1.6+) ---
		// Independiente de la entidad — no se corta al destruirla
		BH_PlayExplosionSoundAtPos(pos);
	}

	protected void BH_PlayExplosionSoundAtPos(vector pos)
	{
		IEntity owner = GetOwner();
		if (!owner) return;

		SoundComponent soundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if (soundComp)
		{
			soundComp.SoundEvent(m_sExplosionSoundEvent);
			BH_IED_Utils.BH_Log("[BH_IED] Sonido explosión reproducido.");
		}
	}

	// --------------------------------------------------------
	// Daño por explosión
	// --------------------------------------------------------

	protected ref array<IEntity> m_aDamagedEntities;

	protected void BH_ApplyExplosionDamage(vector pos)
	{
		m_aDamagedEntities = new array<IEntity>();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, m_fExplosionRadius, BH_QueryCallback, null, EQueryEntitiesFlags.ALL);
		m_aDamagedEntities = null;
	}

	protected bool BH_QueryCallback(IEntity ent)
	{
		if (!ent) return true;
		if (!m_aDamagedEntities) return true;

		IEntity root = ent.GetRootParent();
		if (!root) root = ent;

		if (m_aDamagedEntities.Contains(root)) return true;

		IEntity owner = GetOwner();
		if (!owner) return true;

		// IED normal: no dañarse a sí misma
		// VBIED: sí dañar el propio vehículo
		if (root == owner && !m_bIsVBIED) return true;

		vector pos       = owner.GetOrigin();
		float  dist      = vector.Distance(pos, root.GetOrigin());
		float  damageMod = 1.0 - (dist / m_fExplosionRadius);
		float  finalDmg  = m_fExplosionDamage * damageMod;

		if (finalDmg <= 0) return true;

		DamageManagerComponent dmgMgr = DamageManagerComponent.Cast(root.FindComponent(DamageManagerComponent));
		if (dmgMgr)
		{
			HitZone hitZone = dmgMgr.GetDefaultHitZone();
			if (hitZone)
			{
				BaseDamageContext dmgContext = new BaseDamageContext();
				dmgContext.damageValue = finalDmg;
				dmgContext.struckHitZone = hitZone;
				dmgMgr.HandleDamage(dmgContext);
				BH_IED_Utils.BH_Log(string.Format("[BH_IED] Daño %1 aplicado a %2 (dist: %3m)", finalDmg, root.GetName(), dist));
			}
		}

		m_aDamagedEntities.Insert(root);
		return true;
	}

	protected void BH_DestroyOwner()
	{
		IEntity owner = GetOwner();
		if (!owner) return;

		owner.ClearFlags(EntityFlags.ACTIVE, false);
		SCR_EntityHelper.DeleteEntityAndChildren(owner);
	}

	// --------------------------------------------------------
	// Gestión de estado y replicación
	// --------------------------------------------------------

	protected void BH_SetState(BH_EIEDState newState)
	{
		m_eState = newState;
		Replication.BumpMe();
	}

	// Se ejecuta SOLO en clientes remotos (proxies)
	protected void BH_OnStateChanged()
	{
		switch (m_eState)
		{
			case BH_EIEDState.BH_IED_TRIGGERED:
				BH_OnClientTriggered();
				break;

			case BH_EIEDState.BH_IED_DISARMED:
				BH_OnClientDisarmed();
				break;

			case BH_EIEDState.BH_IED_EXPLODED:
				BH_OnClientExploded();
				break;
		}
	}

	// --------------------------------------------------------
	// Callbacks de cliente remoto
	// --------------------------------------------------------

	protected void BH_OnClientTriggered()
	{
		BH_PlaySoundLocal(m_sRingSound);
		BH_IED_Utils.BH_Log("[BH_IED][CLIENT] Estado: TRIGGERED");
	}

	protected void BH_OnClientDisarmed()
	{
		BH_IED_Utils.BH_Log("[BH_IED][CLIENT] Estado: DISARMED");
	}

	protected void BH_OnClientExploded()
	{
		BH_IED_Utils.BH_Log("[BH_IED][CLIENT] Estado: EXPLODED");

		// Usar posición replicada (más fiable)
		vector pos = m_vExplosionPos;

		if (pos == vector.Zero)
		{
			IEntity owner = GetOwner();
			if (owner)
				pos = owner.GetOrigin();
		}

		if (pos != vector.Zero)
			BH_PlayExplosionEffects(pos);
	}

	// --------------------------------------------------------
	// Getters
	// --------------------------------------------------------

	BH_EIEDState BH_GetState()         { return m_eState; }
	int          BH_GetCorrectWire()   { return m_iCorrectWire; }
	float        BH_GetCountdownTime() { return m_fCountdownTime; }
	bool         BH_IsFake()           { return m_bIsFake; }
	bool         BH_IsVBIED()          { return m_bIsVBIED; }

	void BH_SetIsVBIED(bool isVBIED)   { m_bIsVBIED = isVBIED; }

	// --------------------------------------------------------
	// Helpers de sonido
	// Busca SoundComponent genérico y SCR_VehicleSoundComponent
	// para compatibilidad con IEDs normales y VBIEDs
	// --------------------------------------------------------

	protected void BH_PlaySoundGlobal(string soundPath)
	{
		if (soundPath.IsEmpty()) return;

		IEntity owner = GetOwner();
		if (!owner) return;

		SoundComponent soundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if (soundComp)
			soundComp.SoundEvent(soundPath);
	}

	protected void BH_PlaySoundLocal(string soundPath)
	{
		if (soundPath.IsEmpty()) return;

		IEntity owner = GetOwner();
		if (!owner) return;

		SoundComponent soundComp = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		if (soundComp)
			soundComp.SoundEvent(soundPath);
	}
}

// ============================================================
//  BH_IED_Utils
// ============================================================

class BH_IED_Utils
{
	// Flag global de debug — se activa desde cualquier componente
	// que tenga el checkbox m_bDebug marcado
	protected static bool s_bDebugEnabled = false;

	static bool BH_IsServer()
	{
		return Replication.IsServer();
	}

	// Activar/desactivar debug globalmente
	static void BH_SetDebug(bool enabled)
	{
		s_bDebugEnabled = enabled;
	}

	// Imprimir solo si debug está activo
	static void BH_Log(string message)
	{
		if (s_bDebugEnabled)
			Print(message);
	}

	static string BH_WireColorToString(int wire)
	{
		if (wire == BH_EWireColor.BH_WIRE_RED)    return "ROJO";
		if (wire == BH_EWireColor.BH_WIRE_GREEN)  return "VERDE";
		if (wire == BH_EWireColor.BH_WIRE_BLUE)   return "AZUL";
		if (wire == BH_EWireColor.BH_WIRE_YELLOW) return "AMARILLO";
		return "DESCONOCIDO";
	}

	static void BH_SpawnParticleEffect(string effectName, vector pos)
	{
		ParticleEffectEntitySpawnParams spawnParams = new ParticleEffectEntitySpawnParams();
		spawnParams.Transform[3] = pos;
		ParticleEffectEntity.SpawnParticleEffect(effectName, spawnParams);
	}
}