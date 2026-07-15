#include "General/TimingDomainsRuntime.h"

#include <assert.h>
#include <string.h>

#include "General/DiagnosticLog.h"

static int saw_weapon_timeout;

bool diag_log_event(DiagSeverity severity, const char* subsystem, const char* event)
{
    (void)severity;
    if (subsystem && event && strcmp(subsystem, "timing_validation") == 0 &&
        strstr(event, "domain=weapon") && strstr(event, "reason=timeout")) {
        saw_weapon_timeout = 1;
    }
    return true;
}

static void test_weapon_activation_request_is_rate_limited_and_retried(void)
{
    assert(TimingDomainsRuntime_Configure(STARTUP_VALIDATION_TIMING_DOMAINS,
                                          "diagnostics", 60));

    TimingDomainsRuntime_Tick(100, 1000);
    assert(TimingDomainsRuntime_ShouldActivateWeapon());
    assert(!TimingDomainsRuntime_ShouldActivateWeapon());

    TimingDomainsRuntime_Tick(349, 250000);
    assert(!TimingDomainsRuntime_ShouldActivateWeapon());
    TimingDomainsRuntime_Tick(350, 251000);
    assert(TimingDomainsRuntime_ShouldActivateWeapon());

    TimingDomainsRuntime_Shutdown();
}

static void test_unfired_weapon_times_out_the_validation_run(void)
{
    assert(TimingDomainsRuntime_Configure(STARTUP_VALIDATION_TIMING_DOMAINS,
                                          "diagnostics", 60));

    TimingDomainsRuntime_Tick(100, 1000);
    assert(TimingDomainsRuntime_IsEnabled());
    assert(!TimingDomainsRuntime_IsFinished());

    TimingDomainsRuntime_Tick(15101, 16000000);
    TimingDomainsRuntime_Tick(15102, 16001000);

    assert(saw_weapon_timeout);
    assert(TimingDomainsRuntime_IsFinished());
    assert(!TimingDomainsRuntime_Passed());
    TimingDomainsRuntime_Shutdown();
}

int main(void)
{
    test_weapon_activation_request_is_rate_limited_and_retried();
    test_unfired_weapon_times_out_the_validation_run();
    return 0;
}
