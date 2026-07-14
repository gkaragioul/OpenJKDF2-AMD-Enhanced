#include "General/SaveLoadProbe.h"

#include <assert.h>

int main(void)
{
    SaveLoadProbe probe = { 0 };
    RestoreProbe restore = { 0 };

    assert(save_load_probe_snapshot_health(100.0f) == 73.0f);
    assert(save_load_probe_snapshot_health(1.0f) == 1.0f);

    assert(save_load_probe_step(&probe, false, false) == SAVE_LOAD_PROBE_SAVE);
    assert(save_load_probe_step(&probe, true, false) == SAVE_LOAD_PROBE_NONE);
    assert(save_load_probe_step(&probe, false, false) == SAVE_LOAD_PROBE_PERTURB);
    assert(save_load_probe_step(&probe, false, false) == SAVE_LOAD_PROBE_RESTORE);
    assert(save_load_probe_step(&probe, true, false) == SAVE_LOAD_PROBE_NONE);
    assert(save_load_probe_step(&probe, false, true) == SAVE_LOAD_PROBE_COMPLETE);
    assert(save_load_probe_step(&probe, false, true) == SAVE_LOAD_PROBE_NONE);

    probe = (SaveLoadProbe){ 0 };
    assert(save_load_probe_step(&probe, false, false) == SAVE_LOAD_PROBE_SAVE);
    assert(save_load_probe_step(&probe, false, false) == SAVE_LOAD_PROBE_PERTURB);
    assert(save_load_probe_step(&probe, false, false) == SAVE_LOAD_PROBE_RESTORE);
    assert(save_load_probe_step(&probe, false, false) == SAVE_LOAD_PROBE_FAILED);
    assert(save_load_probe_step(&probe, false, true) == SAVE_LOAD_PROBE_NONE);

    assert(save_load_probe_step(NULL, false, false) == SAVE_LOAD_PROBE_NONE);

    assert(restore_probe_step(&restore, false, false) == SAVE_LOAD_PROBE_RESTORE);
    assert(restore_probe_step(&restore, true, false) == SAVE_LOAD_PROBE_NONE);
    assert(restore_probe_step(&restore, false, true) == SAVE_LOAD_PROBE_COMPLETE);
    assert(restore_probe_step(&restore, false, true) == SAVE_LOAD_PROBE_NONE);

    restore = (RestoreProbe){ 0 };
    assert(restore_probe_step(&restore, false, false) == SAVE_LOAD_PROBE_RESTORE);
    assert(restore_probe_step(&restore, false, false) == SAVE_LOAD_PROBE_FAILED);
    assert(restore_probe_step(NULL, false, false) == SAVE_LOAD_PROBE_NONE);
    return 0;
}
