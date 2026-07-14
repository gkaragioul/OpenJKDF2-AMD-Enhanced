# Rendering cadence and first-door timing evidence — 2026-07-14

## Scope

This acceptance test compares the same first-level door interaction at explicit
60 and 120 FPS presentation caps in the Release build. VSync is disabled for the
capture. Real Win32 keyboard and mouse input drives the production Modern control
preset, activates the real wall switch, moves both physical door things, and
crosses the opened doorway. The guarded observer records presentation frame times
and wall-clock door travel without modifying door speed or script timing.

## Reproduction

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-timing-invariance.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -EvidenceRoot '<fresh-directory>'
```

Each child run requires a fresh user directory and independently verifies the
foreground gameplay window, clean shutdown, screenshot creation, display-mode
invariance, and unchanged Steam asset metadata. Runtime evidence is ignored by Git
because screenshots contain proprietary game imagery.

## Measured results

| Measurement | 60 FPS cap | 120 FPS cap |
|---|---:|---:|
| Frame samples | 60 | 60 |
| Target frame time | 16.6667 ms | 8.3333 ms |
| Median frame time | 16.6510 ms | 8.3232 ms |
| p95 frame time | 17.0941 ms | 8.5754 ms |
| p99 / worst frame time | 17.9506 ms | 8.8171 ms |
| Both doors' measured travel | 466 ms | 473 ms |
| Door maximum displacement | 0.2000 | 0.2000 |
| Doorway crossed | yes | yes |
| Display before/after | 2560x1440@165 | 2560x1440@165 |

The median pacing errors were approximately 0.094% at 60 FPS and 0.122% at
120 FPS. Door duration differed by 7 ms, or 1.48%, which is below the harness's
20 ms and 5% observation tolerances. The higher rendering rate therefore did not
accelerate or truncate this measured scripted door interaction.

## Boundaries

This proves 60/120 presentation pacing and first-door timing invariance on the
tested RX 7900 XTX system. It does not by itself prove timing invariance for every
elevator, weapon, dialogue, cutscene, animation, particle, AI behavior, physics
interaction, or multiplayer session, and it does not imply RDNA 1/2 validation.
