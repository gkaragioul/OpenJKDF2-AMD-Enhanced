# Aspect-correction runtime evidence — 2026-07-14

## Scope

This run verifies the production renderer's independently configurable aspect
policies for decoded video, 640x480 menus, and the in-game HUD at 2560x1440.
The capture observer is disabled unless `OPENJKDF2_ASPECT_CAPTURE` is set. It
captures the composed window framebuffer and exits through normal teardown.

## Reproduction

From the repository root after the Release build:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-aspect-correction.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -EvidenceRoot 'runtime-evidence\aspect-correction-2026-07-14-08'
```

The harness uses a fresh writable profile per domain. It advances the authored
intro with a Win32 Escape key event for the menu run, samples each PNG to reject
blank captures, and compares both the Windows display mode and the complete
Steam asset-tree metadata before and after all runs.

## Measured results

| Domain | Content | Preserve | Destination | Visible samples | Result |
|---|---:|---:|---:|---:|---|
| Video | 640x300 | yes | 0,120,2560,1200 | 424 | authored image visible with correct letterboxing |
| Menu | 640x480 | yes | 320,0,1920,1440 | 2297 | undistorted 4:3 UI with symmetric pillarboxing |
| HUD | 640x480 | no | 0,0,2560,1440 | 3177 | gameplay and HUD fill the configured viewport |

All three Release processes returned the expected code, recorded clean run
state, and emitted `process_finished`. The desktop remained 2560x1440@165 and
all 75 legitimate Steam asset files retained identical length and modification
metadata. Visual inspection confirmed visible video content, the pillarboxed
player menu, and full-width first-level gameplay with both HUD corners present.

The policy itself is covered by `test_aspect_policy`; the menu renderer and mouse
mapping share a contract test; Debug and Release each pass all 25 CTest tests.

## Boundaries

This proves the default independent policies at 2560x1440 on the local RX 7900
XTX system. The settings and menu expose independent gameplay, menu, HUD, and
video toggles, but this run does not cover every toggle combination, arbitrary
resolutions, multi-monitor placement, DPI scaling, or other GPU families.
