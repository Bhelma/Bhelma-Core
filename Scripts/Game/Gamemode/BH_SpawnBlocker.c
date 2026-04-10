// ============================================================================
// BH_SpawnBlocker.c
// Mod: Bhelma - Sistema de Briefing Pre-Partida
// 
// DESCRIPCION:
//   Modded class de SCR_BaseGameMode que intercepta CanPlayerSpawn_S
//   para bloquear spawns en dos casos:
//     1. Durante la fase BRIEFING (antes de /startmission)
//     2. Jugadores con muerte permanente (ya murieron en la mision)
//
//   No requiere configuracion. Se activa automaticamente.
//
//   DEBUG: Usa el flag de debug de BH_BriefingManagerComponent
// ============================================================================

modded class SCR_BaseGameMode
{
	override bool CanPlayerSpawn_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, out SCR_ESpawnResult result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED)
	{
		int playerId = requestComponent.GetPlayerId();
		
		// 1. Bloquear durante BRIEFING
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (briefingMgr && briefingMgr.IsBriefing())
		{
			briefingMgr.BH_Debug("CanPlayerSpawn_S: BLOQUEADO (BRIEFING) jugador " + playerId);
			result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
			return false;
		}
		
		// 2. Bloquear jugadores con muerte permanente
		BH_DeathSpectatorComponent deathComp = BH_DeathSpectatorComponent.GetInstance();
		if (deathComp && deathComp.IsPlayerPermaDead(playerId))
		{
			result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
			return false;
		}
		
		return super.CanPlayerSpawn_S(requestComponent, handlerComponent, data, result);
	}
}