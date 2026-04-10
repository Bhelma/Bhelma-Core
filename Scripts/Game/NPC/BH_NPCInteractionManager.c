// ============================================================
//  BH_NPCInteractionManager.c
//  Singleton manager que registra todos los NPCs interactivos.
//
//  Uso: Añadir a la entidad GameMode o cualquier entidad
//       persistente de la misión (una sola vez).
//
//  Permite consultas globales desde otros scripts:
//    - Cuántos NPCs han sido arrestados
//    - Cuántos NPCs han sido interrogados
//    - Lista de todos los NPCs registrados
// ============================================================

[ComponentEditorProps(category: "BH/NPC", description: "Manager global de NPCs interactivos BH. Añadir a una entidad persistente de la misión.")]
class BH_NPCInteractionManagerClass : ScriptComponentClass {}

class BH_NPCInteractionManager : ScriptComponent
{
	// ----------------------------------------------------------
	//  SINGLETON
	// ----------------------------------------------------------

	protected static BH_NPCInteractionManager s_Instance;

	static BH_NPCInteractionManager GetInstance()
	{
		return s_Instance;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
	}

	override void OnDelete(IEntity owner)
	{
		s_Instance = null;
		super.OnDelete(owner);
	}

	// ----------------------------------------------------------
	//  REGISTRO DE NPCs
	// ----------------------------------------------------------

	protected ref array<BH_NPCInteractionComponent> m_aNPCs = new array<BH_NPCInteractionComponent>();

	void BH_RegisterNPC(BH_NPCInteractionComponent npc)
	{
		if (npc && m_aNPCs.Find(npc) == -1)
			m_aNPCs.Insert(npc);
	}

	void BH_UnregisterNPC(BH_NPCInteractionComponent npc)
	{
		int idx = m_aNPCs.Find(npc);
		if (idx != -1)
			m_aNPCs.Remove(idx);
	}

	// ----------------------------------------------------------
	//  CONSULTAS GLOBALES
	// ----------------------------------------------------------

	array<BH_NPCInteractionComponent> BH_GetAllNPCs()
	{
		return m_aNPCs;
	}

	array<BH_NPCInteractionComponent> BH_GetArrestedNPCs()
	{
		array<BH_NPCInteractionComponent> result = new array<BH_NPCInteractionComponent>();
		foreach (BH_NPCInteractionComponent npc : m_aNPCs)
		{
			if (npc && npc.BH_IsArrested())
				result.Insert(npc);
		}
		return result;
	}

	int BH_GetArrestedCount()
	{
		int count = 0;
		foreach (BH_NPCInteractionComponent npc : m_aNPCs)
		{
			if (npc && npc.BH_IsArrested())
				count++;
		}
		return count;
	}

	int BH_GetSpokenCount()
	{
		int count = 0;
		foreach (BH_NPCInteractionComponent npc : m_aNPCs)
		{
			if (npc && npc.BH_HasSpoken())
				count++;
		}
		return count;
	}
}