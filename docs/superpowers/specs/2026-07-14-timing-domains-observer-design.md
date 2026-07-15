# Timing Domains Validation Observer Design

Date: 2026-07-14
Status: approved design

## Purpose

Prove that selecting 120 FPS changes presentation cadence without accelerating,
truncating, or skipping representative gameplay timing domains. The observer
must exercise production game-loop paths, emit measured evidence, and leave
normal gameplay behavior unchanged when validation is not explicitly enabled.

This design covers an automated deterministic observer only. A longer external
real-input playthrough is outside this slice by user choice. Existing first-door,
save/load, death/reload, display, frame-pacing, and input evidence remains part
of the overall acceptance record.

## Selected approach

Use a guarded production observer activated by an explicit validation-only
startup option. It observes real subsystem state while the normal fixed-step
simulation and presentation loop run. It does not replace subsystem functions,
modify rates, synthesize successful outcomes, or advance time itself.

This approach is stronger than isolated unit simulations because it includes
the production loop, asset loading, scripts, renderer, and subsystem integration.
It is more deterministic than external input automation and can identify the
specific domain that timed out or diverged.

## Scope

The observer must measure these representative domains:

1. Weapon timing: a production player-weapon activation remains held until the
   real projectile-spawn path fires, then follows normal deactivation and
   cooldown/ready transitions without an extra or missing activation.
2. AI timing: a live AI actor accumulates production update ticks and reaches a
   deterministic observer milestone without skipped or duplicated updates.
3. Physics timing: a production physics thing travels a bounded displacement;
   wall-clock and simulation-time completion are recorded.
4. Animation timing: a real puppet/keyframe animation reaches its completion
   state and reports its nominal and observed duration.
5. Particle timing: a real particle thing reaches its production expiry/removal
   state after its configured lifetime.
6. Script timing: a COG timer or scripted movement event fires once at its
   configured simulation time.
7. Dialogue timing: a real voice/sound playback lifecycle starts and completes
   without being cut short by presentation rate. If the selected deterministic
   asset lacks a reliable completion callback, the observer must fail rather
   than infer completion from elapsed time alone.
8. Cutscene timing: the production cutscene lifecycle starts and reaches its
   natural completion or transition event. The observer may skip rendering
   frames only through the engine's normal playback behavior; it may not force
   the completion flag.
9. Level transition: the production transition request loads the expected next
   level and reaches its post-load gameplay state exactly once.

Multiplayer determinism is not claimed by this observer. It remains explicitly
unverified until a real two-peer test is available.

## Activation and isolation

Add a validation-only option named `--validation-observer=timing-domains` using
the existing startup-option pattern. The option is disabled by default and is
not exposed as a normal player setting.

Activation requires all of the following:

- an explicit validation observer option;
- an explicit data directory and fresh user directory supplied by the harness;
- single-player autostart into the selected deterministic scenario;
- Windowed or Borderless mode; Exclusive Fullscreen is rejected;
- a finite frame cap of 60 or 120 selected by the harness.

If any guard is absent, the observer reports a configuration failure and exits
without running the scenario. Normal launches compile the observer code but do
not allocate observer state, change game state, or emit validation events.

## Architecture

### TimingDomainsObserver

A focused module owns validation state and structured event emission. Its public
surface is limited to startup configuration, per-tick observation, explicit
subsystem notifications, finalization, and read-only status queries.

The module stores, for each domain:

- start and completion simulation tick;
- start and completion high-resolution wall-clock timestamp;
- expected terminal state or event count;
- observed terminal state or event count;
- timeout and failure reason;
- whether the measurement is available and trustworthy.

The observer module never owns or mutates gameplay objects. The runtime bridge
may retain pointer identity only for the current validation action, clears that
identity before advancing, and never carries a gameplay pointer across a level
load. Tracked telemetry records stable identifiers rather than pointer values.

### TimingDomainsScenario

A separate validation-only scenario driver initiates each measured action at a
scheduled simulation tick. It may invoke the same production entry points that
normal input, scripts, or level logic invoke—for example weapon activation,
animation start, sound/cutscene playback, and an end-level transition request.
It may select or create ordinary production things through existing world APIs
when the loaded level does not expose a stable candidate.

The driver may not write completion flags, remove things to simulate expiry,
advance clocks, call observer completion functions, bypass loaders, or mutate a
subsystem's configured duration. Once an action is initiated, only normal
production updates can complete it. The observer and scenario driver communicate
through stable phase identifiers, never gameplay object ownership.

The scenario uses legitimate installed assets but records only stable domain and
phase identifiers. Asset filenames, dialogue text, and media content are not
included in tracked telemetry.

### Production integration points

Small notification calls are placed immediately after existing production
events are known to have occurred, such as weapon activation, AI update,
animation completion, thing removal, COG timer dispatch, sound completion,
cutscene completion, and level-load completion. Notifications receive only the
minimum immutable values needed for validation.

The main simulation tick calls the observer's read-only sampling function after
all normal subsystem updates for that tick. Presentation-frame telemetry remains
the existing `FrameTelemetry` implementation and is not duplicated.

### Runtime harness

`scripts/test-timing-domains.ps1` launches one validated warm-up and then three
fresh Release runs per cap in an interleaved reversible order. Each run uses the
same source assets and deterministic scenario but a distinct user directory.
The warm-up is excluded by a rule fixed before capture; all six measured runs are
retained and scored. The harness:

- snapshots Windows display state before and after each process and Steam asset
  metadata around the complete batch;
- requires a real gameplay window and clean observer completion without focusing
  the process or injecting desktop input;
- parses the structured summary and per-domain lifecycle events;
- rejects missing, duplicate, timed-out, or failed domain records;
- rejects within-cap simulation spread over one fixed tick and compares 60/120
  medians;
- verifies the display invariant after every run and the Steam metadata invariant
  after the batch;
- writes a privacy-safe aggregate JSON result.

## Data flow

1. The harness creates fresh user directories, validates one warm-up process,
   and launches the six interleaved measured Release processes.
2. Startup parsing validates observer guards and records the requested frame cap.
3. The game autostarts the selected first-level validation context through normal
   load paths.
4. The scenario driver initiates one production action per phase at scheduled
   simulation ticks and then waits for observer-confirmed lifecycle events.
5. Production subsystems run at the stable simulation cadence while rendering
   follows the selected 60 or 120 FPS presentation cap.
6. Integration notifications and end-of-tick sampling populate observer records.
7. The final phase requests the normal end-level transition path and waits for
   the expected next-level post-load state.
8. After level-transition completion, or on the first failure, the observer emits
   one final structured `timing_domains_summary` event and requests normal exit.
9. The harness validates all scored runs and compares the two cap medians.

## Structured evidence

The combined per-run lifecycle events and aggregate result contain:

- schema version and observer name;
- requested presentation cap and the observer's monotonic simulation timeline;
- per-domain expected state, observed state, event counts, simulation ticks, all
  measured simulation/wall samples, medians, within-cap ranges, and pass/fail
  reason;
- overall completion state;
- selected map identifiers that contain no proprietary asset content;
- display and asset invariants supplied by the harness.

Tracked evidence must not include usernames, hostnames, absolute paths, serial
numbers, save contents, screenshots, proprietary text, dialogue transcripts, or
game assets. Raw local logs and captures remain under ignored runtime-evidence
directories.

## Comparison rules

For both caps, every domain must reach its expected terminal state exactly once
in every scored run. No domain may time out or report an unavailable measurement.
One fully validated warm-up precedes the scored batch to prevent cold asset or
security scanning from contaminating the first measurement.

The aggregate harness applies these rules:

- within-cap non-media simulation range: at most one presentation interval plus
  1 ms (18 ms at 60 FPS and 10 ms at 120 FPS); asynchronous dialogue and
  cutscene completion use an 18 ms floor;
- per-domain simulation-duration medians: equal within one 60 Hz simulation tick
  (17 ms);
- ordinary wall-clock medians: within the tighter of 50 ms or 5%; dialogue and
  cutscene use the tighter of 100 ms or 5%. Relative comparisons use a 100 ms
  denominator floor for short domains;
- level-transition wall medians: within the larger of 100 ms or 20%, because
  level loading is an I/O boundary, while its target and simulation delta remain
  exact;
- event count and terminal state: exact equality;
- level transition target and post-load state: exact equality;
- display state and Steam metadata: exact equality before and after each run.

Any necessary random behavior uses the engine's existing deterministic seed
mechanism with a seed recorded in telemetry. A result cannot pass by omitting an
unmeasurable domain.

## Failure handling

Each domain has a finite timeout derived from its expected production duration
plus a documented margin. Failures emit the domain, last observed state, current
simulation tick, elapsed wall time, and a stable reason code.

The observer finalizes once. A crash, renderer failure, invalid asset set,
unexpected level, duplicate completion, missing callback, or harness timeout is
a hard failure. The harness then terminates only a Windowed or Borderless process
and still performs display and asset invariant checks.

The observer must not silently skip dialogue, cutscene, or level-transition
coverage. If current game data or lifecycle hooks cannot provide trustworthy
measurement, the requirement remains incomplete and the report explains why.

## Testing strategy

### Unit and contract tests

- Startup parser tests cover disabled default, valid activation, invalid value,
  missing data/user directory, unsupported frame cap, and Exclusive rejection.
- Observer unit tests cover ordered state transitions, duplicate events, missing
  events, timeouts, one-shot finalization, and JSON-safe stable reason codes.
- A source contract test confirms all required production notification points
  remain connected.
- Harness fixtures accept seven boundary-valid datasets and reject eleven
  missing, duplicate, failed, malformed, invariant, or threshold-invalid cases.
- Source contracts require child and batch failures to remain deferred until
  display, Application Error, and Steam metadata safety checks are captured.

### Runtime verification

- Run the existing exact Debug and Release build/test commands.
- Run `test-timing-domains.ps1` against legitimate Steam data in fresh evidence
  directories.
- Inspect both raw summaries and the aggregate comparison.
- Confirm normal launch behavior remains unchanged with no validation flag.
- Review Application Error logs for engine or driver crashes during the capture.

## Acceptance for this slice

This slice is complete only when:

- all nine scoped domains are measured through production lifecycle events at
  both 60 and 120 FPS;
- all comparison and safety rules pass on the tested RX 7900 XTX system;
- Debug and Release test suites pass;
- privacy-safe evidence and limitations are documented;
- M5-TIMING and AC-120FPS are updated only to the extent the evidence proves;
- M9-GAMEPLAY is not marked proven unless its remaining dialogue, cutscene, and
  level-transition requirements all pass;
- multiplayer and unavailable GPU generations remain explicitly unverified.

## Non-goals

- Changing gameplay timing or balancing values.
- Replacing the stable fixed-step simulation.
- Adding a user-facing benchmark mode.
- Claiming compatibility for untested RDNA 1/2, NVIDIA, or Intel hardware.
- Claiming multiplayer determinism from a single-player observer.
- Redistributing any Jedi Knight game asset or derived proprietary content.
