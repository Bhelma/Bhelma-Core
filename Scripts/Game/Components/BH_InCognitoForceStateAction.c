/*
=============================================================================
	BH_InCognitoForceStateAction.c

	Acción del ScenarioFramework que fuerza el estado de incógnito
	de los jugadores de forma manual.

	Uso en el editor:
	- Añadir en cualquier lista de acciones del ScenarioFramework
	  (OnActivation, OnTaskFinished, OnInit, etc.)
	- Configurar los atributos según el comportamiento deseado

	Ejemplos de uso:
	- Trigger de zona militarizada → forzar COMPROMETIDO permanente
	- Trigger de zona segura → forzar INCOGNITO durante 30s
	- Al matar un civil → forzar COMPROMETIDO durante 60s
	- Acción de reseteo → volver al estado automático por ropa

	Autor: BH
=============================================================================
*/

[BaseContainerProps()]
class BH_InCognitoForceStateAction : SCR_ScenarioFrameworkActionBase
{
	//---------------------------------------------------------------------
	// ATRIBUTOS CONFIGURABLES DESDE EL EDITOR
	//---------------------------------------------------------------------

	[Attribute("1", UIWidgets.ComboBox, "Estado al que forzar a los jugadores", "", ParamEnumArray.FromEnum(BH_EInCognitoEstado), category: "BH Incognito")]
	protected BH_EInCognitoEstado m_eEstadoForzado;

	[Attribute("1", UIWidgets.CheckBox, "¿Afectar a TODOS los jugadores? Si está desmarcado, solo afecta al jugador que activó el trigger.", category: "BH Incognito")]
	protected bool m_bAfectarTodos;

	[Attribute("0", UIWidgets.CheckBox, "¿El estado forzado es permanente? Si está marcado, ignora la detección automática de ropa.", category: "BH Incognito")]
	protected bool m_bPermanente;

	[Attribute("0", UIWidgets.Slider, "Duración del estado forzado en segundos. 0 = permanente. Solo aplica si NO es permanente.", "0 3600 1", category: "BH Incognito")]
	protected float m_fDuracion;

	//---------------------------------------------------------------------
	// OVERRIDE
	//---------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! El ScenarioFramework llama a este método para ejecutar la acción.
	//! object es la entidad asociada al trigger o slot que lanzó la acción.
	override void OnActivate(IEntity object)
	{
		if (!CanActivate())
			return;

		// La lógica de override solo se ejecuta en el servidor
		// El servidor notifica a los clientes via RPC (gestionado por BH_InCognitoComponent)
		if (Replication.IsRunning() && !Replication.IsServer())
			return;

		BH_InCognitoComponent pIncognito = BH_InCognitoComponent.Cast(
			GetGame().GetGameMode().FindComponent(BH_InCognitoComponent)
		);

		if (!pIncognito)
		{
			Print("[BH_Incognito] ERROR: BH_InCognitoForceStateAction no encontró BH_InCognitoComponent.", LogLevel.ERROR);
			return;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		if (m_bAfectarTodos)
		{
			// Afectar a todos los jugadores conectados
			array<int> jugadoresIds = {};
			playerManager.GetAllPlayers(jugadoresIds);

			foreach (int pid : jugadoresIds)
			{
				BH_ForzarEstadoJugador(pIncognito, pid);
			}
		}
		else
		{
			// Afectar solo al jugador que activó el trigger
			if (!object)
				return;

			int playerId = playerManager.GetPlayerIdFromControlledEntity(object);
			if (playerId <= 0)
				return;

			BH_ForzarEstadoJugador(pIncognito, playerId);
		}
	}

	//---------------------------------------------------------------------
	// LÓGICA INTERNA
	//---------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Aplica el override de estado a un jugador concreto
	protected void BH_ForzarEstadoJugador(BH_InCognitoComponent pIncognito, int playerId)
	{
		if (m_bPermanente || m_fDuracion <= 0)
		{
			// Permanente: el estado forzado no expira
			pIncognito.BH_SetOverrideEstado(playerId, m_eEstadoForzado, true, 0);
		}
		else
		{
			// Temporal: el estado forzado expira tras m_fDuracion segundos
			pIncognito.BH_SetOverrideEstado(playerId, m_eEstadoForzado, false, m_fDuracion);
		}

		string duracionStr;
		if (m_bPermanente)
			duracionStr = " (permanente)";
		else
			duracionStr = " (duración: " + m_fDuracion + "s)";
		
		Print("[BH_Incognito] Estado forzado a " + m_eEstadoForzado + " para jugador " + playerId + duracionStr, LogLevel.NORMAL);
	}
}