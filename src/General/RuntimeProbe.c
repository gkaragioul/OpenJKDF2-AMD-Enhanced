#include "General/RuntimeProbe.h"

bool runtime_probe_due(RuntimeProbe* probe, uint32_t now_ms, uint32_t delay_ms)
{
    if (!probe || !delay_ms || probe->completed)
        return false;
    if (!probe->started)
    {
        probe->start_ms = now_ms;
        probe->started = true;
        return false;
    }
    if ((uint32_t)(now_ms - probe->start_ms) < delay_ms)
        return false;
    probe->completed = true;
    return true;
}

bool runtime_probe_should_skip_loading_wait(bool autostart)
{
    return autostart;
}
