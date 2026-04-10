// ============================================================================
// BH_PlayerBriefingComponent.c
// Mod: Bhelma - Sistema de Briefing Pre-Partida
//
// DESCRIPCION:
//   Componente que se anade al SCR_PlayerController via modded class.
//   
//   PROBLEMA RESUELTO:
//   En Enfusion, los RPCs con RplRcver.Server solo se envian si
//   el cliente es OWNER de la entidad. El GameMode pertenece al
//   servidor, asi que el cliente NO es owner y los RPCs desde
//   BH_BriefingManagerComponent se ignoran silenciosamente en
//   servidor dedicado.
//
//   SOLUCION:
//   El cliente SI es owner de su PlayerController. Este componente
//   se anade al PlayerController y actua como puente:
//   Cliente -> RPC via PlayerController -> Servidor -> GameMode
//
//   DEBUG: Activar m_bDebugEnabled en BH_BriefingManagerComponent
// ============================================================================

// -------------------------------------------------------
// Modded SCR_PlayerController: anade nuestro componente
// para que el RPC funcione desde el cliente
// -------------------------------------------------------
modded class SCR_PlayerController
{
	// -------------------------------------------------------
	// Metodo publico: el cliente llama esto para pedir inicio de mision
	// El RPC se envia correctamente porque el cliente ES owner del PC
	// -------------------------------------------------------
	void BH_RequestStartMission()
	{
		int playerId = GetPlayerId();
		
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (briefingMgr)
			briefingMgr.BH_Debug("BH_RequestStartMission: enviando RPC desde PlayerController (owner=" + playerId + ")");
		
		Rpc(BH_RpcAsk_StartMission, playerId);
	}
	
	// -------------------------------------------------------
	// RPC: Cliente (owner) -> Servidor (authority)
	// Esto SI funciona en dedicado porque el cliente es owner del PC
	// -------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void BH_RpcAsk_StartMission(int playerId)
	{
		BH_BriefingManagerComponent briefingMgr = BH_BriefingManagerComponent.GetInstance();
		if (!briefingMgr)
			return;
		
		briefingMgr.BH_Debug("[SERVER] RPC recibido via PlayerController de jugador " + playerId);
		
		// Delegar al GameMode que tiene la logica de inicio
		briefingMgr.ServerStartMission(playerId);
	}
}