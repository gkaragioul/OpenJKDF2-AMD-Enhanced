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
float runtime_probe_distance_squared(float start_x, float start_y, float start_z,
                                     float end_x, float end_y, float end_z);
bool runtime_probe_moved(float start_x, float start_y, float start_z,
                         float end_x, float end_y, float end_z, float minimum_distance);
float runtime_probe_angle_delta_degrees(float start_degrees, float end_degrees);
bool runtime_probe_turned(float start_degrees, float end_degrees, float minimum_degrees);
bool runtime_probe_parse_degrees(const char* text, float* out_degrees);

#endif
