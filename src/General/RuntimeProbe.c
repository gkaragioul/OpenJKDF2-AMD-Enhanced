#include "General/RuntimeProbe.h"

#include <math.h>

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

float runtime_probe_distance_squared(float start_x, float start_y, float start_z,
                                     float end_x, float end_y, float end_z)
{
    const float dx = end_x - start_x;
    const float dy = end_y - start_y;
    const float dz = end_z - start_z;
    return dx * dx + dy * dy + dz * dz;
}

bool runtime_probe_moved(float start_x, float start_y, float start_z,
                         float end_x, float end_y, float end_z, float minimum_distance)
{
    if (minimum_distance < 0.0f)
        minimum_distance = -minimum_distance;
    return runtime_probe_distance_squared(start_x, start_y, start_z, end_x, end_y, end_z) >=
           minimum_distance * minimum_distance;
}

float runtime_probe_angle_delta_degrees(float start_degrees, float end_degrees)
{
    float delta = end_degrees - start_degrees;
    while (delta > 180.0f)
        delta -= 360.0f;
    while (delta < -180.0f)
        delta += 360.0f;
    return delta;
}

bool runtime_probe_turned(float start_degrees, float end_degrees, float minimum_degrees)
{
    if (minimum_degrees < 0.0f)
        minimum_degrees = -minimum_degrees;
    return fabsf(runtime_probe_angle_delta_degrees(start_degrees, end_degrees)) >= minimum_degrees;
}
