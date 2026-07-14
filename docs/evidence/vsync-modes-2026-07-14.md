# VSync and limiter runtime evidence — 2026-07-14

## Scope

This acceptance run verifies all three production presentation modes on the
local RX 7900 XTX system: VSync Off with the cap-only limiter, VSync On, and
Adaptive VSync. The guarded observer applies each requested setting through the
same live globals used by the Video menu, records the mode actually accepted by
SDL, samples the real swap/pacing loop, captures the composed gameplay frame,
and exits through normal teardown.

## Reproduction

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-vsync-modes.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -EvidenceRoot 'runtime-evidence\vsync-modes-2026-07-14-04'
```

Each mode uses a fresh writable profile and a 10-second first-level run. The
harness rejects missing or visually blank captures and compares the Windows
display mode and complete Steam asset-tree metadata before and after the matrix.

## Measured results

| Requested mode | Applied mode | Frame cap | Median | p95 | p99 / worst |
|---|---|---:|---:|---:|---:|
| Off | Off | 120 FPS | 8.3332 ms | 8.7175 ms | 8.8981 ms |
| On | On | 60 FPS | 16.6624 ms | 17.1967 ms | 17.6148 ms |
| Adaptive | Adaptive | 60 FPS | 16.6577 ms | 17.0437 ms | 17.1375 ms |

All modes produced 60 telemetry samples and visible 2560x1440 gameplay captures.
SDL accepted Adaptive directly; no fallback was logged. Every process returned
the expected code, recorded clean run state, and emitted `process_finished`.
The desktop remained 2560x1440@165 and all 75 legitimate Steam asset files were
metadata-identical after the run.

The cap-only median error was below 0.01%. On and Adaptive median errors were
below 0.06% against the 60 FPS budget; all p95 values were within 3.4%.

## Additional observation and boundaries

An earlier diagnostic run with On and Adaptive uncapped was intentionally
rejected: it alternated around 5.55 and 11.09 ms on this VRR/high-refresh setup.
Pairing synchronization with the supported 60 FPS limiter produced the stable
results above. This evidence does not claim uncapped pacing is low-jitter, does
not measure scan-out tearing with a camera, and does not cover other drivers,
monitors, GPUs, or refresh rates.
