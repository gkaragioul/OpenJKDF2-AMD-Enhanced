# Timed display confirmation and automatic revert evidence — 2026-07-14

## Scope

This run verifies the production Windows display-change confirmation path on the
local Windows 11 RX 7900 XTX system. The opt-in route invokes the same
`jkGuiDisplay_ApplyDisplayChange()` function used by the Video menu, changes only
between Windowed and Borderless modes, and never selects Exclusive Fullscreen.

## Reproduction

After the Release build, from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-display-confirmation-timeout.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -UserDir 'runtime-evidence\display-confirmation-2026-07-14-02'
```

The harness creates an isolated configuration, skips the startup intro, waits
for structured confirmation-start telemetry, captures the actual Windows task
dialog, provides no input to it, and waits for the production 15-second timer.
It compares the Windows desktop mode and read-only Steam asset-tree metadata
before and after the process.

## Measured result

| Measurement | Result |
|---|---:|
| Original mode | Borderless |
| Proposed safe mode | Windowed |
| Dialog visibly reviewed | yes |
| Confirmation input sent | none |
| Confirmation elapsed time | 16,866 ms |
| `display_settings_reverted` emitted | yes |
| Engine settings restored exactly | yes |
| Validation result | pass |
| Desktop before/after | 2560x1440@165 / 2560x1440@165 |
| Steam asset metadata invariant | yes |
| Clean run state and process-finished event | yes |
| Process exit code | 1 (normal application exit) |

The authoritative machine-readable result is
`runtime-evidence/display-confirmation-2026-07-14-02/display-confirmation-result.json`.
The screenshot and runtime logs are intentionally ignored by Git because a
desktop capture can include local user content.

## Related evidence

- `test_display_transaction` covers equality, apply requirements, confirmation,
  cancellation, expiry, and tick wraparound.
- `docs/evidence/frame-rate-ui-2026-07-14.md` proves a real Video-menu Apply and
  persisted restart.
- `docs/evidence/gameplay-crash-restoration-2026-07-14.md` proves accepted
  last-known-good recovery after a real controlled gameplay crash.
- Video-menu source contracts cover explicit Apply, recommended/safe reset, and
  configuration persistence paths.

## Conclusion and boundary

The live risky-change dialog and its unattended automatic revert are now proven
for the desktop-safe Windowed/Borderless path. Exclusive Fullscreen remains
separately gated and was not exercised.
