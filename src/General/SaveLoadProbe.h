#ifndef OPENJKDF2_SAVE_LOAD_PROBE_H
#define OPENJKDF2_SAVE_LOAD_PROBE_H

#include <stdbool.h>

typedef enum SaveLoadProbeAction
{
    SAVE_LOAD_PROBE_NONE = 0,
    SAVE_LOAD_PROBE_SAVE,
    SAVE_LOAD_PROBE_PERTURB,
    SAVE_LOAD_PROBE_RESTORE,
    SAVE_LOAD_PROBE_COMPLETE,
    SAVE_LOAD_PROBE_FAILED
} SaveLoadProbeAction;

typedef struct SaveLoadProbe
{
    unsigned int phase;
} SaveLoadProbe;

typedef struct RestoreProbe
{
    unsigned int phase;
} RestoreProbe;

SaveLoadProbeAction save_load_probe_step(SaveLoadProbe* probe, bool save_system_busy, bool state_matches);
SaveLoadProbeAction restore_probe_step(RestoreProbe* probe, bool save_system_busy, bool state_matches);
float save_load_probe_snapshot_health(float max_health);

#endif
