#include "General/TimingDomainsScenario.h"

#include <assert.h>

static void mark_passed(TimingDomainsObserver* observer, TimingDomainId id)
{
    assert(TimingDomainsObserver_Begin(observer, id, 0, 1, 1));
    assert(TimingDomainsObserver_Complete(observer, id, 2, 2));
}

static void test_waits_for_world_and_dispatches_once(void)
{
    TimingDomainsObserver observer;
    TimingDomainsScenario scenario;
    TimingDomainsObserver_Init(&observer);
    TimingDomainsScenario_Init(&scenario);
    assert(TimingDomainsScenario_Update(&scenario, &observer, 0) == TIMING_ACTION_WAIT_READY);
    assert(TimingDomainsScenario_Update(&scenario, &observer, 1) == TIMING_ACTION_START_WEAPON);
    TimingDomainsScenario_MarkDispatched(&scenario);
    assert(TimingDomainsScenario_Update(&scenario, &observer, 1) == TIMING_ACTION_START_WEAPON);
    assert(scenario.action_dispatched);
}

static void test_advances_all_domains_in_order(void)
{
    static const TimingScenarioAction expected[] = {
        TIMING_ACTION_START_WEAPON, TIMING_ACTION_START_AI, TIMING_ACTION_START_PHYSICS,
        TIMING_ACTION_START_ANIMATION, TIMING_ACTION_START_PARTICLE, TIMING_ACTION_START_SCRIPT,
        TIMING_ACTION_START_DIALOGUE, TIMING_ACTION_START_CUTSCENE,
        TIMING_ACTION_REQUEST_LEVEL_TRANSITION
    };
    TimingDomainsObserver observer;
    TimingDomainsScenario scenario;
    int id;
    TimingDomainsObserver_Init(&observer);
    TimingDomainsScenario_Init(&scenario);
    for (id = 0; id < TIMING_DOMAIN_COUNT; ++id) {
        assert(TimingDomainsScenario_Update(&scenario, &observer, 1) == expected[id]);
        TimingDomainsScenario_MarkDispatched(&scenario);
        mark_passed(&observer, (TimingDomainId)id);
    }
    assert(TimingDomainsScenario_Update(&scenario, &observer, 1) == TIMING_ACTION_FINISH);
}

static void test_failure_aborts(void)
{
    TimingDomainsObserver observer;
    TimingDomainsScenario scenario;
    TimingDomainsObserver_Init(&observer);
    TimingDomainsScenario_Init(&scenario);
    assert(TimingDomainsScenario_Update(&scenario, &observer, 1) == TIMING_ACTION_START_WEAPON);
    TimingDomainsScenario_MarkDispatched(&scenario);
    assert(!TimingDomainsObserver_Complete(&observer, TIMING_DOMAIN_WEAPON, 3, 3));
    assert(TimingDomainsScenario_Update(&scenario, &observer, 1) == TIMING_ACTION_FAIL);
}

int main(void)
{
    test_waits_for_world_and_dispatches_once();
    test_advances_all_domains_in_order();
    test_failure_aborts();
    return 0;
}
