#include "General/SaveLoadProbe.h"

SaveLoadProbeAction save_load_probe_step(SaveLoadProbe* probe, bool save_system_busy, bool state_matches)
{
    if (!probe)
        return SAVE_LOAD_PROBE_NONE;

    switch (probe->phase)
    {
        case 0:
            probe->phase = 1;
            return SAVE_LOAD_PROBE_SAVE;
        case 1:
            if (save_system_busy)
                return SAVE_LOAD_PROBE_NONE;
            probe->phase = 2;
            return SAVE_LOAD_PROBE_PERTURB;
        case 2:
            probe->phase = 3;
            return SAVE_LOAD_PROBE_RESTORE;
        case 3:
            if (save_system_busy)
                return SAVE_LOAD_PROBE_NONE;
            probe->phase = 4;
            return state_matches ? SAVE_LOAD_PROBE_COMPLETE : SAVE_LOAD_PROBE_FAILED;
        default:
            return SAVE_LOAD_PROBE_NONE;
    }
}

SaveLoadProbeAction restore_probe_step(RestoreProbe* probe, bool save_system_busy, bool state_matches)
{
    if (!probe)
        return SAVE_LOAD_PROBE_NONE;
    if (probe->phase == 0)
    {
        probe->phase = 1;
        return SAVE_LOAD_PROBE_RESTORE;
    }
    if (probe->phase == 1)
    {
        if (save_system_busy)
            return SAVE_LOAD_PROBE_NONE;
        probe->phase = 2;
        return state_matches ? SAVE_LOAD_PROBE_COMPLETE : SAVE_LOAD_PROBE_FAILED;
    }
    return SAVE_LOAD_PROBE_NONE;
}

float save_load_probe_snapshot_health(float max_health)
{
    return max_health > 1.0f ? max_health * 0.73f : max_health;
}
