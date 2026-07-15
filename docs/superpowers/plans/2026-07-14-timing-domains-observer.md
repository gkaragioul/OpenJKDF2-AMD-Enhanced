# Timing Domains Observer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an opt-in automated validation observer that proves weapon, AI, physics, animation, particle, script, dialogue, cutscene, and level-transition behavior remains time-correct at 60 and 120 FPS.

**Architecture:** A startup-only flag enables a pure-C observer and deterministic scenario state machine. A narrow runtime bridge starts normal engine actions and receives passive lifecycle notifications from production subsystems; a PowerShell harness runs isolated 60/120 sessions, compares structured results, and writes evidence without altering normal gameplay.

**Tech Stack:** C11, OpenJKDF2 engine APIs, CMake/CTest, Windows PowerShell 5.1, JSON Lines diagnostics, Git.

## Global Constraints

- Activation is explicit: `--validation-observer=timing-domains`; normal launches have zero scenario behavior.
- Read legitimate assets from `D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight`; never modify that directory.
- Runtime output and temporary users live outside the Steam directory.
- Never launch or test exclusive fullscreen in this work.
- The observer may measure and report production state but must never force a subsystem to complete successfully.
- The scenario driver may invoke ordinary production entry points only after the previous domain passes.
- Compare 60 and 120 FPS using simulation time as the correctness authority and wall time as supporting evidence.
- Preserve display resolution, refresh rate, gamma, HDR, color profile, scaling, and topology.
- A failed, missing, duplicated, or timed-out domain fails the run; partial evidence cannot promote a requirement.
- Required final builds are exactly `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Debug -Test` and the corresponding `-Configuration Release -Test` command.

---

## File Map

- `src/General/StartupOptions.h/.c`: parse and validate the observer selector.
- `src/General/TimingDomainsObserver.h/.c`: engine-independent domain records and terminal result logic.
- `src/General/TimingDomainsScenario.h/.c`: ordered validation action state machine.
- `src/General/TimingDomainsRuntime.h/.c`: sole engine-facing bridge, JSONL writer, and production notifications.
- `src/main.c`: configure the bridge after startup paths are resolved.
- `src/Main/jkGame.c`: advance the validation scenario once per simulation frame.
- Production subsystem files listed in Task 4: emit passive lifecycle notifications only.
- `src/Tests/test_*.c` and `cmake/OpenJKDF2Tests.cmake`: unit and source-contract tests.
- `scripts/test-timing-domains.ps1`: isolated 60/120 runtime harness and comparison.
- `docs/evidence/timing-domains-2026-07-14.*`: raw machine-readable proof and human summary.
- `docs/evidence/requirements.csv`: promote only requirements demonstrated by passing evidence.

### Task 1: Startup Activation Contract

**Files:**
- Modify: `src/General/StartupOptions.h`
- Modify: `src/General/StartupOptions.c`
- Modify: `src/Tests/test_startup_options.c`

**Interfaces:**
- Consumes: existing `int StartupOptions_Parse(int argc, char **argv, StartupOptions *options)`.
- Produces: `StartupValidationObserver`, `StartupOptions.validation_observer`, and parsing of both `--validation-observer=timing-domains` and `--validation-observer timing-domains`.

- [ ] **Step 1: Write failing parser tests**

Add cases that assert both accepted spellings set `STARTUP_VALIDATION_TIMING_DOMAINS`, absence yields `STARTUP_VALIDATION_NONE`, a missing value fails, and `--validation-observer=unknown` fails with `options.error` containing `validation-observer`:

```c
static void test_timing_observer_equals_form(void)
{
    char *argv[] = {"openjkdf2", "--validation-observer=timing-domains"};
    StartupOptions options;
    TEST_ASSERT_TRUE(StartupOptions_Parse(2, argv, &options));
    TEST_ASSERT_EQUAL_INT(STARTUP_VALIDATION_TIMING_DOMAINS, options.validation_observer);
}
```

- [ ] **Step 2: Verify the tests fail**

Run: `cmake --build build-debug --target test_startup_options && build-debug\src\Tests\test_startup_options.exe`

Expected: compile failure because `STARTUP_VALIDATION_TIMING_DOMAINS` and `validation_observer` do not exist.

- [ ] **Step 3: Add the startup enum and field**

```c
typedef enum StartupValidationObserver {
    STARTUP_VALIDATION_NONE = 0,
    STARTUP_VALIDATION_TIMING_DOMAINS
} StartupValidationObserver;

typedef struct StartupOptions {
    /* existing fields remain unchanged */
    StartupValidationObserver validation_observer;
} StartupOptions;
```

Initialize the field to `STARTUP_VALIDATION_NONE`; accept only the exact value `timing-domains`, consume a separate value exactly once, and set a stable error message for absent or unknown values.

- [ ] **Step 4: Verify parser behavior**

Run: `cmake --build build-debug --target test_startup_options && build-debug\src\Tests\test_startup_options.exe`

Expected: all startup-option tests pass.

- [ ] **Step 5: Commit the activation contract**

```powershell
git add src/General/StartupOptions.h src/General/StartupOptions.c src/Tests/test_startup_options.c
git commit -m "test: define timing observer startup contract"
```

### Task 2: Pure Timing-Domain Observer

**Files:**
- Create: `src/General/TimingDomainsObserver.h`
- Create: `src/General/TimingDomainsObserver.c`
- Create: `src/Tests/test_timing_domains_observer.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Consumes: caller-provided monotonically increasing `uint64_t simulation_tick` and `uint64_t wall_time_us`.
- Produces:

```c
typedef enum TimingDomainId {
    TIMING_DOMAIN_WEAPON = 0, TIMING_DOMAIN_AI, TIMING_DOMAIN_PHYSICS,
    TIMING_DOMAIN_ANIMATION, TIMING_DOMAIN_PARTICLE, TIMING_DOMAIN_SCRIPT,
    TIMING_DOMAIN_DIALOGUE, TIMING_DOMAIN_CUTSCENE, TIMING_DOMAIN_LEVEL_TRANSITION,
    TIMING_DOMAIN_COUNT
} TimingDomainId;
typedef enum TimingDomainStatus { TIMING_STATUS_NOT_STARTED, TIMING_STATUS_RUNNING, TIMING_STATUS_PASSED, TIMING_STATUS_FAILED } TimingDomainStatus;
typedef enum TimingDomainReason { TIMING_REASON_NONE, TIMING_REASON_DUPLICATE, TIMING_REASON_TIMEOUT, TIMING_REASON_BAD_STATE, TIMING_REASON_MISSING } TimingDomainReason;
typedef struct TimingDomainRecord {
    TimingDomainStatus status; TimingDomainReason reason;
    uint64_t start_tick, end_tick, start_wall_us, end_wall_us;
    uint32_t expected_events, observed_events;
} TimingDomainRecord;
typedef struct TimingDomainsObserver {
    TimingDomainRecord domains[TIMING_DOMAIN_COUNT];
    int finalized; int passed;
} TimingDomainsObserver;
void TimingDomainsObserver_Init(TimingDomainsObserver *observer);
int TimingDomainsObserver_Begin(TimingDomainsObserver *observer, TimingDomainId id, uint32_t expected_events, uint64_t tick, uint64_t wall_us);
int TimingDomainsObserver_Notify(TimingDomainsObserver *observer, TimingDomainId id, uint64_t tick, uint64_t wall_us);
int TimingDomainsObserver_Complete(TimingDomainsObserver *observer, TimingDomainId id, uint64_t tick, uint64_t wall_us);
void TimingDomainsObserver_CheckTimeout(TimingDomainsObserver *observer, TimingDomainId id, uint64_t deadline_tick, uint64_t tick, uint64_t wall_us);
int TimingDomainsObserver_Finalize(TimingDomainsObserver *observer);
const char *TimingDomainsObserver_DomainName(TimingDomainId id);
const char *TimingDomainsObserver_ReasonName(TimingDomainReason reason);
```

- [ ] **Step 1: Write observer lifecycle tests**

Test ordered begin/notify/complete, exact event counting, duplicate begin/complete failure, timeout at `tick > deadline_tick`, finalization failure for any missing domain, and idempotent read-only finalization. Populate all nine domains in the success fixture.

- [ ] **Step 2: Register and run the failing observer target**

Add:

```cmake
openjkdf2_add_unit_test(test_timing_domains_observer
    ../../General/TimingDomainsObserver.c
    test_timing_domains_observer.c)
```

Run: `cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build-debug --target test_timing_domains_observer`

Expected: failure because the observer files do not exist.

- [ ] **Step 3: Implement the minimal observer**

Use bounds checks on every `TimingDomainId`. `Notify` increments only a running record and fails on more than `expected_events`. `Complete` passes only when observed equals expected. `Finalize` marks unstarted/running records failed with `TIMING_REASON_MISSING` and returns true only when all nine passed; a second call returns the stored result without mutation.

- [ ] **Step 4: Run observer tests**

Run: `cmake --build build-debug --target test_timing_domains_observer && build-debug\src\Tests\test_timing_domains_observer.exe`

Expected: all observer tests pass.

- [ ] **Step 5: Commit the pure observer**

```powershell
git add src/General/TimingDomainsObserver.* src/Tests/test_timing_domains_observer.c cmake/OpenJKDF2Tests.cmake
git commit -m "feat: add pure timing-domain observer"
```

### Task 3: Pure Ordered Scenario Driver

**Files:**
- Create: `src/General/TimingDomainsScenario.h`
- Create: `src/General/TimingDomainsScenario.c`
- Create: `src/Tests/test_timing_domains_scenario.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Consumes: `const TimingDomainsObserver *observer` from Task 2.
- Produces:

```c
typedef enum TimingScenarioAction {
    TIMING_ACTION_WAIT_READY = 0, TIMING_ACTION_START_WEAPON,
    TIMING_ACTION_START_AI, TIMING_ACTION_START_PHYSICS,
    TIMING_ACTION_START_ANIMATION, TIMING_ACTION_START_PARTICLE,
    TIMING_ACTION_START_SCRIPT, TIMING_ACTION_START_DIALOGUE,
    TIMING_ACTION_START_CUTSCENE, TIMING_ACTION_REQUEST_LEVEL_TRANSITION,
    TIMING_ACTION_FINISH, TIMING_ACTION_FAIL
} TimingScenarioAction;
typedef struct TimingDomainsScenario { TimingScenarioAction action; int action_dispatched; } TimingDomainsScenario;
void TimingDomainsScenario_Init(TimingDomainsScenario *scenario);
TimingScenarioAction TimingDomainsScenario_Update(TimingDomainsScenario *scenario, const TimingDomainsObserver *observer, int world_ready);
void TimingDomainsScenario_MarkDispatched(TimingDomainsScenario *scenario);
```

- [ ] **Step 1: Write failing state-machine tests**

Assert `WAIT_READY` remains until `world_ready`, each start action is returned once until marked dispatched, only a passed current domain advances, any failed domain returns `FAIL`, and nine passes return `FINISH`.

- [ ] **Step 2: Register and verify failure**

Register `test_timing_domains_scenario` with both pure `.c` files, then run `cmake --build build-debug --target test_timing_domains_scenario`.

Expected: failure because scenario symbols do not exist.

- [ ] **Step 3: Implement the table-driven state machine**

Use a static mapping from each start action to its domain. Never mutate observer records. Clear `action_dispatched` only when advancing to the next action; a dispatched running action remains selected but is not eligible for a second dispatch.

- [ ] **Step 4: Run scenario tests**

Run: `cmake --build build-debug --target test_timing_domains_scenario && build-debug\src\Tests\test_timing_domains_scenario.exe`

Expected: all scenario tests pass.

- [ ] **Step 5: Commit the driver**

```powershell
git add src/General/TimingDomainsScenario.* src/Tests/test_timing_domains_scenario.c cmake/OpenJKDF2Tests.cmake
git commit -m "feat: add timing validation scenario driver"
```

### Task 4: Runtime Bridge and Passive Production Hooks

**Files:**
- Create: `src/General/TimingDomainsRuntime.h`
- Create: `src/General/TimingDomainsRuntime.c`
- Modify: `src/main.c`
- Modify: `src/Main/jkGame.c`
- Modify: `src/World/sithWeapon.c`
- Modify: `src/AI/sithAI.c`
- Modify: `src/Engine/sithPhysics.c`
- Modify: `src/Engine/rdPuppet.c`
- Modify: `src/World/sithThing.c`
- Modify: `src/Gameplay/sithEvent.c`
- Modify: `src/Devices/sithSoundMixer.c`
- Modify: `src/Main/jkCutscene.c`
- Modify: `src/Main/jkMain.c`
- Create: `scripts/check-timing-domains-runtime.ps1`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Consumes: Tasks 1-3, resolved diagnostics directory, production subsystem state, and existing normal action APIs.
- Produces:

```c
int TimingDomainsRuntime_Configure(StartupValidationObserver selection, const char *diagnostics_dir, int frame_limit);
void TimingDomainsRuntime_Shutdown(void);
void TimingDomainsRuntime_Tick(uint64_t simulation_tick, uint64_t wall_time_us);
int TimingDomainsRuntime_IsEnabled(void);
void TimingDomainsRuntime_Notify(TimingDomainId id, uint64_t simulation_tick, uint64_t wall_time_us);
void TimingDomainsRuntime_Complete(TimingDomainId id, uint64_t simulation_tick, uint64_t wall_time_us);
```

- [ ] **Step 1: Write the failing source-contract check**

The script must fail unless it finds: runtime configuration in `src/main.c`; one tick call in `src/Main/jkGame.c`; notify/complete calls in every listed subsystem; the literal flag name; JSON event types `timing_domain_start`, `timing_domain_complete`, and `timing_domains_summary`; and no assignments through observer-owned production pointers.

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\check-timing-domains-runtime.ps1`

Expected: non-zero exit listing all missing integration sites.

- [ ] **Step 2: Implement bridge configuration and JSONL output**

`Configure` must reject enabled runs unless `diagnostics_dir` is non-empty and `frame_limit` is exactly 60 or 120. Open `timing-domains.jsonl` under the diagnostics directory. Each line contains `schema_version:1`, `event`, `domain`, `simulation_tick`, `wall_time_us`, `expected_events`, `observed_events`, `status`, and `reason`. `Shutdown` finalizes once, writes the summary, flushes, and closes.

- [ ] **Step 3: Connect startup and central ticking**

After `StartupOptions_Parse` and path resolution in `src/main.c`, call `TimingDomainsRuntime_Configure`. On configuration failure, log and return non-zero before `Window_Main_Linux`. Call `TimingDomainsRuntime_Shutdown` on every normal engine return path. In `jkGame.c`, call `TimingDomainsRuntime_Tick` once per simulation update, never from the render-only path.

- [ ] **Step 4: Dispatch ordinary scenario actions**

In the bridge, dispatch one action once using existing production entry points and
validation-owned engine objects. Begin each observer record before dispatch,
except weapon timing, which arms a finite timeout and starts measurement only
when the passive real-projectile hook confirms a shot. Do not call observer
completion from the dispatcher.

- [ ] **Step 5: Add passive subsystem notifications**

Add enabled-guarded calls only:

- `sithWeapon.c`: commit held primary state before synchronous COG activation,
  observe real projectile spawn/cooldown, release normally after the spawn, and
  complete only at the natural weapon-ready transition.
- `sithAI.c:sithAI_Tick`: expected AI decision/tick count.
- `sithPhysics.c:sithPhysics_UpdateThing`: validation object crossing its target threshold.
- `rdPuppet.c:rdPuppet_RemoveTrack`: natural animation track removal.
- `sithThing.c`: natural validation particle expiration/removal.
- `sithEvent.c:sithEvent_Process`: validation event dispatch.
- `sithSoundMixer.c:sithSoundMixer_StopSound`: natural dialogue channel completion.
- `jkCutscene.c`: natural cutscene end state.
- `jkMain.c:jkMain_LoadLevelSingleplayer`: requested next-level load success.

Every hook identifies validation-owned objects/tokens before notifying; unrelated gameplay cannot satisfy a domain.

- [ ] **Step 6: Register and pass the contract test**

Register the PowerShell script with `add_test(NAME timing_domains_runtime_contract ...)` on Windows.

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\check-timing-domains-runtime.ps1`

Expected: `PASS: timing-domain runtime bridge is connected to all nine production domains.`

- [ ] **Step 7: Build and run all unit tests**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Debug -Test`

Expected: build succeeds and all CTest tests pass, including the three new timing tests.

- [ ] **Step 8: Commit runtime integration**

```powershell
git add src scripts/check-timing-domains-runtime.ps1 cmake/OpenJKDF2Tests.cmake
git commit -m "feat: connect timing observer to production domains"
```

### Task 5: Two-Rate Runtime Harness

**Files:**
- Create: `scripts/test-timing-domains.ps1`
- Create: `scripts/test-timing-domains-fixtures.ps1`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Consumes: Release executable, `--validation-observer=timing-domains`, `--frame-limit`, `--data-dir`, `--user-dir`, diagnostics JSONL, Windows display state, and Application Error event log.
- Produces: `timing-domains-comparison.json` plus process exit success only if both runs and every comparison pass.

- [ ] **Step 1: Create failing parser/comparison fixtures**

Fixtures cover nine complete domains, missing/duplicate/failed records, malformed
JSON, non-zero child exit, and threshold failures. They accept seven boundary-valid
datasets and reject eleven invalid datasets. Source contracts separately cover
child and batch safety paths. Require exact event counts and terminal states;
within-cap non-media ranges of 18 ms at 60 FPS and 10 ms at 120 FPS with an
18 ms asynchronous-media floor; ordinary wall medians within the tighter of
50 ms or 5%; dialogue/cutscene within the tighter of 100 ms or 5%; level-load
wall medians within the larger of 100 ms or 20%; and unchanged safety invariants.

- [ ] **Step 2: Run fixtures before implementation**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-timing-domains-fixtures.ps1`

Expected: failure because `scripts/test-timing-domains.ps1` is absent.

- [ ] **Step 3: Implement strict JSONL parsing and comparison**

Define parameters `-Executable`, `-DataDir`, `-OutputDir`, and `-FixtureOnly`. Reject unknown schema versions, duplicate summaries, absent domains, non-passed statuses, and non-zero exits. Emit comparison JSON with both raw summaries, per-domain deltas, thresholds, display before/after, event-log findings, and final `passed`.

- [ ] **Step 4: Implement isolated child launches**

Create a fresh user and diagnostics directory per process. Run one validated
warm-up, then three measured samples per cap in the reversible interleaved order
60/120/120/60/60/120. Enforce non-media within-cap simulation ranges of 18 ms
at 60 FPS and 10 ms at 120 FPS, with an 18 ms floor for asynchronous media.
Compare simulation medians within 17 ms and enforce the domain-specific wall
policies defined in Step 1. Bound each process; on timeout terminate it and
record the failure, but defer throwing until display, Application Error, and
Steam metadata safety checks are captured. Never pass an Exclusive option,
focus the game, or inject desktop input.

- [ ] **Step 5: Pass fixture tests and register them**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-timing-domains-fixtures.ps1`

Expected: all positive and negative fixtures pass. Register fixture mode as `timing_domains_harness_fixtures` in CTest.

- [ ] **Step 6: Commit the harness**

```powershell
git add scripts/test-timing-domains.ps1 scripts/test-timing-domains-fixtures.ps1 cmake/OpenJKDF2Tests.cmake
git commit -m "test: compare gameplay timing at 60 and 120 fps"
```

### Task 6: Runtime Proof, Documentation, and Ledger Promotion

**Files:**
- Create: `docs/evidence/timing-domains-2026-07-14.json`
- Create: `docs/evidence/timing-domains-2026-07-14.md`
- Modify: `docs/evidence/requirements.csv`

**Interfaces:**
- Consumes: exact Debug/Release test results and `scripts/test-timing-domains.ps1` comparison output.
- Produces: auditable evidence for `M5-TIMING`, `M9-GAMEPLAY`, and `AC-120FPS`; no other ledger status changes.

- [ ] **Step 1: Run the exact Debug gate**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Debug -Test`

Expected: Debug build succeeds and every test passes.

- [ ] **Step 2: Run the exact Release gate**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Release -Test`

Expected: Release build succeeds and every test passes.

- [ ] **Step 3: Capture the 60/120 runtime evidence**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-timing-domains.ps1 `
  -Executable build\msvc-release\openjkdf2-64.exe `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -OutputDir docs\evidence\timing-domains-raw-2026-07-14
```

Expected: exit 0; the warm-up and all six measured processes complete all nine
domains; within-cap ranges and median comparisons pass; no Application Error
event is recorded; every display snapshot matches.

- [ ] **Step 4: Review raw evidence before promotion**

Open all seven JSONL files and the comparison JSON. Confirm domain names are
unique and complete, process exits use the engine's documented success code, the
backend is hardware rendering, no `atio6axx.dll` failure is present, and Steam
directory metadata remains unchanged.

- [ ] **Step 5: Write evidence artifacts**

Curate the comparison into `docs/evidence/timing-domains-2026-07-14.json`, retaining
every scored simulation and wall-time sample. The Markdown report records commit,
executable hash, GPU/driver, Windows build, commands, the nine-domain 60/120 table,
thresholds, display snapshots, event-log result, Steam invariant, and limitations.
Raw profiles remain ignored because they contain proprietary autosaves.

- [ ] **Step 6: Promote only proven ledger rows**

Change `M5-TIMING`, `M9-GAMEPLAY`, and `AC-120FPS` from `incomplete` to `proven` only when Steps 1-5 pass. Cite the Markdown and JSON evidence paths. Keep multiplayer and unavailable GPU generations explicitly unverified.

- [ ] **Step 7: Verify the clean tree and final diff**

Run: `git diff --check; git status --short; git diff -- docs/evidence/requirements.csv docs/evidence/timing-domains-2026-07-14.md`

Expected: no whitespace errors; only intended evidence/ledger files remain uncommitted; no build output or proprietary assets are staged.

- [ ] **Step 8: Commit evidence and promotions**

```powershell
git add docs/evidence/timing-domains-2026-07-14.json docs/evidence/timing-domains-2026-07-14.md docs/evidence/requirements.csv
git commit -m "docs: prove gameplay timing at 60 and 120 fps"
```

## Self-Review Record

- Spec coverage: all nine approved domains, explicit opt-in, read-only completion observation, sequential production actions, 60/120 comparison, timeouts, structured evidence, display/driver/Steam invariants, and ledger gates map to Tasks 1-6.
- Scope exclusions are explicit: this plan does not claim multiplayer determinism or compatibility on unavailable RDNA 1, RDNA 2, NVIDIA, or Intel hardware.
- Placeholder scan: every implementation step contains concrete code, commands, expected outcomes, and defined neighboring interfaces.
- Type consistency: the observer identifiers and signatures produced in Task 2 are consumed unchanged by Tasks 3-5; startup selection from Task 1 is consumed by Task 4.
