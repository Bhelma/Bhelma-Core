modded class SCR_EmptyPositionHelper
{
    static bool BH_TryFindNearbyGeometryPositionForEntity(IEntity entity = null, vector optionalPosition = vector.Zero, vector bounds_mins = vector.Zero, vector bounds_max = vector.Zero, out vector outPos = vector.Zero, array<IEntity> optionalExcludeEntitys = null)
    {
        const vector charBoundsMin = "-0.65 0.004 -0.25";
        const vector charBoundsMax = "0.65 1.7 0.35";

        BaseWorld world = GetGame().GetWorld();
        vector mins, maxs;
        vector position = optionalPosition;

        if (entity)
        {
            if (position == vector.Zero)
                position = entity.GetOrigin();

            if (SCR_ChimeraCharacter.Cast(entity))
            {
                mins = charBoundsMin;
                maxs = charBoundsMax;
            }
            else
            {
                entity.GetBounds(mins, maxs);
            }
        }
        else
        {
            if (optionalPosition == vector.Zero)
            {
                Print("[BH_EmptyPositionHelper] BH_TryFindNearbyGeometryPositionForEntity failed! No entity and no position is set!", LogLevel.WARNING);
                return false;
            }
            if (bounds_mins == vector.Zero || bounds_max == vector.Zero)
            {
                Print("[BH_EmptyPositionHelper] BH_TryFindNearbyGeometryPositionForEntity failed! No bounds were set!", LogLevel.WARNING);
                return false;
            }
            mins = bounds_mins;
            maxs = bounds_max;
        }

        TraceParam traceParams = new TraceParam();
        vector start = position;
        vector end = position;

        start[1] = position[1] + 0.3;
        end[1] = world.GetSurfaceY(end[0], end[2]);

        traceParams.Start = start;
        traceParams.End = end;

        array<IEntity> exclude = {};
        if (entity) exclude.Insert(entity);
        if (optionalExcludeEntitys && optionalExcludeEntitys.Count() > 0)
            exclude.InsertAll(optionalExcludeEntitys);

        traceParams.ExcludeArray = exclude;
        traceParams.LayerMask = EPhysicsLayerDefs.Projectile;
        traceParams.Flags = TraceFlags.ENTS | TraceFlags.WORLD;

        const float traceDistPercentage = world.TraceMove(traceParams, null);

        if (traceDistPercentage > 0)
            position[1] = 0.05 + traceParams.Start[1] + (traceParams.End[1] - traceParams.Start[1]) * traceDistPercentage;

        const float horizontalDiagonalLength = Math.Sqrt(Pow2(maxs[0] - mins[0]) + Pow2(maxs[2] - mins[2]));
        const float cuboidDiagonalLength = Math.Sqrt(Pow2(horizontalDiagonalLength) + Pow2(maxs[1] - mins[1]));

        TraceSphere trace = new TraceSphere();
        trace.Radius = 0.5 * horizontalDiagonalLength;
        trace.Exclude = entity;
        const float entityHeight = maxs[1] - mins[1];
        const float spacing = 2 * cuboidDiagonalLength;
        const float maxHorizontalDistance = 7 * spacing;
        const float maxSubmersionDepth = 0.5 * entityHeight;

        bool success = TryFindNearbyFloorPosition(world, position, trace, entityHeight, spacing, maxHorizontalDistance, false, maxSubmersionDepth, outPos);
        if (!success)
        {
            outPos = position;
            return false;
        }

        if (!float.AlmostEqual(position[1], outPos[1], 2))
            outPos = position;

        return true;
    }
}