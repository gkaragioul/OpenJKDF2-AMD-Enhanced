#include "General/RuntimeProbe.h"

#include <assert.h>
#include <math.h>

int main(void)
{
    RuntimeProbe probe = { 0 };

    assert(!runtime_probe_due(&probe, 1000u, 5000u));
    assert(!runtime_probe_due(&probe, 5999u, 5000u));
    assert(runtime_probe_due(&probe, 6000u, 5000u));
    assert(!runtime_probe_due(&probe, 7000u, 5000u));

    probe = (RuntimeProbe){ 0 };
    assert(!runtime_probe_due(&probe, 0xFFFFFFF0u, 32u));
    assert(runtime_probe_due(&probe, 0x00000010u, 32u));
    assert(!runtime_probe_due(NULL, 1000u, 5000u));
    assert(!runtime_probe_due(&probe, 1000u, 0u));
    assert(runtime_probe_should_skip_loading_wait(true));
    assert(!runtime_probe_should_skip_loading_wait(false));

    assert(fabsf(runtime_probe_distance_squared(1.0f, 2.0f, 3.0f,
                                                4.0f, 6.0f, 3.0f) - 25.0f) < 0.0001f);
    assert(runtime_probe_moved(0.0f, 0.0f, 0.0f,
                               0.03f, 0.04f, 0.0f, 0.049f));
    assert(!runtime_probe_moved(0.0f, 0.0f, 0.0f,
                                0.03f, 0.04f, 0.0f, 0.051f));

    assert(fabsf(runtime_probe_angle_delta_degrees(350.0f, 10.0f) - 20.0f) < 0.0001f);
    assert(fabsf(runtime_probe_angle_delta_degrees(10.0f, 350.0f) + 20.0f) < 0.0001f);
    assert(fabsf(runtime_probe_angle_delta_degrees(-170.0f, 170.0f) + 20.0f) < 0.0001f);
    assert(runtime_probe_turned(179.0f, -179.0f, 1.5f));
    assert(!runtime_probe_turned(45.0f, 45.9f, 1.0f));
    return 0;
}
