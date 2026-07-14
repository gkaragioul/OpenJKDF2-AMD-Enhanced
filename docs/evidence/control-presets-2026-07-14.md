# Modern and Classic control preset evidence — 2026-07-14

## Scope

This verification closes the control-preset requirement on the local Windows 11
RX 7900 XTX system. It combines a source-contract regression test with two
fresh-profile Release gameplay runs through the production preset application
paths. The runtime observer and synthetic input are opt-in and inactive during
ordinary play.

## Automated contract

`control_preset_mapping_contract` verifies that the production control system
exposes binding inspection and validation, supports runtime selection of both
presets, and emits structured binding-validation telemetry. The engine-side
validator inspects the live binding table after preset application.

The shared bindings validated for both presets are WASD movement, Tab map,
number-key weapon selection, and R inventory. Classic additionally validates X
jump, C crouch, Ctrl primary fire, Space activate, Z secondary fire, the legacy
mouse buttons, and mouse pitch. Modern validates Space jump, Ctrl crouch, E use,
right-click secondary fire, wheel weapon selection, and removal of conflicting
Classic Space/Ctrl bindings. Existing default bindings retain Shift run,
left-click primary fire, Escape menu, and the remaining game functions.

The exact Release command passed all 31 tests:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Release -Test
```

## Runtime reproduction

From the repository root after the Release build, using a fresh directory for
each run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-modern-input.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -UserDir 'runtime-evidence\control-presets-2026-07-14-02\modern' -Preset Modern

powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-modern-input.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -UserDir 'runtime-evidence\control-presets-2026-07-14-02\classic' -Preset Classic
```

The harness focuses the actual SDL gameplay window, sends a W hold and relative
mouse movement, and allows the production validation path to exit normally.

## Measured results

| Measurement | Modern | Classic |
|---|---:|---:|
| Live binding table valid | yes | yes |
| Player distance | 0.1950 | 0.1950 |
| Wrap-safe yaw delta | -103.602° | -77.102° |
| Start/end sector | 339 / 339 | 339 / 339 |
| Foreground gameplay window verified | yes | yes |
| Display before/after | 2560x1440@165 | 2560x1440@165 |
| Steam asset metadata invariant | yes | yes |
| Screenshot visibly reviewed | yes | yes |
| Clean run state and process-finished event | yes | yes |

The authoritative machine-readable results are `modern-input-result.json` and
`classic-input-result.json` under the caller-selected evidence directories.
Runtime screenshots and logs are intentionally ignored by Git because they can
contain proprietary game imagery or local system details.

## Conclusion and boundaries

Both selectable production presets now have complete binding-table validation
and actual gameplay movement/mouse evidence. This proves the preset requirement
on the tested system; it does not generalize input latency or GPU compatibility
to other hardware.
