# Production timing-domain invariance -- 2026-07-15

## Scope

The Release x64 build ran one validated warm-up followed by six scored first-level
runs: three with an explicit 60 FPS presentation cap and three at 120 FPS. The
scored order was interleaved as 60/120/120/60/60/120. A guarded observer drove
validation-owned objects through the engine's production weapon, AI, physics,
puppet animation, particle, event-script, voice, cutscene-decoder, and level-load
paths. Every domain had to produce exactly one identity-scoped completion event
in every run.

The weapon action uses the normal mount and selection readiness checks, commits
the normal pressed state before synchronous current-weapon COG activation, holds
that state until the real projectile-spawn path fires, and then performs the
normal deactivation. The harness does not focus the game or inject desktop mouse
input; separate input evidence covers the real control path.

Test host:

- Microsoft Windows 11 Pro 10.0.26200
- AMD Radeon RX 7900 XTX, driver 32.0.31021.5001
- Capture: 2026-07-15T11:40:01Z through 2026-07-15T11:42:20Z
- Release executable SHA-256:
  `A36C96C8D005E5BB330E6DC22F4AB19E4F53B56AEA3592BCDEFEC53BD8FE4C0C`

## Reproduction

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-timing-domains.ps1 `
  -Executable build\msvc-release\openjkdf2-64.exe `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -OutputDir '<fresh-directory-outside-the-game-installation>'
```

The harness creates a fresh user and diagnostics directory for every process,
waits on observer readiness rather than a fixed startup delay, validates the
warm-up, checks clean run-state markers, inspects the Windows Application log,
compares display state, and snapshots Steam asset metadata before and after the
batch. Child failures are deferred until display and event-log safety assertions
have been captured and evaluated.

## Acceptance rules

The rules were fixture-proven before the retained capture; no threshold was
changed after this dataset was collected.

- Cross-cap simulation medians may differ by at most 17 ms.
- Non-media within-cap spread is limited to one presentation interval plus 1 ms:
  18 ms at 60 FPS and 10 ms at 120 FPS.
- Dialogue and cutscene completion is asynchronous to the simulation tick and
  receives an 18 ms within-cap observation limit.
- Ordinary wall medians must satisfy the tighter of 5% relative difference and a
  50 ms absolute ceiling. Relative comparisons use a 100 ms denominator floor so
  sub-frame measurements are not dominated by near-zero percentages.
- Dialogue and cutscene use the same 5% rule with a 100 ms absolute ceiling.
- Level loading is an I/O boundary. It must load the expected level exactly once
  with no simulation delta, and its wall delta must remain within the larger of
  100 ms or 20%.
- The fixture suite accepts seven boundary-valid datasets and rejects eleven
  malformed, incomplete, duplicate, failed, or over-threshold datasets.

## Measured results

| Production domain | 60 FPS median | 120 FPS median | Simulation delta | Within-cap ranges (60/120) | Wall median delta | Result |
|---|---:|---:|---:|---:|---:|---|
| Weapon cooldown | 501 ms | 505 ms | 4 ms | 14 / 2 ms | 3.487 ms | pass |
| AI update window | 500 ms | 500 ms | 0 ms | 0 / 0 ms | 0.461 ms | pass |
| Physics displacement | 34 ms | 33 ms | 1 ms | 0 / 1 ms | 0.254 ms | pass |
| Puppet animation | 716 ms | 717 ms | 1 ms | 17 / 9 ms | 0.052 ms | pass |
| Particle lifecycle | 800 ms | 792 ms | 8 ms | 0 / 1 ms | 8.207 ms | pass |
| Queued script event | 517 ms | 508 ms | 9 ms | 0 / 1 ms | 7.636 ms | pass |
| Voice dialogue | 1366 ms | 1359 ms | 7 ms | 17 / 7 ms | 7.722 ms | pass |
| Decoded cutscene | 6750 ms | 6750 ms | 0 ms | 17 / 9 ms | 0.106 ms | pass |
| Level transition | 0 ms | 0 ms | 0 ms | 0 / 0 ms | 33.641 ms | pass |

The level transition completed to the episode-defined next level with wall-time
medians of 862.111 ms at 60 FPS and 828.470 ms at 120 FPS, a 3.902% difference.
Its simulation duration is intentionally zero at the post-load boundary because
the level loader resets game time; the observer's monotonic epoch preserves valid
ordering across that reset.

All seven processes completed all nine domains, wrote a clean run-state file, and
exited through the engine's historical success path (exit code 1). No Windows
Application Error was recorded. Every before/after display sample remained
2560x1440@165, Steam asset metadata was identical, and Exclusive fullscreen was
not invoked. No error-severity, failed-shader, software-renderer, or Exclusive
marker appeared in the seven diagnostics logs.

The compact machine-readable record retains every scored simulation and
wall-time sample in `timing-domains-2026-07-14.json`. Raw captures are not
committed because the isolated profiles contain autosaves made from proprietary
game data.

## Capture conditioning

The `+rpt_sparks` particle uses its production fixed lifetime; the observer does
not alter particle flags or duration. One validated warm-up run precedes
the scored interleaved samples. The harness rejects excessive within-cap
simulation spread before calculating medians, so a contaminated simulation
sample cannot be concealed by aggregation.

All wall-clock samples are retained. One 120 FPS level-transition sample took
23.941078 seconds while its simulation duration remained exactly zero. The
level-I/O comparison uses the predeclared cap medians; those medians passed, and
no scored sample was discarded.

A negative harness exercise used a harmless unit-test executable that exited
before creating a gameplay window. The harness failed as expected while still
writing metadata that recorded exit code 0, zero Application Errors, the explicit
window-readiness failure, and an unchanged 2560x1440@165 display.
The batch finally also completed the Steam asset-metadata comparison before
surfacing that original failure.

## Build and test gates

The exact required commands rebuilt the final source and passed all tests:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Debug -Test
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Release -Test
```

Debug passed 40/40 and Release passed 40/40. The earlier Smart App Control launch
block did not recur on the rebuilt binaries; its historical reproduction and
resolution are recorded in `smart-app-control-block-2026-07-15.md`. No Windows
security control was changed.

## Boundaries

This proves that these nine scoped production actions do not accelerate or
truncate when presentation changes from 60 to 120 FPS on the tested RX 7900 XTX
host. Combined with separate first-door, save/load, death/reload, restart, input,
and display evidence, it closes the tested single-player gameplay-timing scope.
It does not prove every asset, script, elevator, particle distribution,
multiplayer session, or unavailable GPU generation.
