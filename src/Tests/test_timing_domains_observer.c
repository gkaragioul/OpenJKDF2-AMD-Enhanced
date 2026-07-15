#include "General/TimingDomainsObserver.h"

#include <assert.h>
#include <string.h>

static void pass_domain(TimingDomainsObserver* observer, TimingDomainId id, uint64_t tick)
{
    assert(TimingDomainsObserver_Begin(observer, id, 1, tick, tick * 1000));
    assert(TimingDomainsObserver_Notify(observer, id, tick + 1, (tick + 1) * 1000));
    assert(TimingDomainsObserver_Complete(observer, id, tick + 2, (tick + 2) * 1000));
}

static void test_ordered_success(void)
{
    TimingDomainsObserver observer;
    int id;
    TimingDomainsObserver_Init(&observer);
    for (id = 0; id < TIMING_DOMAIN_COUNT; ++id) pass_domain(&observer, (TimingDomainId)id, (uint64_t)id * 10);
    assert(TimingDomainsObserver_Finalize(&observer));
    assert(observer.finalized && observer.passed);
    assert(TimingDomainsObserver_Finalize(&observer));
    assert(strcmp(TimingDomainsObserver_DomainName(TIMING_DOMAIN_DIALOGUE), "dialogue") == 0);
}

static void test_event_count_and_duplicates(void)
{
    TimingDomainsObserver observer;
    TimingDomainsObserver_Init(&observer);
    assert(TimingDomainsObserver_Begin(&observer, TIMING_DOMAIN_AI, 2, 10, 100));
    assert(!TimingDomainsObserver_Begin(&observer, TIMING_DOMAIN_AI, 2, 10, 100));
    assert(observer.domains[TIMING_DOMAIN_AI].reason == TIMING_REASON_DUPLICATE);

    TimingDomainsObserver_Init(&observer);
    assert(TimingDomainsObserver_Begin(&observer, TIMING_DOMAIN_AI, 1, 10, 100));
    assert(TimingDomainsObserver_Notify(&observer, TIMING_DOMAIN_AI, 11, 110));
    assert(!TimingDomainsObserver_Notify(&observer, TIMING_DOMAIN_AI, 12, 120));
    assert(observer.domains[TIMING_DOMAIN_AI].reason == TIMING_REASON_DUPLICATE);
}

static void test_bad_state_and_timeout(void)
{
    TimingDomainsObserver observer;
    TimingDomainsObserver_Init(&observer);
    assert(!TimingDomainsObserver_Complete(&observer, TIMING_DOMAIN_PHYSICS, 2, 20));
    assert(observer.domains[TIMING_DOMAIN_PHYSICS].reason == TIMING_REASON_BAD_STATE);

    TimingDomainsObserver_Init(&observer);
    assert(TimingDomainsObserver_Begin(&observer, TIMING_DOMAIN_PHYSICS, 0, 2, 20));
    TimingDomainsObserver_CheckTimeout(&observer, TIMING_DOMAIN_PHYSICS, 10, 10, 100);
    assert(observer.domains[TIMING_DOMAIN_PHYSICS].status == TIMING_STATUS_RUNNING);
    TimingDomainsObserver_CheckTimeout(&observer, TIMING_DOMAIN_PHYSICS, 10, 11, 110);
    assert(observer.domains[TIMING_DOMAIN_PHYSICS].status == TIMING_STATUS_FAILED);
    assert(observer.domains[TIMING_DOMAIN_PHYSICS].reason == TIMING_REASON_TIMEOUT);
}

static void test_finalize_marks_missing_domains(void)
{
    TimingDomainsObserver observer;
    TimingDomainsObserver_Init(&observer);
    pass_domain(&observer, TIMING_DOMAIN_WEAPON, 1);
    assert(!TimingDomainsObserver_Finalize(&observer));
    assert(observer.domains[TIMING_DOMAIN_AI].status == TIMING_STATUS_FAILED);
    assert(observer.domains[TIMING_DOMAIN_AI].reason == TIMING_REASON_MISSING);
    assert(!TimingDomainsObserver_Finalize(&observer));
    assert(strcmp(TimingDomainsObserver_ReasonName(TIMING_REASON_MISSING), "missing") == 0);
}

static void test_monotonic_clock_carries_epoch_across_level_reset(void)
{
    TimingDomainsClock clock;
    TimingDomainsClock_Init(&clock);
    assert(TimingDomainsClock_Update(&clock, 100) == 100);
    assert(TimingDomainsClock_Update(&clock, 120) == 120);
    assert(TimingDomainsClock_Update(&clock, 0) == 120);
    assert(TimingDomainsClock_Update(&clock, 5) == 125);
    assert(TimingDomainsClock_Update(&clock, 0) == 125);
    assert(TimingDomainsClock_Update(&clock, 7) == 132);
}

int main(void)
{
    test_ordered_success();
    test_event_count_and_duplicates();
    test_bad_state_and_timeout();
    test_finalize_marks_missing_domains();
    test_monotonic_clock_carries_epoch_across_level_reset();
    return 0;
}
