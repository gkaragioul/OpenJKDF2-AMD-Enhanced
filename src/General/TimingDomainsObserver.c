#include "General/TimingDomainsObserver.h"

#include <string.h>

static int timing_domain_valid(TimingDomainId id)
{
    return id >= TIMING_DOMAIN_WEAPON && id < TIMING_DOMAIN_COUNT;
}

static int timing_domain_fail(TimingDomainRecord* record, TimingDomainReason reason, uint64_t tick, uint64_t wall_us)
{
    if (record) {
        record->status = TIMING_STATUS_FAILED;
        record->reason = reason;
        record->end_tick = tick;
        record->end_wall_us = wall_us;
    }
    return 0;
}

void TimingDomainsObserver_Init(TimingDomainsObserver* observer)
{
    if (observer) memset(observer, 0, sizeof(*observer));
}

int TimingDomainsObserver_Begin(TimingDomainsObserver* observer, TimingDomainId id, uint32_t expected_events, uint64_t tick, uint64_t wall_us)
{
    TimingDomainRecord* record;
    if (!observer || observer->finalized || !timing_domain_valid(id)) return 0;
    record = &observer->domains[id];
    if (record->status != TIMING_STATUS_NOT_STARTED) return timing_domain_fail(record, TIMING_REASON_DUPLICATE, tick, wall_us);
    record->status = TIMING_STATUS_RUNNING;
    record->start_tick = tick;
    record->start_wall_us = wall_us;
    record->expected_events = expected_events;
    return 1;
}

int TimingDomainsObserver_Notify(TimingDomainsObserver* observer, TimingDomainId id, uint64_t tick, uint64_t wall_us)
{
    TimingDomainRecord* record;
    if (!observer || observer->finalized || !timing_domain_valid(id)) return 0;
    record = &observer->domains[id];
    if (record->status != TIMING_STATUS_RUNNING) return timing_domain_fail(record, TIMING_REASON_BAD_STATE, tick, wall_us);
    ++record->observed_events;
    if (record->observed_events > record->expected_events) return timing_domain_fail(record, TIMING_REASON_DUPLICATE, tick, wall_us);
    return 1;
}

int TimingDomainsObserver_Complete(TimingDomainsObserver* observer, TimingDomainId id, uint64_t tick, uint64_t wall_us)
{
    TimingDomainRecord* record;
    if (!observer || observer->finalized || !timing_domain_valid(id)) return 0;
    record = &observer->domains[id];
    if (record->status != TIMING_STATUS_RUNNING) return timing_domain_fail(record, TIMING_REASON_BAD_STATE, tick, wall_us);
    if (record->observed_events != record->expected_events) return timing_domain_fail(record, TIMING_REASON_BAD_STATE, tick, wall_us);
    record->status = TIMING_STATUS_PASSED;
    record->end_tick = tick;
    record->end_wall_us = wall_us;
    return 1;
}

void TimingDomainsObserver_CheckTimeout(TimingDomainsObserver* observer, TimingDomainId id, uint64_t deadline_tick, uint64_t tick, uint64_t wall_us)
{
    if (!observer || observer->finalized || !timing_domain_valid(id)) return;
    if (observer->domains[id].status == TIMING_STATUS_RUNNING && tick > deadline_tick) {
        timing_domain_fail(&observer->domains[id], TIMING_REASON_TIMEOUT, tick, wall_us);
    }
}

int TimingDomainsObserver_Finalize(TimingDomainsObserver* observer)
{
    int id;
    if (!observer) return 0;
    if (observer->finalized) return observer->passed;
    observer->passed = 1;
    for (id = 0; id < TIMING_DOMAIN_COUNT; ++id) {
        TimingDomainRecord* record = &observer->domains[id];
        if (record->status != TIMING_STATUS_PASSED) {
            if (record->status != TIMING_STATUS_FAILED) timing_domain_fail(record, TIMING_REASON_MISSING, record->end_tick, record->end_wall_us);
            observer->passed = 0;
        }
    }
    observer->finalized = 1;
    return observer->passed;
}

const char* TimingDomainsObserver_DomainName(TimingDomainId id)
{
    static const char* names[TIMING_DOMAIN_COUNT] = {
        "weapon", "ai", "physics", "animation", "particle", "script", "dialogue", "cutscene", "level_transition"
    };
    return timing_domain_valid(id) ? names[id] : "invalid";
}

const char* TimingDomainsObserver_ReasonName(TimingDomainReason reason)
{
    static const char* names[] = { "none", "duplicate", "timeout", "bad_state", "missing" };
    return reason >= TIMING_REASON_NONE && reason <= TIMING_REASON_MISSING ? names[reason] : "invalid";
}
