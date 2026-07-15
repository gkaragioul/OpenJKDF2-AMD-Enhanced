#ifndef OPENJKDF2_TIMING_DOMAINS_RUNTIME_H
#define OPENJKDF2_TIMING_DOMAINS_RUNTIME_H

#include <stdint.h>

#include "General/StartupOptions.h"
#include "General/TimingDomainsObserver.h"

int TimingDomainsRuntime_Configure(StartupValidationObserver selection, const char* diagnostics_dir, int frame_limit);
void TimingDomainsRuntime_Shutdown(void);
void TimingDomainsRuntime_RefreshClock(uint64_t simulation_tick, uint64_t wall_time_us);
void TimingDomainsRuntime_Tick(uint64_t simulation_tick, uint64_t wall_time_us);
int TimingDomainsRuntime_IsEnabled(void);
int TimingDomainsRuntime_IsFinished(void);
int TimingDomainsRuntime_Passed(void);
int TimingDomainsRuntime_FrameLimit(void);
int TimingDomainsRuntime_ShouldActivateWeapon(void);
int TimingDomainsRuntime_ShouldStartAI(void);
int TimingDomainsRuntime_ShouldStartPhysics(void);
int TimingDomainsRuntime_ShouldStartAnimation(void);
int TimingDomainsRuntime_ShouldSpawnParticle(void);
int TimingDomainsRuntime_ShouldStartScript(void);
int TimingDomainsRuntime_ShouldStartDialogue(void);
int TimingDomainsRuntime_ShouldStartCutscene(void);
int TimingDomainsRuntime_ShouldRequestLevelTransition(void);
int TimingDomainsRuntime_RegisterTarget(TimingDomainId id, const void* primary, int secondary);
int TimingDomainsRuntime_RegisterPhysicsTarget(const void* primary, float x, float y, float z);
int TimingDomainsRuntime_RegisterScriptTarget(int task_id, int token);
void TimingDomainsRuntime_NotifyWeaponFire(const void* shooter, double cooldown_deadline_seconds);
void TimingDomainsRuntime_NotifyWeaponReady(const void* shooter);
void TimingDomainsRuntime_NotifyAI(const void* thing);
void TimingDomainsRuntime_NotifyPhysics(const void* thing, float x, float y, float z);
void TimingDomainsRuntime_NotifyAnimation(const void* puppet, int track);
void TimingDomainsRuntime_NotifyParticle(const void* thing);
void TimingDomainsRuntime_NotifyScript(int task_id, int token);
void TimingDomainsRuntime_NotifyDialogue(const void* channel);
void TimingDomainsRuntime_NotifyCutscene(void);
void TimingDomainsRuntime_NotifyLevelTransition(const char* level_name, int load_succeeded);

#endif
