[BaseContainerProps()]
class BH_AmbientHelicopterWaypoint
{
    [Attribute("", UIWidgets.Auto, desc: "NOMBRE de la entidad de punto de referencia (Cadena) La entidad debe ubicarse en el mundo (por ejemplo, SCR_Position) o el nombre de waypoints ubicados en el mapa.", category: "BH Ambient Helicopter Waypoint")]
    string m_WaypoinName;

    [Attribute("0", UIWidgets.CheckBox, desc: "¿Deberíamos aterrizar en este punto de referencia?", category: "BH Ambient Helicopter Waypoint")]
    bool m_IsLandingEvent;

    [Attribute("0", UIWidgets.CheckBox, desc: "¿Apagar el motor? AVISO: Si se activa, el helicoptero NO podra volver a arrancar. Usar solo si este es el destino final del helicoptero. Desmarcar si debe volver a despegar.", category: "BH Ambient Helicopter Waypoint")]
    bool m_ShutdownEngine;

    [Attribute("5", UIWidgets.Auto, desc: "Si no es el último punto de referencia después del aterrizaje, ¿cuántos segundos deberíamos permanecer aquí?", category: "BH Ambient Helicopter Waypoint")]
    int m_WaitSeconds;

    [Attribute("0", UIWidgets.CheckBox, desc: "Si el helicóptero está en tierra, ¿debemos esperar a que todos los jugadores suban a bordo?. Si el primer punto esta al lado del spawn del helicoptero y se marca esta casilla, el helicoptero espera al embarque de los jugadores antes de despegar.", category: "BH Ambient Helicopter Waypoint")]
    bool m_WaitForPlayersGetIn;

    [Attribute("0", UIWidgets.CheckBox, desc: "Si el helicóptero está en tierra, ¿debemos esperar a que todos los jugadores salgan del helicóptero?", category: "BH Ambient Helicopter Waypoint")]
    bool m_WaitForPlayersGetOut;

    [Attribute("", UIWidgets.EditBox, desc: "Especifique una entidad 'NOMBRE' que debe estar en el helicóptero hasta que se maneje el siguiente punto de referencia", category: "BH Ambient Helicopter Waypoint")]
    string m_ImportantEntityName;

    [Attribute("0", UIWidgets.CheckBox, desc: "¿Eliminar propietario (helicóptero) en este punto de referencia? (Última acción)", category: "BH Ambient Helicopter Waypoint")]
    bool m_DeleteHelicopter;

    [Attribute("0", UIWidgets.CheckBox, desc: "¿Deberíamos anular la velocidad máxima para este punto de referencia?", category: "BH Ambient Helicopter Waypoint")]
    bool m_DoOverrideMaxSpeed;

    [Attribute("100", UIWidgets.Auto, desc: "Anular la velocidad máxima para este punto de referencia", params: "20 120", category: "BH Ambient Helicopter Waypoint")]
    float m_OverrideMaxSpeed;

    [Attribute("0", UIWidgets.CheckBox, desc: "¿Deberíamos anular la potencia de rotación máxima para este punto de referencia?", category: "BH Ambient Helicopter Waypoint")]
    bool m_DoOverrideRotationPowerDividor;

    [Attribute("15", UIWidgets.Auto, desc: "¿Qué tan rápido podemos girar? (valor más bajo = giros más rápidos, valor más alto = giros más lentos)", params: "10 20", category: "BH Ambient Helicopter Waypoint")]
    float m_OverrideRotationPowerDividor;

    [Attribute("0", UIWidgets.CheckBox, desc: "¿Deberíamos anular la altura sobre el terreno mientras volamos hacia este punto de referencia?", category: "BH Ambient Helicopter Waypoint")]
    bool m_DoOverrideMinHeightAboveTerrain;

    [Attribute("35", UIWidgets.Auto, desc: "Anular la altura sobre el terreno mientras vuela", params: "10 100", category: "BH Ambient Helicopter Waypoint")]
    int m_OverrideMinHeightAboveTerrain;

    [Attribute(desc: "Acciones que se activarán al alcanzar el Waypoint", category: "BH Ambient Helicopter Waypoint")]
    ref array<ref SCR_ScenarioFrameworkActionBase> m_aActionsOnWaypoint;

    [Attribute(desc: "Acciones que se activarán cuando la condición del jugador definida se haga verdadera", category: "BH Ambient Helicopter Waypoint")]
    ref array<ref SCR_ScenarioFrameworkActionBase> m_aActionsOnPlayerCondition;
	
	[Attribute("0", UIWidgets.CheckBox, desc: "Usar numero minimo fijo de jugadores", category: "BH Ambient Helicopter Waypoint")]
	bool m_UseFixedPlayerCount;
	
	[Attribute("6", UIWidgets.EditBox, desc: "Numero minimo fijo de pasajeros (jugadores + IAs) para despegar", category: "BH Ambient Helicopter Waypoint")]
	int m_FixedPlayerCount;
	
	[Attribute("120", UIWidgets.EditBox, desc: "Segundos maximos de espera desde que sube el primero", category: "BH Ambient Helicopter Waypoint")]
	int m_WaitTimeout;
	
	[Attribute("10", UIWidgets.EditBox, desc: "Segundos adicionales al alcanzar el minimo antes de despegar", category: "BH Ambient Helicopter Waypoint")]
	int m_WaitExtraTime;
	
	[Attribute("30", UIWidgets.EditBox, desc: "Cada cuantos segundos el piloto manda mensaje de radio", category: "BH Ambient Helicopter Waypoint")]
	int m_RadioMessageInterval;
	
	[Attribute("Attention all units, boarding sequence initiated", UIWidgets.EditBox, desc: "Mensaje inicial cuando sube el primero", category: "BH Ambient Helicopter Waypoint")]
	string m_RadioMsgBoarding;
	
	[Attribute("Minimum crew on board, departing soon", UIWidgets.EditBox, desc: "Mensaje al alcanzar el minimo", category: "BH Ambient Helicopter Waypoint")]
	string m_RadioMsgMinReached;
	
	[Attribute("Time is up, lifting off now", UIWidgets.EditBox, desc: "Mensaje al agotar el tiempo", category: "BH Ambient Helicopter Waypoint")]
	string m_RadioMsgTimeout;
	
	[Attribute("Hurry up, get on board", UIWidgets.EditBox, desc: "Mensaje periodico de aviso", category: "BH Ambient Helicopter Waypoint")]
	string m_RadioMsgPeriodic;
	
	[Attribute("", UIWidgets.EditBox, "Faction key que recibe los mensajes de radio (vacio = todos)", category: "BH Ambient Helicopter Waypoint")]
	string m_sRadioFactionKey;
}