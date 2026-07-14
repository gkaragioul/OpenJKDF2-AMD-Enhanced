#ifndef OPENJKDF2_TIMING_DOMAINS_RUNTIME_H
#define OPENJKDF2_TIMING_DOMAINS_RUNTIME_H

#include <stdint.h>

#include "General/StartupOptions.h"
#include "General/TimingDomainsObserver.h"

int TimingDomainsRuntime_Configure(StartupValidationObserver selection, const char* diagnostics_dir, int frame_limit);
void TimingDomainsRuntime_Shutdown(void);
void TimingDomainsRuntime_Tick(uint64_t simulation_tick, uint64_t wall_time_us);
int TimingDomainsRuntime_IsEnabled(void);
int TimingDomainsRuntime_IsFinished(void);
int TimingDomainsRuntime_Passed(void);
int TimingDomainsRuntime_FrameLimit(void);
int TimingDomainsRuntime_ShouldStartCutscene(void);
int TimingDomainsRuntime_ShouldRequestLevelTransition(void);
void TimingDomainsRuntime_NotifyWeapon(void);
void TimingDomainsRuntime_NotifyAI(void);
void TimingDomainsRuntime_NotifyPhysics(void);
void TimingDomainsRuntime_NotifyAnimation(void);
void TimingDomainsRuntime_NotifyParticle(void);
void TimingDomainsRuntime_NotifyScript(void);
void TimingDomainsRuntime_NotifyDialogue(void);
void TimingDomainsRuntime_NotifyCutscene(void);
void TimingDomainsRuntime_NotifyLevelTransition(void);

#endif
