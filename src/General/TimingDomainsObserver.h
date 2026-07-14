#ifndef OPENJKDF2_TIMING_DOMAINS_OBSERVER_H
#define OPENJKDF2_TIMING_DOMAINS_OBSERVER_H

#include <stdint.h>

typedef enum TimingDomainId
{
    TIMING_DOMAIN_WEAPON = 0,
    TIMING_DOMAIN_AI,
    TIMING_DOMAIN_PHYSICS,
    TIMING_DOMAIN_ANIMATION,
    TIMING_DOMAIN_PARTICLE,
    TIMING_DOMAIN_SCRIPT,
    TIMING_DOMAIN_DIALOGUE,
    TIMING_DOMAIN_CUTSCENE,
    TIMING_DOMAIN_LEVEL_TRANSITION,
    TIMING_DOMAIN_COUNT
} TimingDomainId;

typedef enum TimingDomainStatus
{
    TIMING_STATUS_NOT_STARTED = 0,
    TIMING_STATUS_RUNNING,
    TIMING_STATUS_PASSED,
    TIMING_STATUS_FAILED
} TimingDomainStatus;

typedef enum TimingDomainReason
{
    TIMING_REASON_NONE = 0,
    TIMING_REASON_DUPLICATE,
    TIMING_REASON_TIMEOUT,
    TIMING_REASON_BAD_STATE,
    TIMING_REASON_MISSING
} TimingDomainReason;

typedef struct TimingDomainRecord
{
    TimingDomainStatus status;
    TimingDomainReason reason;
    uint64_t start_tick;
    uint64_t end_tick;
    uint64_t start_wall_us;
    uint64_t end_wall_us;
    uint32_t expected_events;
    uint32_t observed_events;
} TimingDomainRecord;

typedef struct TimingDomainsObserver
{
    TimingDomainRecord domains[TIMING_DOMAIN_COUNT];
    int finalized;
    int passed;
} TimingDomainsObserver;

void TimingDomainsObserver_Init(TimingDomainsObserver* observer);
int TimingDomainsObserver_Begin(TimingDomainsObserver* observer, TimingDomainId id, uint32_t expected_events, uint64_t tick, uint64_t wall_us);
int TimingDomainsObserver_Notify(TimingDomainsObserver* observer, TimingDomainId id, uint64_t tick, uint64_t wall_us);
int TimingDomainsObserver_Complete(TimingDomainsObserver* observer, TimingDomainId id, uint64_t tick, uint64_t wall_us);
void TimingDomainsObserver_CheckTimeout(TimingDomainsObserver* observer, TimingDomainId id, uint64_t deadline_tick, uint64_t tick, uint64_t wall_us);
int TimingDomainsObserver_Finalize(TimingDomainsObserver* observer);
const char* TimingDomainsObserver_DomainName(TimingDomainId id);
const char* TimingDomainsObserver_ReasonName(TimingDomainReason reason);

#endif
