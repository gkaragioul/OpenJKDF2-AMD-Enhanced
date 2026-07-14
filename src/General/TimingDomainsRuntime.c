#include "General/TimingDomainsRuntime.h"

#include <stdio.h>
#include <string.h>

#include "General/DiagnosticLog.h"
#include "General/TimingDomainsScenario.h"

typedef struct TimingDomainsRuntimeState
{
    TimingDomainsObserver observer;
    TimingDomainsScenario scenario;
    uint64_t simulation_tick;
    uint64_t wall_time_us;
    uint64_t deadline_tick;
    int enabled;
    int frame_limit;
    int summary_written;
    int cutscene_request_consumed;
    int level_request_consumed;
} TimingDomainsRuntimeState;

static TimingDomainsRuntimeState timing_runtime;

static TimingDomainId timing_runtime_action_domain(TimingScenarioAction action)
{
    return (TimingDomainId)(action - TIMING_ACTION_START_WEAPON);
}

static void timing_runtime_log_record(const char* event_name, TimingDomainId id)
{
    const TimingDomainRecord* record = &timing_runtime.observer.domains[id];
    char event[512];
    snprintf(event, sizeof(event),
             "%s domain=%s simulation_tick=%llu wall_time_us=%llu expected_events=%u observed_events=%u status=%d reason=%s frame_limit=%d",
             event_name, TimingDomainsObserver_DomainName(id),
             (unsigned long long)(record->status == TIMING_STATUS_RUNNING ? record->start_tick : record->end_tick),
             (unsigned long long)(record->status == TIMING_STATUS_RUNNING ? record->start_wall_us : record->end_wall_us),
             record->expected_events, record->observed_events, (int)record->status,
             TimingDomainsObserver_ReasonName(record->reason), timing_runtime.frame_limit);
    diag_log_event(record->status == TIMING_STATUS_FAILED ? DIAG_SEVERITY_ERROR : DIAG_SEVERITY_INFO,
                   "timing_validation", event);
}

static void timing_runtime_write_summary(void)
{
    char event[256];
    int passed;
    if (!timing_runtime.enabled || timing_runtime.summary_written) return;
    passed = TimingDomainsObserver_Finalize(&timing_runtime.observer);
    snprintf(event, sizeof(event),
             "timing_domains_summary passed=%s domains=%d frame_limit=%d simulation_tick=%llu wall_time_us=%llu",
             passed ? "true" : "false", TIMING_DOMAIN_COUNT, timing_runtime.frame_limit,
             (unsigned long long)timing_runtime.simulation_tick,
             (unsigned long long)timing_runtime.wall_time_us);
    diag_log_event(passed ? DIAG_SEVERITY_INFO : DIAG_SEVERITY_ERROR, "timing_validation", event);
    timing_runtime.summary_written = 1;
}

static void timing_runtime_notify(TimingDomainId id)
{
    TimingDomainRecord* record;
    if (!timing_runtime.enabled || id < TIMING_DOMAIN_WEAPON || id >= TIMING_DOMAIN_COUNT) return;
    record = &timing_runtime.observer.domains[id];
    if (record->status != TIMING_STATUS_RUNNING) return;
    if (!TimingDomainsObserver_Notify(&timing_runtime.observer, id, timing_runtime.simulation_tick, timing_runtime.wall_time_us)) return;
    if (TimingDomainsObserver_Complete(&timing_runtime.observer, id, timing_runtime.simulation_tick, timing_runtime.wall_time_us)) {
        timing_runtime_log_record("timing_domain_complete", id);
    }
}

int TimingDomainsRuntime_Configure(StartupValidationObserver selection, const char* diagnostics_dir, int frame_limit)
{
    memset(&timing_runtime, 0, sizeof(timing_runtime));
    if (selection == STARTUP_VALIDATION_NONE) return 1;
    if (selection != STARTUP_VALIDATION_TIMING_DOMAINS || !diagnostics_dir || !diagnostics_dir[0] ||
        (frame_limit != 60 && frame_limit != 120)) return 0;
    TimingDomainsObserver_Init(&timing_runtime.observer);
    TimingDomainsScenario_Init(&timing_runtime.scenario);
    timing_runtime.enabled = 1;
    timing_runtime.frame_limit = frame_limit;
    return 1;
}

void TimingDomainsRuntime_Shutdown(void)
{
    timing_runtime_write_summary();
    timing_runtime.enabled = 0;
}

void TimingDomainsRuntime_Tick(uint64_t simulation_tick, uint64_t wall_time_us)
{
    TimingScenarioAction action;
    TimingDomainId id;
    if (!timing_runtime.enabled || timing_runtime.summary_written) return;
    timing_runtime.simulation_tick = simulation_tick;
    timing_runtime.wall_time_us = wall_time_us;
    action = TimingDomainsScenario_Update(&timing_runtime.scenario, &timing_runtime.observer, 1);
    if (action == TIMING_ACTION_FAIL || action == TIMING_ACTION_FINISH) {
        timing_runtime_write_summary();
        return;
    }
    if (action < TIMING_ACTION_START_WEAPON || action > TIMING_ACTION_REQUEST_LEVEL_TRANSITION) return;
    id = timing_runtime_action_domain(action);
    if (!timing_runtime.scenario.action_dispatched) {
        TimingDomainsObserver_Begin(&timing_runtime.observer, id, 1, simulation_tick, wall_time_us);
        timing_runtime.deadline_tick = simulation_tick + 15000;
        TimingDomainsScenario_MarkDispatched(&timing_runtime.scenario);
        timing_runtime_log_record("timing_domain_start", id);
    } else {
        TimingDomainsObserver_CheckTimeout(&timing_runtime.observer, id, timing_runtime.deadline_tick, simulation_tick, wall_time_us);
        if (timing_runtime.observer.domains[id].status == TIMING_STATUS_FAILED) {
            timing_runtime_log_record("timing_domain_complete", id);
        }
    }
}

int TimingDomainsRuntime_IsEnabled(void) { return timing_runtime.enabled; }
int TimingDomainsRuntime_IsFinished(void) { return timing_runtime.summary_written; }
int TimingDomainsRuntime_Passed(void) { return timing_runtime.summary_written && timing_runtime.observer.passed; }
int TimingDomainsRuntime_FrameLimit(void) { return timing_runtime.enabled ? timing_runtime.frame_limit : 0; }
int TimingDomainsRuntime_ShouldStartCutscene(void)
{
    if (!timing_runtime.enabled || timing_runtime.cutscene_request_consumed ||
        timing_runtime.scenario.action != TIMING_ACTION_START_CUTSCENE || !timing_runtime.scenario.action_dispatched) return 0;
    timing_runtime.cutscene_request_consumed = 1;
    return 1;
}
int TimingDomainsRuntime_ShouldRequestLevelTransition(void)
{
    if (!timing_runtime.enabled || timing_runtime.level_request_consumed ||
        timing_runtime.scenario.action != TIMING_ACTION_REQUEST_LEVEL_TRANSITION || !timing_runtime.scenario.action_dispatched) return 0;
    timing_runtime.level_request_consumed = 1;
    return 1;
}

void TimingDomainsRuntime_NotifyWeapon(void) { timing_runtime_notify(TIMING_DOMAIN_WEAPON); }
void TimingDomainsRuntime_NotifyAI(void) { timing_runtime_notify(TIMING_DOMAIN_AI); }
void TimingDomainsRuntime_NotifyPhysics(void) { timing_runtime_notify(TIMING_DOMAIN_PHYSICS); }
void TimingDomainsRuntime_NotifyAnimation(void) { timing_runtime_notify(TIMING_DOMAIN_ANIMATION); }
void TimingDomainsRuntime_NotifyParticle(void) { timing_runtime_notify(TIMING_DOMAIN_PARTICLE); }
void TimingDomainsRuntime_NotifyScript(void) { timing_runtime_notify(TIMING_DOMAIN_SCRIPT); }
void TimingDomainsRuntime_NotifyDialogue(void) { timing_runtime_notify(TIMING_DOMAIN_DIALOGUE); }
void TimingDomainsRuntime_NotifyCutscene(void) { timing_runtime_notify(TIMING_DOMAIN_CUTSCENE); }
void TimingDomainsRuntime_NotifyLevelTransition(void) { timing_runtime_notify(TIMING_DOMAIN_LEVEL_TRANSITION); }
