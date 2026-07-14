# Modern input runtime evidence — 2026-07-14

## Scope

This run verifies the production Modern control preset with actual Win32 keyboard
and relative mouse events on the local RX 7900 XTX Windows 11 system. The
validation-only observer is disabled unless `OPENJKDF2_VALIDATE_INPUT_MS` is set.
It applies the same `sithControl_ApplyModernPreset()` path used by the Controls UI,
records only player deltas and sector IDs, and exits through the ordinary shutdown
path.

## Reproduction

From the repository root after the Release build:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-modern-input.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -UserDir '<fresh-directory>'
```

The harness requires a fresh writable user tree, identifies and focuses the SDL
gameplay window, sends a real W key hold and relative mouse motion, and compares
the Steam asset-tree metadata and Windows display mode before and after the run.

## Measured results

Two consecutive fresh-profile Release runs passed:

| Measurement | Run 1 | Run 2 |
|---|---:|---:|
| Player distance | 0.1950 | 0.1950 |
| Wrap-safe yaw delta | -91.012° | -92.486° |
| Start/end sector | 339 / 339 | 339 / 339 |
| Foreground SDL window verified | yes | yes |
| Display before/after | 2560x1440@165 | 2560x1440@165 |
| Steam asset metadata invariant | yes | yes |
| Screenshot written | yes | yes |
| Clean run state and process-finished event | yes | yes |

The authoritative machine-readable result and screenshot are generated under the
caller-selected user directory as `modern-input-result.json` and
`diagnostics/modern-input.png`. Runtime screenshots are intentionally ignored by
Git because they contain proprietary game imagery.

## Boundaries

This proves responsive Modern-preset forward movement and mouse turning through
the actual window/input stack on this machine. It does not yet prove Classic
preset behavior, Alt+Tab cursor reacquisition loops, every Modern binding, the
first-door interaction, or compatibility on other hardware.
