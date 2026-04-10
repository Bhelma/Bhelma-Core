// ============================================================================
// BH_HoldBreath.c v9
// Sistema de "Mantener Respiración" para Arma Reforger
//
// MODDED CLASS - se inyecta automáticamente en cualquier misión.
//
// Comportamiento realista:
// - Al activar: el sway se reduce drásticamente (aguantas respiración)
// - Con el tiempo: el sway vuelve progresivamente (te cuesta más)
// - Al soltar: spike de sway extra (recuperas el aliento, jadeo)
// - Después del spike: vuelve a la normalidad
// ============================================================================

modded class SCR_CharacterControllerComponent
{
	// ========================================================================
	// CONFIGURACIÓN - Modifica estos valores para ajustar el comportamiento
	// ========================================================================
	
	// Segundos máximos que puedes aguantar la respiración (a estamina completa).
	// A menor estamina, el tiempo se reduce proporcionalmente.
	// Ejemplo: con 0.5 de estamina → 15 * 0.5 = 7.5 segundos
	protected static const float BH_MAX_TIME = 15.0;
	
	// Estamina mínima necesaria para poder activar el hold breath.
	// Rango: 0.0 (sin mínimo) a 1.0 (necesitas estamina completa).
	// 0.35 = necesitas al menos un 35% de estamina.
	protected static const float BH_MIN_STAMINA = 0.35;
	
	// Cuánta estamina pierdes al soltar. Se multiplica por el ratio
	// de tiempo que aguantaste. Si aguantas el 100% del tiempo → pierdes 0.20.
	// Si aguantas la mitad → pierdes 0.10.
	protected static const float BH_STAMINA_COST = 0.20;
	
	// Segundos de espera (cooldown) antes de poder volver a aguantar
	// la respiración después de soltar.
	protected static const float BH_COOLDOWN = 3.0;
	
	// Segundos iniciales donde el arma está TOTALMENTE inmóvil (congelada).
	// Durante esta fase el sway es 0 absoluto, sin ningún temblor.
	// Después de esta fase, entra en la fase estable con micro-temblor.
	// Ejemplo: 3.0 = los 3 primeros segundos el arma no se mueve nada.
	protected static const float BH_FREEZE_TIME = 3.0;
	
	// Porcentaje del tiempo total donde el arma está completamente estable.
	// Rango: 0.0 a 1.0. Ejemplo: 0.6 = el 60% del tiempo el arma está
	// casi sin moverse. El 40% restante el sway vuelve progresivamente.
	// Con 15 seg → 9 seg estable + 6 seg degradándose.
	protected static const float BH_STABLE_PHASE = 0.6;
	
	// Segundos que dura el "jadeo" después de soltar la respiración.
	// Durante este tiempo la estamina se drena un poco extra y el
	// personaje se está recuperando.
	protected static const float BH_EXHALE_SPIKE_DURATION = 3.0;
	
	// Intensidad del jadeo al soltar. Multiplicador que escala según
	// cuánto tiempo aguantaste. Más alto = más penalización al soltar.
	protected static const float BH_EXHALE_SPIKE_INTENSITY = 2.0;
	
	// Máxima reducción de sway (0.0 = no reduce nada, 1.0 = elimina todo).
	// 0.95 deja un micro-temblor residual para que no se sienta artificial.
	// Pon 1.0 si quieres que sea completamente inmóvil en la fase estable.
	protected static const float BH_MAX_EFFECTIVENESS = 0.95;
	
	// Nombre de la acción de input (debe coincidir con el .conf)
	protected static const string BH_INPUT_ACTION = "BH_HoldBreath";
	
	// Intervalo de actualización en milisegundos (20ms ≈ 50 ticks/seg).
	// Bajar este valor = más suave pero más coste de CPU.
	protected static const int BH_UPDATE_INTERVAL = 20;
	
	// ========================================================================
	// ESTADO INTERNO
	// ========================================================================
	
	protected bool m_bBH_IsHolding;
	protected float m_fBH_HoldTimer;
	protected float m_fBH_CooldownTimer;
	protected float m_fBH_StaminaAtStart;
	protected CharacterStaminaComponent m_pBH_StaminaComp;
	protected CharacterInputContext m_pBH_InputCtx;
	protected bool m_bBH_Running;
	protected vector m_vBH_FrozenAimAngles;
	
	// Estado del spike post-soltar
	protected bool m_bBH_ExhaleSpike;
	protected float m_fBH_ExhaleSpikeTimer;
	protected float m_fBH_ExhaleSpikeRatio;
	
	// ========================================================================
	// LIFECYCLE
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		
		if (System.IsConsoleApp())
			return;
		
		if (!m_bBH_Running)
		{
			m_bBH_Running = true;
			GetGame().GetCallqueue().CallLater(BH_Update, BH_UPDATE_INTERVAL, true);
		}
	}
	
	// ========================================================================
	// LOOP PRINCIPAL
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	protected void BH_Update()
	{
		if (!IsPlayerControlled())
			return;
		
		// Resolver componentes
		if (!m_pBH_StaminaComp)
		{
			m_pBH_StaminaComp = GetStaminaComponent();
			if (!m_pBH_StaminaComp)
				return;
		}
		
		if (!m_pBH_InputCtx)
		{
			m_pBH_InputCtx = GetInputContext();
			if (!m_pBH_InputCtx)
				return;
		}
		
		InputManager inputMgr = GetGame().GetInputManager();
		if (!inputMgr)
			return;
		
		bool inputPressed = inputMgr.GetActionValue(BH_INPUT_ACTION) > 0;
		float dt = BH_UPDATE_INTERVAL * 0.001;
		
		// Cooldown
		if (m_fBH_CooldownTimer > 0)
		{
			m_fBH_CooldownTimer -= dt;
			if (m_fBH_CooldownTimer < 0)
				m_fBH_CooldownTimer = 0;
		}
		
		// ================================================================
		// SPIKE POST-SOLTAR (jadeo al recuperar aliento)
		// ================================================================
		
		if (m_bBH_ExhaleSpike)
		{
			m_fBH_ExhaleSpikeTimer -= dt;
			
			if (m_fBH_ExhaleSpikeTimer <= 0)
			{
				// Spike terminado, volver a la normalidad
				m_bBH_ExhaleSpike = false;
			}
			else
			{
				// Drenar estamina extra durante el spike (simula jadeo)
				float spikeFade = m_fBH_ExhaleSpikeTimer / BH_EXHALE_SPIKE_DURATION;
				float drainExtra = -0.02 * spikeFade * m_fBH_ExhaleSpikeRatio;
				m_pBH_StaminaComp.AddStamina(drainExtra);
			}
		}
		
		// ================================================================
		// HOLD BREATH ACTIVO
		// ================================================================
		
		if (m_bBH_IsHolding)
		{
			bool shouldStop = false;
			
			if (!inputPressed)
				shouldStop = true;
			
			m_fBH_HoldTimer += dt;
			float maxTime = BH_GetMaxHoldTime();
			if (m_fBH_HoldTimer >= maxTime)
				shouldStop = true;
			
			if (!BH_IsInADS())
				shouldStop = true;
			
			if (GetLifeState() != ECharacterLifeState.ALIVE)
				shouldStop = true;
			
			if (shouldStop)
			{
				BH_Stop();
			}
			else
			{
				// Calcular en qué fase estamos (0.0 = inicio, 1.0 = final)
				float progress = m_fBH_HoldTimer / maxTime;
				
				// Calcular la efectividad del hold breath (1.0 = máxima, 0.0 = nula)
				float effectiveness = BH_CalcEffectiveness(progress);
				
				// Comprobar si estamos en fase de congelación total
				bool isFrozen = (m_fBH_HoldTimer <= BH_FREEZE_TIME && maxTime > BH_FREEZE_TIME);
				
				// === APLICAR ANTI-SWAY ===
				
				// Estamina: saturar siempre al máximo durante congelación y fase estable
				m_pBH_StaminaComp.AddStamina(effectiveness);
				
				if (isFrozen)
				{
					// FASE CONGELACIÓN TOTAL
					// Triple ataque para inmovilizar el arma por completo:
					
					// 1. Leer el cambio que el engine quiere aplicar
					vector engineAimChange = m_pBH_InputCtx.GetAimChange();
					
					// 2. Aplicar la inversa exacta para cancelarlo
					m_pBH_InputCtx.SetAimChange(-engineAimChange);
					
					// 3. Forzar los ángulos guardados al activar
					m_pBH_InputCtx.SetAimingAngles(m_vBH_FrozenAimAngles);
					
					// 4. Saturar estamina al máximo
					m_pBH_StaminaComp.AddStamina(1.0);
				}
				else if (effectiveness > 0.8)
				{
					// Fase estable: anular completamente el sway
					m_pBH_InputCtx.SetAimChange(vector.Zero);
				}
				else if (effectiveness > 0.01)
				{
					// Fase degradación: reducir el sway proporcionalmente
					vector currentAimChange = m_pBH_InputCtx.GetAimChange();
					vector dampedAimChange = currentAimChange * (1.0 - effectiveness);
					m_pBH_InputCtx.SetAimChange(dampedAimChange);
				}
			}
		}
		else
		{
			// Intentar activar
			if (inputPressed && BH_CanStart())
			{
				BH_Start();
			}
		}
	}
	
	// ========================================================================
	// CURVA DE EFECTIVIDAD
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	// Calcula la efectividad del hold breath según el progreso del tiempo
	// progress: 0.0 (acaba de empezar) → 1.0 (tiempo agotado)
	// retorna: 1.0 (sway eliminado) → 0.0 (sway normal)
	//
	// FASES (ejemplo con 15 seg y BH_FREEZE_TIME=3):
	// 0s-3s    → CONGELACIÓN: effectiveness = 1.0 (arma 100% inmóvil)
	// 3s-9s    → ESTABLE: effectiveness = 0.95 (micro-temblor residual)
	// 9s-15s   → DEGRADACIÓN: effectiveness baja de 0.95 a 0.0 (sway vuelve)
	protected float BH_CalcEffectiveness(float progress)
	{
		float maxTime = BH_GetMaxHoldTime();
		
		// Calcular el ratio de tiempo de congelación respecto al total
		float freezeRatio = 0;
		if (maxTime > BH_FREEZE_TIME)
			freezeRatio = BH_FREEZE_TIME / maxTime;
		
		if (progress <= freezeRatio)
		{
			// Fase congelación: efectividad total
			return 1.0;
		}
		
		// Ajustar el progreso para las fases restantes (estable + degradación)
		// Remapear de [freezeRatio, 1.0] a [0.0, 1.0]
		float remainingProgress = (progress - freezeRatio) / (1.0 - freezeRatio);
		
		if (remainingProgress <= BH_STABLE_PHASE)
		{
			// Fase estable: efectividad alta pero con micro-temblor
			return BH_MAX_EFFECTIVENESS;
		}
		else
		{
			// Fase degradación: el sway vuelve progresivamente
			float degradeProgress = (remainingProgress - BH_STABLE_PHASE) / (1.0 - BH_STABLE_PHASE);
			return BH_MAX_EFFECTIVENESS * (1.0 - degradeProgress);
		}
	}
	
	// ========================================================================
	// INICIO Y FIN
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	protected void BH_Start()
	{
		if (m_bBH_IsHolding)
			return;
		
		m_bBH_IsHolding = true;
		m_fBH_HoldTimer = 0;
		m_fBH_StaminaAtStart = m_pBH_StaminaComp.GetStamina();
		
		// Capturar los ángulos de aiming actuales para congelarlos
		m_vBH_FrozenAimAngles = m_pBH_InputCtx.GetAimingAngles();
		
		// Cancelar spike si estaba activo
		m_bBH_ExhaleSpike = false;
		
		#ifdef WORKBENCH
		Print("[BH_HoldBreath] ACTIVADO - stamina: " + m_fBH_StaminaAtStart, LogLevel.NORMAL);
		#endif
	}
	
	//------------------------------------------------------------------------------------------------
	protected void BH_Stop()
	{
		if (!m_bBH_IsHolding)
			return;
		
		m_bBH_IsHolding = false;
		
		// Calcular cuánto tiempo aguantó como ratio del máximo
		float maxTime = BH_MAX_TIME * m_fBH_StaminaAtStart;
		float holdRatio = 0;
		if (maxTime > 0)
			holdRatio = Math.Clamp(m_fBH_HoldTimer / maxTime, 0, 1);
		
		// Drenar estamina proporcional
		float cost = BH_STAMINA_COST * holdRatio;
		m_pBH_StaminaComp.AddStamina(-cost);
		
		// Activar spike de jadeo al soltar
		// Cuanto más aguantaste, más fuerte el spike
		m_bBH_ExhaleSpike = true;
		m_fBH_ExhaleSpikeTimer = BH_EXHALE_SPIKE_DURATION;
		m_fBH_ExhaleSpikeRatio = holdRatio * BH_EXHALE_SPIKE_INTENSITY;
		
		m_fBH_CooldownTimer = BH_COOLDOWN;
		
		#ifdef WORKBENCH
		Print("[BH_HoldBreath] DESACTIVADO - coste: " + cost + " spike: " + m_fBH_ExhaleSpikeRatio, LogLevel.NORMAL);
		#endif
	}
	
	// ========================================================================
	// CONDICIONES
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	protected bool BH_CanStart()
	{
		if (!BH_IsInADS())
			return false;
		
		if (GetLifeState() != ECharacterLifeState.ALIVE)
			return false;
		
		if (m_fBH_CooldownTimer > 0)
			return false;
		
		if (m_pBH_StaminaComp.GetStamina() < BH_MIN_STAMINA)
			return false;
		
		if (IsSprinting())
			return false;
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	protected bool BH_IsInADS()
	{
		return IsWeaponRaised() && IsWeaponADS();
	}
	
	//------------------------------------------------------------------------------------------------
	protected float BH_GetMaxHoldTime()
	{
		return BH_MAX_TIME * m_fBH_StaminaAtStart;
	}
	
	// ========================================================================
	// GETTERS PÚBLICOS
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	bool BH_IsHoldingBreath()
	{
		return m_bBH_IsHolding;
	}
	
	//------------------------------------------------------------------------------------------------
	float BH_GetRemainingRatio()
	{
		if (!m_bBH_IsHolding)
			return 0;
		
		float maxTime = BH_GetMaxHoldTime();
		if (maxTime <= 0)
			return 0;
		
		return Math.Clamp(1.0 - (m_fBH_HoldTimer / maxTime), 0, 1);
	}
	
	//------------------------------------------------------------------------------------------------
	float BH_GetCooldownRatio()
	{
		if (BH_COOLDOWN <= 0)
			return 0;
		
		return Math.Clamp(m_fBH_CooldownTimer / BH_COOLDOWN, 0, 1);
	}
	
	//------------------------------------------------------------------------------------------------
	// Devuelve la efectividad actual (para HUD, indicadores, etc.)
	float BH_GetCurrentEffectiveness()
	{
		if (!m_bBH_IsHolding)
			return 0;
		
		float maxTime = BH_GetMaxHoldTime();
		if (maxTime <= 0)
			return 0;
		
		float progress = m_fBH_HoldTimer / maxTime;
		return BH_CalcEffectiveness(progress);
	}
	
	//------------------------------------------------------------------------------------------------
	// ¿Está en fase de jadeo post-soltar?
	bool BH_IsExhaling()
	{
		return m_bBH_ExhaleSpike;
	}
	
	// ========================================================================
	// LIMPIEZA
	// ========================================================================
	
	//------------------------------------------------------------------------------------------------
	void ~SCR_CharacterControllerComponent()
	{
		if (m_bBH_Running)
		{
			GetGame().GetCallqueue().Remove(BH_Update);
			m_bBH_Running = false;
		}
		
		if (m_bBH_IsHolding)
		{
			m_bBH_IsHolding = false;
		}
	}
}