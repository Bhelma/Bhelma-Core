// ============================================================================
// BH_TrafficUrbanBehavior.c
//
// Sistema de comportamiento urbano para el tráfico automático.
// Gestiona la lógica de conductores que aparcan, bajan del vehículo,
// caminan hacia un edificio con puerta accesible, esperan un tiempo,
// vuelven al vehículo y continúan su ruta.
//
// Todo el movimiento se hace mediante waypoints nativos de IA,
// sin teleports ni trucos. El conductor camina de forma natural
// con todas sus animaciones.
//
// Se activa solo cuando un vehículo dinámico está cerca del jugador
// y en zona urbana.
//
// Funciona junto a BH_TrafficComponent.c
//
// Autor: Bhelma
// ============================================================================

class BH_EUrbanState
{
	static const int DRIVING = 0;
	static const int STOPPING = 1;
	static const int GETTING_OUT = 2;
	static const int WALKING_TO_BUILDING = 3;
	static const int WAITING_AT_BUILDING = 4;
	static const int WALKING_TO_VEHICLE = 5;
	static const int GETTING_IN = 6;
	static const int RESUMING_DRIVE = 7;
};

class BH_TrafficUrbanBehavior
{
	protected BH_TrafficComponent m_TrafficComp;
	
	protected const string WP_MOVE = "{22A875E30470BD4F}Prefabs/AI/Waypoints/AIWaypoint_Move.et";
	protected const string WP_GETIN = "{B049D4C74FBC0C4D}Prefabs/AI/Waypoints/AIWaypoint_GetInNearest.et";
	
	void BH_TrafficUrbanBehavior(BH_TrafficComponent trafficComp)
	{
		m_TrafficComp = trafficComp;
	}
	
	// ========================================================================
	// ACTUALIZACIÓN PRINCIPAL
	// ========================================================================
	
	void BH_UpdateUrbanBehavior(array<ref BH_TrafficVehicleData> activeVehicles, array<vector> playerPositions, float urbanRadius, float stayMin, float stayMax, float timeSlice, bool debugMode)
	{
		foreach (BH_TrafficVehicleData data : activeVehicles)
		{
			if (!data || !data.HasAI || !data.VehicleEntity)
				continue;
			
			switch (data.UrbanState)
			{
				case BH_EUrbanState.DRIVING:
					BH_CheckForUrbanStop(data, playerPositions, urbanRadius, debugMode);
					break;
				case BH_EUrbanState.STOPPING:
					BH_ProcessStopping(data, timeSlice, debugMode);
					break;
				case BH_EUrbanState.GETTING_OUT:
					BH_ProcessGettingOut(data, timeSlice, debugMode);
					break;
				case BH_EUrbanState.WALKING_TO_BUILDING:
					BH_ProcessWalkToBuilding(data, timeSlice, debugMode);
					break;
				case BH_EUrbanState.WAITING_AT_BUILDING:
					BH_ProcessWaitAtBuilding(data, timeSlice, stayMin, stayMax, debugMode);
					break;
				case BH_EUrbanState.WALKING_TO_VEHICLE:
					BH_ProcessWalkToVehicle(data, timeSlice, debugMode);
					break;
				case BH_EUrbanState.GETTING_IN:
					BH_ProcessGettingIn(data, timeSlice, debugMode);
					break;
				case BH_EUrbanState.RESUMING_DRIVE:
					BH_ProcessResumeDrive(data, timeSlice, debugMode);
					break;
			}
		}
	}
	
	// ========================================================================
	// DRIVING → Comprobar si debe parar
	// ========================================================================
	
	protected void BH_CheckForUrbanStop(BH_TrafficVehicleData data, array<vector> playerPositions, float urbanRadius, bool debugMode)
	{
		vector vehiclePos = data.VehicleEntity.GetOrigin();
		
		bool nearPlayer = false;
		foreach (vector playerPos : playerPositions)
		{
			if (vector.DistanceSq(vehiclePos, playerPos) <= urbanRadius * urbanRadius)
			{
				nearPlayer = true;
				break;
			}
		}
		
		if (!nearPlayer)
			return;
		
		// Buscar edificio con puerta cerca (confirma zona urbana)
		IEntity building = BH_FindBuildingWithDoor(vehiclePos, 80);
		if (!building)
			return;
		
		// 5% probabilidad por tick
		if (Math.RandomInt(0, 100) > 5)
			return;
		
		data.TargetBuilding = building;
		data.UrbanState = BH_EUrbanState.STOPPING;
		data.UrbanTimer = 0;
		data.ParkPosition = vehiclePos;
		
		BH_ClearGroupWaypoints(data);
		
		if (debugMode)
			BH_Log("[BH_UrbanBehavior] Parada urbana. Edificio a " + vector.Distance(vehiclePos, building.GetOrigin()).ToString() + "m");
	}
	
	// ========================================================================
	// STOPPING → Esperando a que frene
	// ========================================================================
	
	protected void BH_ProcessStopping(BH_TrafficVehicleData data, float timeSlice, bool debugMode)
	{
		data.UrbanTimer += timeSlice;
		
		if (data.UrbanTimer < 4.0)
			return;
		
		data.UrbanTimer = 0;
		data.UrbanState = BH_EUrbanState.GETTING_OUT;
		
		if (debugMode)
			BH_Log("[BH_UrbanBehavior] Vehículo frenado. Bajando...");
	}
	
	// ========================================================================
	// GETTING_OUT → Conductor baja del vehículo
	// ========================================================================
	
	protected void BH_ProcessGettingOut(BH_TrafficVehicleData data, float timeSlice, bool debugMode)
	{
		if (!data.DriverEntity)
		{
			data.UrbanState = BH_EUrbanState.DRIVING;
			return;
		}
		
		data.UrbanTimer += timeSlice;
		
		// Primer tick: sacar al conductor del vehículo (vehículo se queda donde está)
		if (data.UrbanTimer <= timeSlice + 0.1)
		{
			vector vehTransform[4];
			data.VehicleEntity.GetTransform(vehTransform);
			vector vehPos = data.VehicleEntity.GetOrigin();
			
			data.ParkPosition = vehPos;
			
			// Sacar al conductor al lateral izquierdo del vehículo
			CompartmentAccessComponent accessComp = CompartmentAccessComponent.Cast(data.DriverEntity.FindComponent(CompartmentAccessComponent));
			if (accessComp && accessComp.IsInCompartment())
			{
				vector exitPos = vehPos - vehTransform[0] * 3.0;
				BaseWorld world = GetGame().GetWorld();
				if (world)
					exitPos[1] = world.GetSurfaceY(exitPos[0], exitPos[2]);
				
				vector exitTransform[4];
				Math3D.MatrixIdentity4(exitTransform);
				exitTransform[3] = exitPos;
				accessComp.GetOutVehicle_NoDoor(exitTransform, false, false);
			}
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Vehículo aparcado al lateral. Conductor bajó. Esperando 3s...");
		}
		
		// A los 3 segundos: sacar del grupo de conducción y crear grupo a pie
		if (data.UrbanTimer >= 3.0)
		{
			// Sacar al conductor del grupo de conducción
			SCR_AIGroup driveGroup = SCR_AIGroup.Cast(data.GroupEntity);
			if (driveGroup)
			{
				driveGroup.RemoveAIEntityFromGroup(data.DriverEntity);
			}
			
			// Crear nuevo grupo temporal a pie
			Resource groupRes = Resource.Load("{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et");
			if (groupRes && groupRes.IsValid())
			{
				EntitySpawnParams groupParams = new EntitySpawnParams();
				groupParams.TransformMode = ETransformMode.WORLD;
				vector groupTransform[4];
				data.DriverEntity.GetTransform(groupTransform);
				groupParams.Transform = groupTransform;
				
				IEntity walkGroupEntity = GetGame().SpawnEntityPrefab(groupRes, GetGame().GetWorld(), groupParams);
				if (walkGroupEntity)
				{
					SCR_AIGroup walkGroup = SCR_AIGroup.Cast(walkGroupEntity);
					if (walkGroup)
					{
						walkGroup.AddAIEntityToGroup(data.DriverEntity);
						data.WalkGroupEntity = walkGroupEntity;
						
						if (debugMode)
							BH_Log("[BH_UrbanBehavior] Grupo a pie creado.");
					}
				}
			}
			
			data.UrbanState = BH_EUrbanState.WALKING_TO_BUILDING;
			data.UrbanTimer = 0;
		}
	}
	
	// ========================================================================
	// WALKING_TO_BUILDING → Waypoint MOVE hacia el edificio
	// ========================================================================
	
	protected void BH_ProcessWalkToBuilding(BH_TrafficVehicleData data, float timeSlice, bool debugMode)
	{
		if (!data.DriverEntity || !data.TargetBuilding)
		{
			data.UrbanState = BH_EUrbanState.DRIVING;
			return;
		}
		
		data.UrbanTimer += timeSlice;
		
		// Primer tick: asignar waypoint MOVE al grupo A PIE
		if (data.UrbanTimer <= timeSlice + 0.1)
		{
			vector targetPos = BH_FindDoorPosition(data.TargetBuilding);
			
			// Usar el grupo a pie, NO el de conducción
			BH_SpawnWalkWaypoint(data, WP_MOVE, targetPos);
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Waypoint MOVE (grupo a pie) → interior edificio " + targetPos.ToString());
		}
		
		// Comprobar distancia al edificio
		vector driverPos = data.DriverEntity.GetOrigin();
		vector buildingPos = data.TargetBuilding.GetOrigin();
		float dist = vector.Distance(driverPos, buildingPos);
		
		if (dist < 6.0)
		{
			BH_ClearWalkGroupWaypoints(data);
			data.UrbanState = BH_EUrbanState.WAITING_AT_BUILDING;
			data.UrbanTimer = 0;
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Conductor dentro/cerca del edificio. Esperando...");
		}
		else if (data.UrbanTimer > 120.0)
		{
			BH_ClearWalkGroupWaypoints(data);
			data.UrbanState = BH_EUrbanState.WAITING_AT_BUILDING;
			data.UrbanTimer = 0;
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Timeout caminata. Esperando en posición actual.");
		}
	}
	
	// ========================================================================
	// WAITING_AT_BUILDING → Parado esperando
	// ========================================================================
	
	protected void BH_ProcessWaitAtBuilding(BH_TrafficVehicleData data, float timeSlice, float stayMin, float stayMax, bool debugMode)
	{
		if (data.UrbanTimer == 0)
		{
			float stayTime = stayMin + Math.RandomFloat01() * (stayMax - stayMin);
			data.UrbanTimer = -stayTime;
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Esperando " + stayTime.ToString() + "s.");
		}
		
		data.UrbanTimer += timeSlice;
		
		if (data.UrbanTimer >= 0)
		{
			data.UrbanState = BH_EUrbanState.WALKING_TO_VEHICLE;
			data.UrbanTimer = 0;
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Fin espera. Volviendo al vehículo...");
		}
	}
	
	// ========================================================================
	// WALKING_TO_VEHICLE → Waypoint MOVE al vehículo
	// ========================================================================
	
	protected void BH_ProcessWalkToVehicle(BH_TrafficVehicleData data, float timeSlice, bool debugMode)
	{
		if (!data.DriverEntity || !data.VehicleEntity)
		{
			data.UrbanState = BH_EUrbanState.DRIVING;
			return;
		}
		
		data.UrbanTimer += timeSlice;
		
		if (data.UrbanTimer <= timeSlice + 0.1)
		{
			vector vehiclePos = data.VehicleEntity.GetOrigin();
			// Usar grupo a pie
			BH_SpawnWalkWaypoint(data, WP_MOVE, vehiclePos);
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Waypoint MOVE (grupo a pie) → vehículo.");
		}
		
		vector driverPos = data.DriverEntity.GetOrigin();
		vector vehiclePos = data.VehicleEntity.GetOrigin();
		float dist = vector.Distance(driverPos, vehiclePos);
		
		if (dist < 5.0)
		{
			BH_ClearWalkGroupWaypoints(data);
			data.UrbanState = BH_EUrbanState.GETTING_IN;
			data.UrbanTimer = 0;
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Llegó al vehículo. Subiendo...");
		}
		else if (data.UrbanTimer > 120.0)
		{
			BH_ClearWalkGroupWaypoints(data);
			data.UrbanState = BH_EUrbanState.GETTING_IN;
			data.UrbanTimer = 0;
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Timeout vuelta. Forzando subida.");
		}
	}
	
	// ========================================================================
	// GETTING_IN → Waypoint GET_IN al vehículo
	// ========================================================================
	
	protected void BH_ProcessGettingIn(BH_TrafficVehicleData data, float timeSlice, bool debugMode)
	{
		if (!data.DriverEntity || !data.VehicleEntity)
		{
			data.UrbanState = BH_EUrbanState.DRIVING;
			return;
		}
		
		data.UrbanTimer += timeSlice;
		
		// Primer tick: eliminar grupo a pie, recrear grupo conducción, montar
		if (data.UrbanTimer <= timeSlice + 0.1)
		{
			// Sacar del grupo a pie y eliminarlo
			if (data.WalkGroupEntity)
			{
				SCR_AIGroup walkGroup = SCR_AIGroup.Cast(data.WalkGroupEntity);
				if (walkGroup)
				{
					BH_ClearWalkGroupWaypoints(data);
					walkGroup.RemoveAIEntityFromGroup(data.DriverEntity);
				}
				SCR_EntityHelper.DeleteEntityAndChildren(data.WalkGroupEntity);
				data.WalkGroupEntity = null;
			}
			
			// Eliminar grupo de conducción viejo (puede estar corrupto)
			if (data.GroupEntity)
			{
				BH_ClearGroupWaypoints(data);
				SCR_EntityHelper.DeleteEntityAndChildren(data.GroupEntity);
				data.GroupEntity = null;
			}
			
			// Crear grupo de conducción NUEVO
			Resource groupRes = Resource.Load("{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et");
			if (groupRes && groupRes.IsValid())
			{
				EntitySpawnParams groupParams = new EntitySpawnParams();
				groupParams.TransformMode = ETransformMode.WORLD;
				vector groupTransform[4];
				data.VehicleEntity.GetTransform(groupTransform);
				groupParams.Transform = groupTransform;
				
				IEntity newGroupEntity = GetGame().SpawnEntityPrefab(groupRes, GetGame().GetWorld(), groupParams);
				if (newGroupEntity)
				{
					SCR_AIGroup newGroup = SCR_AIGroup.Cast(newGroupEntity);
					if (newGroup)
					{
						newGroup.AddAIEntityToGroup(data.DriverEntity);
						data.GroupEntity = newGroupEntity;
						
						if (debugMode)
							BH_Log("[BH_UrbanBehavior] Nuevo grupo conducción creado.");
					}
				}
			}
			
			// Forzar montaje
			BH_ForceMountDriver(data);
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Conductor montado. Preparando ruta...");
		}
		
		// Comprobar si ya está dentro
		CompartmentAccessComponent accessComp = CompartmentAccessComponent.Cast(data.DriverEntity.FindComponent(CompartmentAccessComponent));
		if (accessComp && accessComp.IsInCompartment())
		{
			data.UrbanState = BH_EUrbanState.RESUMING_DRIVE;
			data.UrbanTimer = 0;
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Retomando ruta...");
		}
		else if (data.UrbanTimer > 5.0)
		{
			BH_ForceMountDriver(data);
			data.UrbanState = BH_EUrbanState.RESUMING_DRIVE;
			data.UrbanTimer = 0;
			
			if (debugMode)
				BH_Log("[BH_UrbanBehavior] Reintento montaje.");
		}
	}
	
	// ========================================================================
	// RESUMING_DRIVE → Nuevo waypoint de conducción
	// ========================================================================
	
	protected void BH_ProcessResumeDrive(BH_TrafficVehicleData data, float timeSlice, bool debugMode)
	{
		data.UrbanTimer += timeSlice;
		
		if (data.UrbanTimer < 2.0)
			return;
		
		if (m_TrafficComp)
			m_TrafficComp.BH_AssignNewDrivingWaypoint(data);
		
		data.UrbanState = BH_EUrbanState.DRIVING;
		data.UrbanTimer = 0;
		data.TargetBuilding = null;
		
		if (debugMode)
			BH_Log("[BH_UrbanBehavior] Ruta retomada.");
	}
	
	// ========================================================================
	// UTILIDADES
	// ========================================================================
	
	//! Limpia waypoints de un grupo específico
	protected void BH_ClearWaypointsOfGroup(IEntity groupEntity, BH_TrafficVehicleData data)
	{
		if (!groupEntity)
			return;
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(groupEntity);
		if (!aiGroup)
			return;
		
		array<AIWaypoint> waypoints = {};
		aiGroup.GetWaypoints(waypoints);
		foreach (AIWaypoint wp : waypoints)
		{
			aiGroup.RemoveWaypoint(wp);
			SCR_EntityHelper.DeleteEntityAndChildren(wp);
		}
		
		if (data)
			data.WaypointEntity = null;
	}
	
	//! Limpia waypoints del grupo de conducción
	protected void BH_ClearGroupWaypoints(BH_TrafficVehicleData data)
	{
		BH_ClearWaypointsOfGroup(data.GroupEntity, data);
	}
	
	//! Limpia waypoints del grupo a pie
	protected void BH_ClearWalkGroupWaypoints(BH_TrafficVehicleData data)
	{
		BH_ClearWaypointsOfGroup(data.WalkGroupEntity, data);
	}
	
	//! Spawnea waypoint y lo asigna a un grupo específico
	protected void BH_SpawnWaypointForGroup(IEntity groupEntity, BH_TrafficVehicleData data, string prefabPath, vector position)
	{
		if (!groupEntity)
			return;
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(groupEntity);
		if (!aiGroup)
			return;
		
		// Limpiar waypoints anteriores del grupo
		BH_ClearWaypointsOfGroup(groupEntity, data);
		
		Resource wpResource = Resource.Load(prefabPath);
		if (!wpResource || !wpResource.IsValid())
		{
			BH_Log("[BH_UrbanBehavior] AVISO: No se pudo cargar waypoint: " + prefabPath);
			return;
		}
		
		EntitySpawnParams wpParams = new EntitySpawnParams();
		wpParams.TransformMode = ETransformMode.WORLD;
		
		vector wpTransform[4];
		Math3D.MatrixIdentity4(wpTransform);
		wpTransform[3] = position;
		wpParams.Transform = wpTransform;
		
		IEntity waypoint = GetGame().SpawnEntityPrefab(wpResource, GetGame().GetWorld(), wpParams);
		if (!waypoint)
			return;
		
		AIWaypoint wp = AIWaypoint.Cast(waypoint);
		if (wp)
		{
			aiGroup.AddWaypoint(wp);
			if (data)
				data.WaypointEntity = waypoint;
		}
		else
		{
			SCR_EntityHelper.DeleteEntityAndChildren(waypoint);
		}
	}
	
	//! Spawnea waypoint para el grupo de conducción
	protected void BH_SpawnAndAssignWaypoint(BH_TrafficVehicleData data, string prefabPath, vector position)
	{
		BH_SpawnWaypointForGroup(data.GroupEntity, data, prefabPath, position);
	}
	
	//! Spawnea waypoint para el grupo a pie
	protected void BH_SpawnWalkWaypoint(BH_TrafficVehicleData data, string prefabPath, vector position)
	{
		BH_SpawnWaypointForGroup(data.WalkGroupEntity, data, prefabPath, position);
	}
	
	protected void BH_ForceMountDriver(BH_TrafficVehicleData data)
	{
		if (!data.DriverEntity || !data.VehicleEntity)
			return;
		
		CompartmentAccessComponent accessComp = CompartmentAccessComponent.Cast(data.DriverEntity.FindComponent(CompartmentAccessComponent));
		if (!accessComp)
			return;
		
		BaseCompartmentManagerComponent compManager = BaseCompartmentManagerComponent.Cast(data.VehicleEntity.FindComponent(BaseCompartmentManagerComponent));
		if (!compManager)
			return;
		
		array<BaseCompartmentSlot> compartments = {};
		compManager.GetCompartments(compartments);
		
		foreach (BaseCompartmentSlot slot : compartments)
		{
			if (PilotCompartmentSlot.Cast(slot) && !slot.GetOccupant())
			{
				accessComp.GetInVehicle(data.VehicleEntity, slot, true, -1, 0, false);
				return;
			}
		}
	}
	
	// ========================================================================
	// BÚSQUEDA DE EDIFICIOS CON PUERTA
	// ========================================================================
	
	IEntity BH_FindBuildingWithDoor(vector center, float radius)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;
		
		m_aTempBuildings = new array<IEntity>();
		world.QueryEntitiesBySphere(center, radius, BH_BuildingWithDoorCallback, null, EQueryEntitiesFlags.STATIC);
		
		if (m_aTempBuildings.IsEmpty())
			return null;
		
		int idx = Math.RandomInt(0, m_aTempBuildings.Count());
		IEntity result = m_aTempBuildings[idx];
		m_aTempBuildings = null;
		
		return result;
	}
	
	//! Busca la posición de la puerta del edificio y calcula un punto interior
	protected vector BH_FindDoorPosition(IEntity building)
	{
		if (!building)
			return vector.Zero;
		
		vector buildingOrigin = building.GetOrigin();
		
		// Buscar la puerta en las entidades hijas
		IEntity child = building.GetChildren();
		while (child)
		{
			BaseDoorComponent doorComp = BaseDoorComponent.Cast(child.FindComponent(BaseDoorComponent));
			if (doorComp)
			{
				// Encontramos una puerta - usar su posición
				vector doorPos = child.GetOrigin();
				
				// Calcular dirección de la puerta hacia el interior del edificio
				// La dirección interior es desde la puerta hacia el centro del edificio
				vector toCenter = buildingOrigin - doorPos;
				toCenter[1] = 0;
				toCenter.Normalize();
				
				// Posicionar el waypoint 2m dentro del edificio pasando la puerta
				vector interiorPos = doorPos + toCenter * 2.0;
				
				// Ajustar Y al suelo del edificio
				BaseWorld world = GetGame().GetWorld();
				if (world)
					interiorPos[1] = world.GetSurfaceY(interiorPos[0], interiorPos[2]) + 0.5;
				
				return interiorPos;
			}
			child = child.GetSibling();
		}
		
		// Si no encontró puerta, usar el centro del edificio al nivel del suelo
		BaseWorld world = GetGame().GetWorld();
		if (world)
			buildingOrigin[1] = world.GetSurfaceY(buildingOrigin[0], buildingOrigin[2]) + 0.5;
		
		return buildingOrigin;
	}
	
	protected ref array<IEntity> m_aTempBuildings;
	
	protected bool BH_BuildingWithDoorCallback(IEntity entity)
	{
		if (!entity || !m_aTempBuildings)
			return true;
		
		string className = entity.ClassName();
		
		if (!className.Contains("Building") && !className.Contains("House") && !className.Contains("Structure"))
			return true;
		
		// Buscar puertas en hijos
		IEntity child = entity.GetChildren();
		while (child)
		{
			BaseDoorComponent doorComp = BaseDoorComponent.Cast(child.FindComponent(BaseDoorComponent));
			if (doorComp)
			{
				m_aTempBuildings.Insert(entity);
				return true;
			}
			child = child.GetSibling();
		}
		
		// Fallback: ActionsManager indica interactividad
		ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent));
		if (actionsManager)
		{
			m_aTempBuildings.Insert(entity);
		}
		
		return true;
	}
	
	// ========================================================================
	// LOG
	// ========================================================================
	
	protected void BH_Log(string message)
	{
		Print(message, LogLevel.NORMAL);
	}
};
