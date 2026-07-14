#include "General/RuntimeProbe.h"

#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>

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

bool runtime_probe_parse_degrees(const char* text, float* out_degrees)
{
    char* end = NULL;
    float value;
    if (!text || !text[0] || !out_degrees)
        return false;
    errno = 0;
    value = strtof(text, &end);
    if (errno == ERANGE || end == text || !end || end[0] != '\0' ||
        !isfinite(value) || value < -180.0f || value > 180.0f)
        return false;
    *out_degrees = value;
    return true;
}

bool runtime_probe_parse_rate(const char* text, int* out_rate)
{
    char* end = NULL;
    long value;
    if (!text || !text[0] || !out_rate)
        return false;
    errno = 0;
    value = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || !end || end[0] != '\0' || value < 30 || value > 1000)
        return false;
    *out_rate = (int)value;
    return true;
}

static bool runtime_probe_text_equal_ignore_case(const char* left, const char* right)
{
    if (!left || !right)
        return false;
    while (*left && *right)
    {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
            return false;
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

bool runtime_probe_parse_vsync(const char* text, int* out_mode)
{
    if (!text || !text[0] || !out_mode)
        return false;
    if (runtime_probe_text_equal_ignore_case(text, "off"))
        *out_mode = 0;
    else if (runtime_probe_text_equal_ignore_case(text, "on"))
        *out_mode = 1;
    else if (runtime_probe_text_equal_ignore_case(text, "adaptive"))
        *out_mode = -1;
    else
        return false;
    return true;
}
