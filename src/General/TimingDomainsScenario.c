#include "General/TimingDomainsScenario.h"

static int timing_action_domain(TimingScenarioAction action, TimingDomainId* id)
{
    if (action < TIMING_ACTION_START_WEAPON || action > TIMING_ACTION_REQUEST_LEVEL_TRANSITION || !id) return 0;
    *id = (TimingDomainId)(action - TIMING_ACTION_START_WEAPON);
    return 1;
}

void TimingDomainsScenario_Init(TimingDomainsScenario* scenario)
{
    if (!scenario) return;
    scenario->action = TIMING_ACTION_WAIT_READY;
    scenario->action_dispatched = 0;
}

TimingScenarioAction TimingDomainsScenario_Update(TimingDomainsScenario* scenario, const TimingDomainsObserver* observer, int world_ready)
{
    TimingDomainId id;
    if (!scenario || !observer) return TIMING_ACTION_FAIL;
    if (scenario->action == TIMING_ACTION_FAIL || scenario->action == TIMING_ACTION_FINISH) return scenario->action;
    if (scenario->action == TIMING_ACTION_WAIT_READY) {
        if (!world_ready) return scenario->action;
        scenario->action = TIMING_ACTION_START_WEAPON;
        scenario->action_dispatched = 0;
        return scenario->action;
    }
    if (!timing_action_domain(scenario->action, &id)) {
        scenario->action = TIMING_ACTION_FAIL;
        return scenario->action;
    }
    if (observer->domains[id].status == TIMING_STATUS_FAILED) {
        scenario->action = TIMING_ACTION_FAIL;
        return scenario->action;
    }
    if (observer->domains[id].status == TIMING_STATUS_PASSED) {
        scenario->action = id == TIMING_DOMAIN_LEVEL_TRANSITION
            ? TIMING_ACTION_FINISH
            : (TimingScenarioAction)(scenario->action + 1);
        scenario->action_dispatched = 0;
    }
    return scenario->action;
}

void TimingDomainsScenario_MarkDispatched(TimingDomainsScenario* scenario)
{
    if (scenario && scenario->action >= TIMING_ACTION_START_WEAPON && scenario->action <= TIMING_ACTION_REQUEST_LEVEL_TRANSITION) {
        scenario->action_dispatched = 1;
    }
}
