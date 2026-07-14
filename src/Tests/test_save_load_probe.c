#include "General/SaveLoadProbe.h"

#include <assert.h>

int main(void)
{
    SaveLoadProbe probe = { 0 };
    RestoreProbe restore = { 0 };
    DeathReloadProbe death = { 0 };

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

    assert(death_reload_probe_step(&death, 1000u, true, false, false, 3000u) == SAVE_LOAD_PROBE_NONE);
    assert(death_reload_probe_step(&death, 1100u, false, false, false, 3000u) == SAVE_LOAD_PROBE_KILL);
    assert(death_reload_probe_step(&death, 1200u, false, true, false, 3000u) == SAVE_LOAD_PROBE_NONE);
    assert(death_reload_probe_step(&death, 4199u, false, true, false, 3000u) == SAVE_LOAD_PROBE_NONE);
    assert(death_reload_probe_step(&death, 4200u, false, true, false, 3000u) == SAVE_LOAD_PROBE_RELOAD);
    assert(death_reload_probe_step(&death, 4300u, true, true, false, 3000u) == SAVE_LOAD_PROBE_NONE);
    assert(death_reload_probe_step(&death, 4400u, false, false, true, 3000u) == SAVE_LOAD_PROBE_COMPLETE);
    assert(death_reload_probe_step(&death, 4500u, false, false, true, 3000u) == SAVE_LOAD_PROBE_NONE);

    death = (DeathReloadProbe){ 0 };
    assert(death_reload_probe_step(&death, 0xFFFFFFF0u, false, false, false, 32u) == SAVE_LOAD_PROBE_KILL);
    assert(death_reload_probe_step(&death, 0xFFFFFFF1u, false, true, false, 32u) == SAVE_LOAD_PROBE_NONE);
    assert(death_reload_probe_step(&death, 0x00000011u, false, true, false, 32u) == SAVE_LOAD_PROBE_RELOAD);
    assert(death_reload_probe_step(&death, 0x00000012u, false, false, false, 32u) == SAVE_LOAD_PROBE_FAILED);
    assert(death_reload_probe_step(NULL, 0u, false, false, false, 3000u) == SAVE_LOAD_PROBE_NONE);
    return 0;
}
