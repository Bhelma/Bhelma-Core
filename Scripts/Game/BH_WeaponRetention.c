/*
=============================================================================
    BH_WeaponRetention.c
    
    DESCRIPCIÓN:
    Este script mantiene el arma del jugador colgada del pecho cuando
    cae inconsciente, simulando una correa de retención de arma (sling).
    Al recuperar la consciencia, el jugador ya tiene el arma consigo y
    puede volver a empuñarla sin necesidad de buscarla por el suelo.
    
    FUNCIONAMIENTO:
    Usamos UpdateConsciousness en SCR_CharacterDamageManagerComponent,
    que es el punto exacto donde Reforger gestiona la inconsciencia tanto
    para jugadores humanos como para IA, en local, multijugador y servidor
    dedicado. Al caer inconsciente colgamos el arma del pecho con
    EEquipTypeUnarmedContextual. Al recuperarse la volvemos a equipar.
    
    INSTALACIÓN:
    Coloca este archivo en Scripts/Game/ dentro de tu addon.
    Se aplica automáticamente a todas las misiones sin modificarlas.
    
    COMPATIBILIDAD:
    No requiere mochila ni inventario adicional. Funciona con cualquier
    rol ya que el arma queda colgada del cuerpo del personaje.
    
    PREFIJO DE FUNCIONES: BH_
=============================================================================
*/

modded class SCR_CharacterDamageManagerComponent : ScriptedDamageManagerComponent
{
    // Referencia directa al arma retenida mientras el jugador está inconsciente
    protected IEntity m_BH_ArmaRetenida;

    // -----------------------------------------------------------------------
    // OVERRIDE: UpdateConsciousness
    // Punto exacto donde Reforger evalúa y aplica el estado de inconsciencia.
    // Al caer inconsciente colgamos el arma del pecho ANTES de que el juego
    // pueda soltarla al suelo. Al recuperarse la volvemos a equipar.
    // -----------------------------------------------------------------------
    override void UpdateConsciousness()
    {
        bool inconsciente = ShouldBeUnconscious();

        // Si debe morir, matamos al personaje y salimos
        if (inconsciente && (!GetPermitUnconsciousness() || (m_pBloodHitZone && m_pBloodHitZone.GetDamageState() == ECharacterBloodState.DESTROYED)))
        {
            Kill(GetInstigator());
            return;
        }

        ChimeraCharacter personaje = ChimeraCharacter.Cast(GetOwner());
        if (!personaje)
            return;

        CharacterControllerComponent controlador = personaje.GetCharacterController();
        if (!controlador)
            return;

        if (inconsciente)
        {
            // Colgamos el arma del pecho ANTES de aplicar la inconsciencia
            // para que el juego no pueda soltarla al suelo
            BH_ColgarArmaDelPecho(controlador);
        }
        else if (m_BH_ArmaRetenida)
        {
            // El personaje se recupera: volvemos a equipar el arma retenida
            BH_EquiparArmaRetenida(controlador);
        }

        // Aplicamos el estado de inconsciencia al controlador
        controlador.SetUnconscious(inconsciente);
    }

    // -----------------------------------------------------------------------
    // FUNCIÓN: BH_ColgarArmaDelPecho
    // Guarda la referencia al arma actual y la cuelga del pecho de forma
    // nativa usando EEquipTypeUnarmedContextual. El arma no cae al suelo
    // y no necesita inventario ni mochila.
    // -----------------------------------------------------------------------
    protected void BH_ColgarArmaDelPecho(CharacterControllerComponent controlador)
    {
        BaseWeaponManagerComponent gestorArmas = controlador.GetWeaponManagerComponent();
        if (!gestorArmas)
        {
            m_BH_ArmaRetenida = null;
            return;
        }

        BaseWeaponComponent armaActual = gestorArmas.GetCurrentWeapon();
        if (!armaActual)
        {
            // No llevaba arma en mano, nada que retener
            m_BH_ArmaRetenida = null;
            return;
        }

        // Guardamos la referencia al arma antes de colgarla
        m_BH_ArmaRetenida = armaActual.GetOwner();

        // EEquipTypeUnarmedContextual cuelga el arma del pecho de forma nativa.
        // Es el mismo mecanismo que usa el juego cuando el jugador guarda
        // el arma manualmente. No la suelta al suelo y no necesita mochila.
        controlador.TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedContextual, true);
    }

    // -----------------------------------------------------------------------
    // FUNCIÓN: BH_EquiparArmaRetenida
    // Usa la referencia guardada para volver a equipar el arma en la mano
    // al recuperar la consciencia.
    // -----------------------------------------------------------------------
    protected void BH_EquiparArmaRetenida(CharacterControllerComponent controlador)
    {
        if (!m_BH_ArmaRetenida)
            return;

        bool resultado = controlador.TryEquipRightHandItem(m_BH_ArmaRetenida, EEquipItemType.EEquipTypeWeapon, true);

        // Limpiamos la referencia solo si el arma se equipó correctamente
        if (resultado)
            m_BH_ArmaRetenida = null;
    }
}