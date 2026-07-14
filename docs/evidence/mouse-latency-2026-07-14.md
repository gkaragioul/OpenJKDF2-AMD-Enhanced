# Mouse input latency verification — 2026-07-14

## Scope

This run verifies the production Windows/SDL relative-mouse path from an SDL
motion event's timestamp through dispatch and consumption by
`stdControl_ReadMouse`. It complements the focus/capture restoration evidence
in `display-lifecycle-2026-07-14.md` and the Modern/Classic binding evidence in
`modern-input-2026-07-14.md`.

## Method

- Release executable: `build/msvc-release/openjkdf2-64.exe`
- Data directory: the read-only local Steam Jedi Knight installation
- Fresh writable user directory:
  `runtime-evidence/mouse-latency-2026-07-14-01`
- Desktop mode before and after: `2560x1440@165`
- Harness: `scripts/test-modern-input.ps1 -Preset Modern`
- Stimulus: eight Win32 relative pointer moves, 100 ms apart, while the
  gameplay window held foreground focus
- Instrumentation was enabled only through
  `OPENJKDF2_VALIDATE_MOUSE_LATENCY=1`; normal runs do not emit per-event
  telemetry.

The dispatch measurement is SDL event timestamp to the engine event handler.
The consume measurement is handler entry to `stdControl_ReadMouse`; total is
SDL timestamp to control consumption. The harness requires at least six
consumed samples and a total p95 no greater than 50,000 microseconds.

## Results

The runtime selected `mouse_backend=SDL_relative raw_preferred=true
acceleration=off`. All eight injected movements reached the gameplay control
consumer and changed player yaw.

| Measurement | Minimum | Median | p95 / maximum |
|---|---:|---:|---:|
| SDL queue to handler | 170 us | 2,868.5 us | 6,029 us |
| Handler to control consume | 46 us | 55 us | 67 us |
| SDL event to control consume | 233 us | 2,926.5 us | 6,078 us |

With eight samples, the nearest-rank p95 is the maximum. The measured 6.078 ms
end-to-end p95 is below the harness's conservative 50 ms acceptance bound.
The process exited cleanly, the screenshot was produced, the original asset
tree metadata did not change, and the desktop mode remained exactly
`2560x1440@165`.

The Release build and all 34 registered tests passed after the telemetry was
added. `mouse_latency_telemetry_contract` prevents removal or accidental
disconnect of the dispatch/consume instrumentation and its opt-in gate.

## Boundary

This is an automated software-path latency measurement on the available
Windows host. It excludes physical mouse sensor, USB polling, compositor, scan
out, and display response latency, so it is not a click-to-photon claim. It
does prove prompt delivery through the engine's selected SDL relative-input
path. If raw relative input is unavailable, the existing SDL grabbed-relative
fallback remains available and is logged, but that fallback was not selected
by this run.

