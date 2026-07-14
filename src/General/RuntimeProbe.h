#ifndef OPENJKDF2_RUNTIME_PROBE_H
#define OPENJKDF2_RUNTIME_PROBE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct RuntimeProbe
{
    uint32_t start_ms;
    bool started;
    bool completed;
} RuntimeProbe;

bool runtime_probe_due(RuntimeProbe* probe, uint32_t now_ms, uint32_t delay_ms);
bool runtime_probe_should_skip_loading_wait(bool autostart);

#endif
