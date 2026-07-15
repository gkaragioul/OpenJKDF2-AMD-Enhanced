#include "General/TimingDomainsRuntime.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "General/DiagnosticLog.h"
#include "General/TimingDomainsScenario.h"

#define TIMING_WEAPON_RETRY_MS 250u
#define TIMING_AI_OBSERVATION_MS 500u
#define TIMING_PHYSICS_MIN_DISPLACEMENT 0.01f
#define TIMING_SCRIPT_TASK_ID 5
#define TIMING_LEVEL_TARGET "02narshadda.jkl"

typedef struct TimingDomainsRuntimeState
{
    TimingDomainsObserver observer;
    TimingDomainsScenario scenario;
    TimingDomainsClock clock;
    uint64_t simulation_tick;
    uint64_t wall_time_us;
    uint64_t deadline_tick;
    uint64_t weapon_arm_tick;
    uint64_t weapon_arm_wall_us;
    uint64_t weapon_next_activation_tick;
    uint64_t weapon_cooldown_deadline_tick;
    const void* target_primary;
    int target_secondary;
    int target_registered;
    float physics_start_x;
    float physics_start_y;
    float physics_start_z;
    int enabled;
    int frame_limit;
    int summary_written;
    int request_consumed[TIMING_DOMAIN_COUNT];
} TimingDomainsRuntimeState;

static TimingDomainsRuntimeState timing_runtime;

static TimingDomainId timing_runtime_action_domain(TimingScenarioAction action)
{
    return (TimingDomainId)(action - TIMING_ACTION_START_WEAPON);
}

static int timing_runtime_current_domain(TimingDomainId id)
{
    TimingScenarioAction action = timing_runtime.scenario.action;
    return timing_runtime.enabled && action >= TIMING_ACTION_START_WEAPON &&
           action <= TIMING_ACTION_REQUEST_LEVEL_TRANSITION &&
           timing_runtime_action_domain(action) == id &&
           timing_runtime.scenario.action_dispatched;
}

static void timing_runtime_reset_target(void)
{
    timing_runtime.target_primary = NULL;
    timing_runtime.target_secondary = 0;
    timing_runtime.target_registered = 0;
    timing_runtime.physics_start_x = 0.0f;
    timing_runtime.physics_start_y = 0.0f;
    timing_runtime.physics_start_z = 0.0f;
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

static void timing_runtime_log_armed(TimingDomainId id)
{
    char event[256];
    snprintf(event, sizeof(event),
             "timing_domain_armed domain=%s simulation_tick=%llu wall_time_us=%llu frame_limit=%d",
             TimingDomainsObserver_DomainName(id),
             (unsigned long long)timing_runtime.simulation_tick,
             (unsigned long long)timing_runtime.wall_time_us,
             timing_runtime.frame_limit);
    diag_log_event(DIAG_SEVERITY_INFO, "timing_validation", event);
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

static void timing_runtime_complete(TimingDomainId id)
{
    TimingDomainRecord* record;
    if (!timing_runtime_current_domain(id)) return;
    record = &timing_runtime.observer.domains[id];
    if (record->status != TIMING_STATUS_RUNNING) return;
    if (!TimingDomainsObserver_Notify(&timing_runtime.observer, id,
                                      timing_runtime.simulation_tick, timing_runtime.wall_time_us)) return;
    if (TimingDomainsObserver_Complete(&timing_runtime.observer, id,
                                       timing_runtime.simulation_tick, timing_runtime.wall_time_us)) {
        timing_runtime_log_record("timing_domain_complete", id);
        if (id == TIMING_DOMAIN_LEVEL_TRANSITION &&
            TimingDomainsScenario_Update(&timing_runtime.scenario, &timing_runtime.observer, 1) == TIMING_ACTION_FINISH) {
            timing_runtime_write_summary();
        }
    }
}

static int timing_runtime_should_dispatch(TimingDomainId id)
{
    if (!timing_runtime_current_domain(id) || timing_runtime.request_consumed[id]) return 0;
    timing_runtime.request_consumed[id] = 1;
    return 1;
}

int TimingDomainsRuntime_Configure(StartupValidationObserver selection, const char* diagnostics_dir, int frame_limit)
{
    memset(&timing_runtime, 0, sizeof(timing_runtime));
    if (selection == STARTUP_VALIDATION_NONE) return 1;
    if (selection != STARTUP_VALIDATION_TIMING_DOMAINS || !diagnostics_dir || !diagnostics_dir[0] ||
        (frame_limit != 60 && frame_limit != 120)) return 0;
    TimingDomainsObserver_Init(&timing_runtime.observer);
    TimingDomainsScenario_Init(&timing_runtime.scenario);
    TimingDomainsClock_Init(&timing_runtime.clock);
    timing_runtime.enabled = 1;
    timing_runtime.frame_limit = frame_limit;
    return 1;
}

void TimingDomainsRuntime_Shutdown(void)
{
    timing_runtime_write_summary();
    timing_runtime.enabled = 0;
}

void TimingDomainsRuntime_RefreshClock(uint64_t simulation_tick, uint64_t wall_time_us)
{
    if (!timing_runtime.enabled || timing_runtime.summary_written) return;
    timing_runtime.simulation_tick = TimingDomainsClock_Update(&timing_runtime.clock, simulation_tick);
    timing_runtime.wall_time_us = wall_time_us;
}

void TimingDomainsRuntime_Tick(uint64_t simulation_tick, uint64_t wall_time_us)
{
    TimingScenarioAction action;
    TimingDomainId id;
    if (!timing_runtime.enabled || timing_runtime.summary_written) return;
    TimingDomainsRuntime_RefreshClock(simulation_tick, wall_time_us);
    action = TimingDomainsScenario_Update(&timing_runtime.scenario, &timing_runtime.observer, 1);
    if (action == TIMING_ACTION_FAIL || action == TIMING_ACTION_FINISH) {
        timing_runtime_write_summary();
        return;
    }
    if (action < TIMING_ACTION_START_WEAPON || action > TIMING_ACTION_REQUEST_LEVEL_TRANSITION) return;
    id = timing_runtime_action_domain(action);
    if (!timing_runtime.scenario.action_dispatched) {
        timing_runtime_reset_target();
        timing_runtime.deadline_tick = timing_runtime.simulation_tick + 15000;
        TimingDomainsScenario_MarkDispatched(&timing_runtime.scenario);
        if (id == TIMING_DOMAIN_WEAPON) {
            timing_runtime.weapon_arm_tick = timing_runtime.simulation_tick;
            timing_runtime.weapon_arm_wall_us = timing_runtime.wall_time_us;
            timing_runtime.weapon_next_activation_tick = timing_runtime.simulation_tick;
            timing_runtime_log_armed(id);
        } else {
            TimingDomainsObserver_Begin(&timing_runtime.observer, id, 1,
                                        timing_runtime.simulation_tick, timing_runtime.wall_time_us);
            timing_runtime_log_record("timing_domain_start", id);
        }
    } else {
        if (id == TIMING_DOMAIN_WEAPON &&
            timing_runtime.observer.domains[id].status == TIMING_STATUS_NOT_STARTED &&
            timing_runtime.simulation_tick > timing_runtime.deadline_tick) {
            TimingDomainsObserver_Begin(&timing_runtime.observer, id, 1,
                                        timing_runtime.weapon_arm_tick,
                                        timing_runtime.weapon_arm_wall_us);
        }
        TimingDomainsObserver_CheckTimeout(&timing_runtime.observer, id,
                                           timing_runtime.deadline_tick,
                                           timing_runtime.simulation_tick,
                                           timing_runtime.wall_time_us);
        if (timing_runtime.observer.domains[id].status == TIMING_STATUS_FAILED)
            timing_runtime_log_record("timing_domain_complete", id);
    }
}

int TimingDomainsRuntime_IsEnabled(void) { return timing_runtime.enabled; }
int TimingDomainsRuntime_IsFinished(void) { return timing_runtime.summary_written; }
int TimingDomainsRuntime_Passed(void) { return timing_runtime.summary_written && timing_runtime.observer.passed; }
int TimingDomainsRuntime_FrameLimit(void) { return timing_runtime.enabled ? timing_runtime.frame_limit : 0; }
int TimingDomainsRuntime_ShouldActivateWeapon(void)
{
    const TimingDomainRecord* record = &timing_runtime.observer.domains[TIMING_DOMAIN_WEAPON];
    if (!timing_runtime_current_domain(TIMING_DOMAIN_WEAPON) ||
        record->status != TIMING_STATUS_NOT_STARTED ||
        timing_runtime.simulation_tick < timing_runtime.weapon_next_activation_tick) {
        return 0;
    }
    timing_runtime.weapon_next_activation_tick =
        timing_runtime.simulation_tick + TIMING_WEAPON_RETRY_MS;
    return 1;
}
int TimingDomainsRuntime_ShouldStartAI(void) { return timing_runtime_should_dispatch(TIMING_DOMAIN_AI); }
int TimingDomainsRuntime_ShouldStartPhysics(void) { return timing_runtime_should_dispatch(TIMING_DOMAIN_PHYSICS); }
int TimingDomainsRuntime_ShouldStartAnimation(void) { return timing_runtime_should_dispatch(TIMING_DOMAIN_ANIMATION); }
int TimingDomainsRuntime_ShouldSpawnParticle(void) { return timing_runtime_should_dispatch(TIMING_DOMAIN_PARTICLE); }
int TimingDomainsRuntime_ShouldStartScript(void) { return timing_runtime_should_dispatch(TIMING_DOMAIN_SCRIPT); }
int TimingDomainsRuntime_ShouldStartDialogue(void) { return timing_runtime_should_dispatch(TIMING_DOMAIN_DIALOGUE); }
int TimingDomainsRuntime_ShouldStartCutscene(void) { return timing_runtime_should_dispatch(TIMING_DOMAIN_CUTSCENE); }
int TimingDomainsRuntime_ShouldRequestLevelTransition(void) { return timing_runtime_should_dispatch(TIMING_DOMAIN_LEVEL_TRANSITION); }

int TimingDomainsRuntime_RegisterTarget(TimingDomainId id, const void* primary, int secondary)
{
    TimingDomainRecord* record;
    if (!primary || !timing_runtime_current_domain(id)) return 0;
    record = &timing_runtime.observer.domains[id];
    if (record->status != TIMING_STATUS_RUNNING || timing_runtime.target_registered) return 0;
    timing_runtime.target_primary = primary;
    timing_runtime.target_secondary = secondary;
    timing_runtime.target_registered = 1;
    return 1;
}

int TimingDomainsRuntime_RegisterPhysicsTarget(const void* primary, float x, float y, float z)
{
    if (!TimingDomainsRuntime_RegisterTarget(TIMING_DOMAIN_PHYSICS, primary, 0)) return 0;
    timing_runtime.physics_start_x = x;
    timing_runtime.physics_start_y = y;
    timing_runtime.physics_start_z = z;
    return 1;
}

int TimingDomainsRuntime_RegisterScriptTarget(int task_id, int token)
{
    if (task_id != TIMING_SCRIPT_TASK_ID) return 0;
    return TimingDomainsRuntime_RegisterTarget(TIMING_DOMAIN_SCRIPT,
                                               (const void*)(uintptr_t)(unsigned int)token,
                                               task_id);
}

void TimingDomainsRuntime_NotifyWeaponFire(const void* shooter, double cooldown_deadline_seconds)
{
    TimingDomainRecord* record;
    if (!shooter || !timing_runtime_current_domain(TIMING_DOMAIN_WEAPON)) return;
    record = &timing_runtime.observer.domains[TIMING_DOMAIN_WEAPON];
    if (record->status != TIMING_STATUS_NOT_STARTED) return;
    if (cooldown_deadline_seconds <= 0.0) return;
    timing_runtime.weapon_cooldown_deadline_tick = (uint64_t)(cooldown_deadline_seconds * 1000.0 + 0.5);
    if (timing_runtime.weapon_cooldown_deadline_tick <= timing_runtime.simulation_tick) return;
    timing_runtime.target_primary = shooter;
    timing_runtime.target_registered = 1;
    timing_runtime.deadline_tick = timing_runtime.weapon_cooldown_deadline_tick + 1000;
    if (TimingDomainsObserver_Begin(&timing_runtime.observer, TIMING_DOMAIN_WEAPON, 1,
                                    timing_runtime.simulation_tick, timing_runtime.wall_time_us))
        timing_runtime_log_record("timing_domain_start", TIMING_DOMAIN_WEAPON);
}

void TimingDomainsRuntime_NotifyWeaponReady(const void* shooter)
{
    if (timing_runtime.target_registered && shooter == timing_runtime.target_primary &&
        timing_runtime.simulation_tick >= timing_runtime.weapon_cooldown_deadline_tick)
        timing_runtime_complete(TIMING_DOMAIN_WEAPON);
}

void TimingDomainsRuntime_NotifyAI(const void* thing)
{
    const TimingDomainRecord* record = &timing_runtime.observer.domains[TIMING_DOMAIN_AI];
    if (timing_runtime.target_registered && thing == timing_runtime.target_primary &&
        record->status == TIMING_STATUS_RUNNING &&
        timing_runtime.simulation_tick - record->start_tick >= TIMING_AI_OBSERVATION_MS)
        timing_runtime_complete(TIMING_DOMAIN_AI);
}

void TimingDomainsRuntime_NotifyPhysics(const void* thing, float x, float y, float z)
{
    float dx;
    float dy;
    float dz;
    float threshold = TIMING_PHYSICS_MIN_DISPLACEMENT;
    if (!timing_runtime.target_registered || thing != timing_runtime.target_primary) return;
    dx = x - timing_runtime.physics_start_x;
    dy = y - timing_runtime.physics_start_y;
    dz = z - timing_runtime.physics_start_z;
    if (dx * dx + dy * dy + dz * dz >= threshold * threshold)
        timing_runtime_complete(TIMING_DOMAIN_PHYSICS);
}

void TimingDomainsRuntime_NotifyAnimation(const void* puppet, int track)
{
    if (timing_runtime.target_registered && puppet == timing_runtime.target_primary &&
        track == timing_runtime.target_secondary)
        timing_runtime_complete(TIMING_DOMAIN_ANIMATION);
}

void TimingDomainsRuntime_NotifyParticle(const void* thing)
{
    if (timing_runtime.target_registered && thing == timing_runtime.target_primary)
        timing_runtime_complete(TIMING_DOMAIN_PARTICLE);
}

void TimingDomainsRuntime_NotifyScript(int task_id, int token)
{
    if (timing_runtime.target_registered && task_id == timing_runtime.target_secondary &&
        (const void*)(uintptr_t)(unsigned int)token == timing_runtime.target_primary)
        timing_runtime_complete(TIMING_DOMAIN_SCRIPT);
}

void TimingDomainsRuntime_NotifyDialogue(const void* channel)
{
    if (timing_runtime.target_registered && channel == timing_runtime.target_primary)
        timing_runtime_complete(TIMING_DOMAIN_DIALOGUE);
}

void TimingDomainsRuntime_NotifyCutscene(void)
{
    timing_runtime_complete(TIMING_DOMAIN_CUTSCENE);
}

void TimingDomainsRuntime_NotifyLevelTransition(const char* level_name, int load_succeeded)
{
    if (load_succeeded && level_name && strcmp(level_name, TIMING_LEVEL_TARGET) == 0)
        timing_runtime_complete(TIMING_DOMAIN_LEVEL_TRANSITION);
}
