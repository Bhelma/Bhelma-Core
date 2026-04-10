modded class SCR_WorldTools
{
    static bool BH_IsNightTime()
    {
        ChimeraWorld world = GetGame().GetWorld();
        TimeAndWeatherManagerEntity timeMgr = world.GetTimeAndWeatherManager();

        float sunsetTime, sunriseTime, currentTime;

        timeMgr.GetSunsetHour(sunsetTime);
        timeMgr.GetSunriseHour(sunriseTime);
        sunsetTime = sunsetTime + 0.5;
        sunriseTime = sunriseTime - 0.5;

        currentTime = timeMgr.GetTimeOfTheDay();

        if (currentTime > sunsetTime || currentTime < sunriseTime)
            return true;

        return false;
    }

    static bool BH_SnapEntityToGeometry(IEntity entity)
    {
        const BaseWorld world = GetGame().GetWorld();

        TraceParam trace = new TraceParam();

        vector start = entity.GetOrigin();
        vector end = start;

        start[1] = start[1] + 1;
        end[1] = world.GetSurfaceY(end[0], end[2]);

        if (start[1] < end[1])
            start[1] = end[1] + 1;

        trace.Start = start;
        trace.End = end;

        array<IEntity> exclude = {entity};

        trace.ExcludeArray = exclude;
        trace.TargetLayers = EPhysicsLayerDefs.FireGeometry;
        trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;

        float traceDistPercentage = world.TraceMove(trace, null);

        if (traceDistPercentage > 0)
        {
            start[1] = trace.Start[1] + (trace.End[1] - trace.Start[1]) * traceDistPercentage;
            entity.SetOrigin(start);
            return true;
        }

        return false;
    }
}