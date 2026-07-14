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

SaveLoadProbeAction death_reload_probe_step(DeathReloadProbe* probe, uint32_t now_ms,
                                            bool save_system_busy, bool player_dead,
                                            bool state_matches, uint32_t reload_delay_ms)
{
    if (!probe)
        return SAVE_LOAD_PROBE_NONE;

    switch (probe->phase)
    {
        case 0:
            if (save_system_busy)
                return SAVE_LOAD_PROBE_NONE;
            probe->phase = 1;
            return SAVE_LOAD_PROBE_KILL;
        case 1:
            if (!player_dead)
                return SAVE_LOAD_PROBE_NONE;
            probe->death_observed_ms = now_ms;
            probe->phase = 2;
            return SAVE_LOAD_PROBE_NONE;
        case 2:
            if ((uint32_t)(now_ms - probe->death_observed_ms) < reload_delay_ms)
                return SAVE_LOAD_PROBE_NONE;
            probe->phase = 3;
            return SAVE_LOAD_PROBE_RELOAD;
        case 3:
            if (save_system_busy)
                return SAVE_LOAD_PROBE_NONE;
            probe->phase = 4;
            return !player_dead && state_matches ? SAVE_LOAD_PROBE_COMPLETE : SAVE_LOAD_PROBE_FAILED;
        default:
            return SAVE_LOAD_PROBE_NONE;
    }
}
