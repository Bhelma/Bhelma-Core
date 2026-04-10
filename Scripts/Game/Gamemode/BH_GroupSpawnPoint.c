// ============================================================================
// BH_GroupSpawnPoint.c
// Mod: Bhelma - Sistema de Briefing Pre-Partida
// 
// DESCRIPCION:
//   Componente que se anade a los SCR_SpawnPoint en el Workbench.
//   Permite al editor del escenario asignar un spawn point a un
//   grupo/escuadra especifico (ej: "Alfa", "Bravo").
//
//   Cuando el admin lanza la mision con /startmission, el sistema
//   busca el BH_GroupSpawnPoint que coincida con el nombre del grupo
//   de cada jugador y lo spawnea ahi automaticamente.
//
//   Uso en Workbench:
//     1. Coloca un SpawnPoint (ej: SpawnPoint_US.et) en el mundo
//     2. Anadele el componente BH_GroupSpawnPoint
//     3. En "Nombre del grupo" escribe el nombre exacto (ej: "Alfa")
//     4. Opcionalmente marca "Es spawn por defecto"
//
//   La comparacion de nombres NO es case-sensitive.
//
//   DEBUG: Activar m_bDebugEnabled para mensajes de log
// ============================================================================

[ComponentEditorProps(category: "Bhelma/SpawnPoints", description: "Asigna un spawn point a un grupo/escuadra para el sistema de briefing de Bhelma.")]
class BH_GroupSpawnPointClass : ScriptComponentClass
{
}

class BH_GroupSpawnPoint : ScriptComponent
{
	[Attribute("", UIWidgets.EditBox, "Nombre del grupo/escuadra (ej: Alfa, Bravo). Debe coincidir con el nombre del grupo en el lobby.", category: "Bhelma Briefing")]
	protected string m_sGroupName;
	
	[Attribute("0", UIWidgets.CheckBox, "Spawn point por defecto para jugadores sin grupo", category: "Bhelma Briefing")]
	protected bool m_bIsDefault;
	
	[Attribute("0", UIWidgets.CheckBox, "Activar mensajes de debug en consola", category: "Bhelma Briefing")]
	protected bool m_bDebugEnabled;
	
	protected static ref array<BH_GroupSpawnPoint> s_aAllGroupSpawnPoints = {};
	
	// -------------------------------------------------------
	// Getters
	// -------------------------------------------------------
	string GetGroupName()
	{
		return m_sGroupName;
	}
	
	bool IsDefault()
	{
		return m_bIsDefault;
	}
	
	// -------------------------------------------------------
	// Obtener RplId del SpawnPoint padre
	// -------------------------------------------------------
	RplId GetRplId()
	{
		SCR_SpawnPoint spawnPoint = SCR_SpawnPoint.Cast(GetOwner());
		if (spawnPoint)
			return spawnPoint.GetRplId();
		
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (rpl)
			return rpl.Id();
		
		return RplId.Invalid();
	}
	
	// -------------------------------------------------------
	// Registro al inicializar
	// -------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		if (!s_aAllGroupSpawnPoints)
			s_aAllGroupSpawnPoints = {};
		
		s_aAllGroupSpawnPoints.Insert(this);
		
		BH_Debug("Registrado: grupo='" + m_sGroupName + "' default=" + m_bIsDefault);
	}
	
	// -------------------------------------------------------
	// Buscar spawn point por nombre de grupo (no case-sensitive)
	// -------------------------------------------------------
	static BH_GroupSpawnPoint FindSpawnPointForGroup(string groupName)
	{
		if (!s_aAllGroupSpawnPoints || groupName.IsEmpty())
			return null;
		
		string groupNameLower = groupName;
		groupNameLower.ToLower();
		
		foreach (BH_GroupSpawnPoint sp : s_aAllGroupSpawnPoints)
		{
			if (!sp)
				continue;
			
			string spName = sp.GetGroupName();
			spName.ToLower();
			
			if (spName == groupNameLower)
				return sp;
		}
		
		return null;
	}
	
	// -------------------------------------------------------
	// Obtener spawn por defecto
	// -------------------------------------------------------
	static BH_GroupSpawnPoint GetDefaultSpawnPoint()
	{
		if (!s_aAllGroupSpawnPoints)
			return null;
		
		foreach (BH_GroupSpawnPoint sp : s_aAllGroupSpawnPoints)
		{
			if (sp && sp.IsDefault())
				return sp;
		}
		
		if (s_aAllGroupSpawnPoints.Count() > 0)
			return s_aAllGroupSpawnPoints[0];
		
		return null;
	}
	
	// -------------------------------------------------------
	// Obtener todos los spawn points
	// -------------------------------------------------------
	static array<BH_GroupSpawnPoint> GetAllGroupSpawnPoints()
	{
		return s_aAllGroupSpawnPoints;
	}
	
	protected void BH_Debug(string message)
	{
		if (m_bDebugEnabled)
			Print("[BH_GroupSpawn] " + message, LogLevel.NORMAL);
	}
	
	void ~BH_GroupSpawnPoint()
	{
		if (s_aAllGroupSpawnPoints)
			s_aAllGroupSpawnPoints.RemoveItem(this);
	}
}