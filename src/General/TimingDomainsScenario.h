#ifndef OPENJKDF2_TIMING_DOMAINS_SCENARIO_H
#define OPENJKDF2_TIMING_DOMAINS_SCENARIO_H

#include "General/TimingDomainsObserver.h"

typedef enum TimingScenarioAction
{
    TIMING_ACTION_WAIT_READY = 0,
    TIMING_ACTION_START_WEAPON,
    TIMING_ACTION_START_AI,
    TIMING_ACTION_START_PHYSICS,
    TIMING_ACTION_START_ANIMATION,
    TIMING_ACTION_START_PARTICLE,
    TIMING_ACTION_START_SCRIPT,
    TIMING_ACTION_START_DIALOGUE,
    TIMING_ACTION_START_CUTSCENE,
    TIMING_ACTION_REQUEST_LEVEL_TRANSITION,
    TIMING_ACTION_FINISH,
    TIMING_ACTION_FAIL
} TimingScenarioAction;

typedef struct TimingDomainsScenario
{
    TimingScenarioAction action;
    int action_dispatched;
} TimingDomainsScenario;

void TimingDomainsScenario_Init(TimingDomainsScenario* scenario);
TimingScenarioAction TimingDomainsScenario_Update(TimingDomainsScenario* scenario, const TimingDomainsObserver* observer, int world_ready);
void TimingDomainsScenario_MarkDispatched(TimingDomainsScenario* scenario);

#endif
