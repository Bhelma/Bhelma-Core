/*
=============================================================================
	BH_InCognitoSlotTrigger.c

	Condición de trigger personalizada para el ScenarioFramework.
	Se añade en la sección "Custom Trigger Conditions" de cualquier
	SCR_ScenarioFrameworkTriggerEntity (el trigger nativo del SlotTrigger.et).

	Uso en el editor:
	1. Selecciona el trigger nativo dentro del SlotTrigger.et en el mapa
	2. En "Custom Trigger Conditions" añade BH_InCognitoActivationCondition
	3. Configura m_eEstadoRequerido y m_bRequiereTodos
	4. En "Custom Trigger Condition Logic" selecciona AND

	Autor: BH
=============================================================================
*/

[BaseContainerProps()]
class BH_InCognitoActivationCondition : SCR_ScenarioFrameworkActivationConditionBase
{
	[Attribute("0", UIWidgets.ComboBox, "Estado que debe tener el jugador para activar el trigger", "", ParamEnumArray.FromEnum(BH_EInCognitoEstado), category: "BH Incognito")]
	protected BH_EInCognitoEstado m_eEstadoRequerido;

	[Attribute("0", UIWidgets.CheckBox, "Si está marcado, TODOS los jugadores en el área deben cumplir la condición. Si no, basta con UNO.", category: "BH Incognito")]
	protected bool m_bRequiereTodos;

	//------------------------------------------------------------------------------------------------
	//! El ScenarioFramework llama a este método para evaluar si la condición se cumple.
	//! entity es el SCR_ScenarioFrameworkTriggerEntity al que está adjunta esta condición.
	override bool Init(IEntity entity)
	{
		SCR_ScenarioFrameworkTriggerEntity trigger = SCR_ScenarioFrameworkTriggerEntity.Cast(entity);
		if (!trigger)
			return false;

		BH_InCognitoComponent pIncognito;
		if (GetGame().GetGameMode())
		{
			pIncognito = BH_InCognitoComponent.Cast(
				GetGame().GetGameMode().FindComponent(BH_InCognitoComponent)
			);
		}

		if (!pIncognito)
		{
			return false;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<IEntity> entidadesDentro = {};
		trigger.GetEntitiesInside(entidadesDentro);

		
		if (entidadesDentro.IsEmpty())
		{
			return false;
		}

		if (m_bRequiereTodos)
		{
			bool hayJugador = false;

			foreach (IEntity entDentro : entidadesDentro)
			{
				int pid = playerManager.GetPlayerIdFromControlledEntity(entDentro);
				if (pid <= 0)
					continue;

				hayJugador = true;
				if (pIncognito.BH_GetEstadoJugador(pid) != m_eEstadoRequerido)
				{
					return false;
				}
			}

			return hayJugador;
		}
		else
		{
			foreach (IEntity entDentro : entidadesDentro)
			{
				int pid = playerManager.GetPlayerIdFromControlledEntity(entDentro);
				if (pid <= 0)
					continue;

				if (pIncognito.BH_GetEstadoJugador(pid) == m_eEstadoRequerido)
				{
					return true;
				}
			}

			return false;
		}
	}
}