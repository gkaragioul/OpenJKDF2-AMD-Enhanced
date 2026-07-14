#include "General/RuntimeProbe.h"

#include <assert.h>

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
    return 0;
}
