// ============================================================
//  BH_IED_Caller.c
//  Autor: BH Mod
//  Descripción: Componente que se añade a IAs insurgentes.
//               Representa al operador que "llama" al móvil
//               de la IED para detonarla. Al inicializarse,
//               añade un móvil al inventario de la IA.
//
//  LÓGICA DE RED:
//  Para que una IED detone por proximidad, necesita:
//  1) La IED tiene cobertura de alguna antena operativa
//  2) Al menos una IA con móvil (Caller) viva tiene
//     cobertura de alguna antena operativa (puede ser
//     una antena diferente — funcionan como red)
//  3) El jammer no está activo en rango
//
//  COMPATIBILIDAD:
//  Si no hay ningún BH_IED_Caller en el mapa,
//  el sistema funciona como antes (sin necesitar IA).
// ============================================================

class BH_IED_CallerClass : ScriptComponentClass {}

class BH_IED_Caller : ScriptComponent
{
	// --------------------------------------------------------
	// Registro estático de todos los callers en el mundo
	// --------------------------------------------------------

	protected static ref array<BH_IED_Caller> s_aCallers = new array<BH_IED_Caller>();

	// --------------------------------------------------------
	// Atributos configurables desde Workbench
	// --------------------------------------------------------

	[Attribute("{D8C747CBFD867CDD}Prefabs/IED/BH_NokiaPhone_IED.et", UIWidgets.ResourceNamePicker, "Prefab del móvil que se añade al inventario de la IA", "et")]
	protected ResourceName m_sPhonePrefab;

	// --------------------------------------------------------
	// Variables internas
	// --------------------------------------------------------

	// (sin variables de estado - comprobamos IsDestroyed en tiempo real)

	// --------------------------------------------------------
	// Init
	// --------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Registrar en la lista global
		if (!s_aCallers.Contains(this))
			s_aCallers.Insert(this);

		if (!BH_IED_Utils.BH_IsServer())
			return;

		// Añadir móvil al inventario con delay para que el inventario de la IA
		// se inicialice completamente antes de intentar insertar
		GetGame().GetCallqueue().CallLater(BH_AddPhoneToInventory, 2000, false);

		BH_IED_Utils.BH_Log(string.Format("[BH_IED_Caller] IA Caller inicializado. Total callers: %1", s_aCallers.Count()));
	}

	// --------------------------------------------------------
	// Cleanup
	// --------------------------------------------------------

	override void OnDelete(IEntity owner)
	{
		s_aCallers.RemoveItem(this);
		BH_IED_Utils.BH_Log(string.Format("[BH_IED_Caller] IA Caller eliminado. Total callers: %1", s_aCallers.Count()));
		super.OnDelete(owner);
	}

	// --------------------------------------------------------
	// Getters
	// --------------------------------------------------------

	// Comprueba si la IA está viva
	bool BH_IsAlive()
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Caller] BH_IsAlive: owner es null — muerto.");
			return false;
		}

		// Para personajes, usar CharacterControllerComponent
		CharacterControllerComponent charCtrl = CharacterControllerComponent.Cast(
			owner.FindComponent(CharacterControllerComponent)
		);
		if (charCtrl)
		{
			bool dead = charCtrl.IsDead();
			BH_IED_Utils.BH_Log(string.Format("[BH_IED_Caller] BH_IsAlive: IsDead=%1", dead));
			return !dead;
		}

		// Fallback: DamageManagerComponent genérico
		DamageManagerComponent dmgMgr = DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent));
		if (dmgMgr)
		{
			bool destroyed = dmgMgr.IsDestroyed();
			BH_IED_Utils.BH_Log(string.Format("[BH_IED_Caller] BH_IsAlive (DmgMgr): IsDestroyed=%1", destroyed));
			return !destroyed;
		}

		BH_IED_Utils.BH_Log("[BH_IED_Caller] BH_IsAlive: Sin CharCtrl ni DmgMgr — asumimos vivo.");
		return true;
	}

	// Comprueba si este caller está operativo:
	// - Está vivo
	// - Tiene cobertura de alguna antena operativa
	bool BH_IsOperational()
	{
		if (!BH_IsAlive()) return false;

		IEntity owner = GetOwner();
		if (!owner) return false;

		return BH_SignalTower.BH_HasSignalCoverage(owner.GetOrigin());
	}

	// --------------------------------------------------------
	// Añadir móvil al inventario
	// --------------------------------------------------------

	protected void BH_AddPhoneToInventory()
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Caller] ERROR: Owner es null al intentar añadir móvil.");
			return;
		}

		if (m_sPhonePrefab.IsEmpty())
		{
			BH_IED_Utils.BH_Log("[BH_IED_Caller] Prefab del móvil vacío, no se añade.");
			return;
		}

		BH_IED_Utils.BH_Log(string.Format("[BH_IED_Caller] Intentando añadir móvil. Prefab: %1", m_sPhonePrefab));

		// Buscar inventario del personaje
		SCR_InventoryStorageManagerComponent scrInvMgr = SCR_InventoryStorageManagerComponent.Cast(
			owner.FindComponent(SCR_InventoryStorageManagerComponent)
		);

		InventoryStorageManagerComponent invMgr;
		if (scrInvMgr)
		{
			invMgr = scrInvMgr;
			BH_IED_Utils.BH_Log("[BH_IED_Caller] SCR_InventoryStorageManagerComponent encontrado.");
		}
		else
		{
			invMgr = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
			if (invMgr)
				BH_IED_Utils.BH_Log("[BH_IED_Caller] InventoryStorageManagerComponent encontrado (base).");
		}

		if (!invMgr)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Caller] ERROR: No se encontró InventoryStorageManager en la IA.");
			return;
		}

		// Crear el prefab del móvil
		Resource phoneRes = Resource.Load(m_sPhonePrefab);
		if (!phoneRes || !phoneRes.IsValid())
		{
			BH_IED_Utils.BH_Log("[BH_IED_Caller] ERROR: No se pudo cargar el prefab del móvil.");
			return;
		}

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		spawnParams.Transform[3] = owner.GetOrigin();

		IEntity phoneEntity = GetGame().SpawnEntityPrefab(phoneRes, GetGame().GetWorld(), spawnParams);
		if (!phoneEntity)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Caller] ERROR: No se pudo spawnear el móvil.");
			return;
		}

		BH_IED_Utils.BH_Log("[BH_IED_Caller] Móvil spawneado. Intentando insertar en inventario...");

		// Insertar en inventario
		if (!invMgr.TryInsertItem(phoneEntity))
		{
			BH_IED_Utils.BH_Log("[BH_IED_Caller] AVISO: TryInsertItem falló. Eliminando móvil.");
			SCR_EntityHelper.DeleteEntityAndChildren(phoneEntity);
			return;
		}

		BH_IED_Utils.BH_Log("[BH_IED_Caller] Móvil añadido al inventario de la IA correctamente.");
	}

	// --------------------------------------------------------
	// Comprobación estática — ¿hay algún caller operativo?
	//
	// Si NO hay NINGÚN caller en el mundo → devuelve TRUE
	// (compatibilidad: el sistema funciona sin IAs con móvil)
	//
	// Si hay callers → necesita al menos uno vivo y
	// con cobertura de antena.
	// --------------------------------------------------------

	static bool BH_HasActiveCaller()
	{
		// Sin callers en el mapa → compatible con sistema clásico
		if (s_aCallers.Count() == 0)
		{
			BH_IED_Utils.BH_Log("[BH_IED_Caller] No hay callers en el mapa — sistema clásico.");
			return true;
		}

		foreach (BH_IED_Caller caller : s_aCallers)
		{
			if (!caller) continue;

			if (caller.BH_IsOperational())
			{
				BH_IED_Utils.BH_Log("[BH_IED_Caller] Caller operativo encontrado (vivo + cobertura).");
				return true;
			}
		}

		BH_IED_Utils.BH_Log("[BH_IED_Caller] Ningún caller operativo (todos muertos o sin cobertura).");
		return false;
	}
}