# Modern controls and borderless defaults verification

Date: 2026-07-16 (Europe/Athens)

## Delivered behavior

- Modern is the default control preset for new profiles and safe migration of
  untouched stock Classic profiles.
- Modern binds WASD movement, mouse look, left-click primary fire, right-click
  secondary fire, Space jump, Ctrl/C crouch, Shift run, E activate, mouse-wheel
  and number-key weapon selection, Tab map, and Escape menu.
- A saved Modern/Classic Control Style selector is available under
  **Setup > Controls > Control Options**. Customized legacy bindings are not
  overwritten by automatic migration.
- Borderless Fullscreen is the clean-install and exact-stock-settings default.
  Display Options exposes Windowed, Borderless Fullscreen (Recommended), and
  safety-gated Exclusive Fullscreen.

## Build and automated tests

The implementation build completed both exact gates successfully before the
documentation-only closeout commits:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Debug -Test
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Release -Test
```

- Debug: 46/46 tests passed (171.62 seconds).
- Release: 46/46 tests passed (85.75 seconds).
- `scripts/check-control-preset-mappings.ps1`: passed.
- `scripts/check-video-menu.ps1`: passed.

The final fresh Release verification rebuilt the full engine and passed 46/46
tests with zero failures in 90.72 seconds.

The final fresh Debug verification rebuilt and linked the full engine, and all
45 tests that Windows permitted to start passed. Windows enterprise Code
Integrity blocked the newly linked, unsigned `test_path_overlay.exe` before its
test body started, so CTest reported 45/46 plus `BAD_COMMAND`. Windows Code
Integrity events 3033 and 3077 name the enterprise signing policy as the cause.
The earlier Debug implementation gate had already executed and passed 46/46.
No security policy was disabled or bypassed.

The same host exhibited this hash-reputation behavior during the pre-change
baseline, when Windows blocked a newly linked `test_display_selection.exe`.

## Runtime acceptance

All runtime probes used the Release build and read the owned Steam data at
`D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight` without modifying it.

### Input

- Fresh default profile: effective preset Modern; bindings valid; movement and
  mouse turning observed; 716 latency samples; p95 total latency 4,676 us;
  clean exit; desktop and Steam metadata invariant.
- Explicit Classic profile: Classic bindings valid; movement and mouse turning
  observed; 758 samples; p95 total latency 5,109 us; clean exit; desktop and
  Steam metadata invariant.
- Explicit Modern persistence path: Modern bindings valid; movement and mouse
  turning observed; 793 samples; p95 total latency 5,500 us; clean exit.

Evidence roots:

- `build/runtime-evidence/modern-default-input-20260715c`
- `build/runtime-evidence/classic-input-20260715`
- `build/runtime-evidence/persisted-modern-input-20260715b`

The input harness was hardened after its first run revealed that fixed-delay
input could be sent before gameplay was ready. It now waits for the engine's
input-ready telemetry and verifies actual foreground ownership before injecting
events.

### Display and lifecycle

`scripts/test-display-options.ps1` passed all safe cases on a three-monitor host:

- Windowed 1920x1080 on the primary monitor.
- Windowed 3840x2160 on the primary monitor.
- Borderless 2560x1440 at the primary desktop's existing 165 Hz mode.
- Windowed 1280x720 on monitor 2.

Every case preserved the desktop mode, persisted the requested safe mode and
windowed size, exited cleanly, produced the expected-size presentation capture,
and left Steam metadata unchanged.

`scripts/test-display-lifecycle.ps1` passed three verified focus transfers,
focus/capture release telemetry, clean shutdown, forced-termination recovery,
cursor-clip restoration, desktop-mode restoration, and Steam metadata
invariance.

Evidence roots:

- `build/runtime-evidence/display-options-20260716`
- `build/runtime-evidence/display-lifecycle-20260716b`

Exclusive Fullscreen was not runtime-tested. The runtime and UI safety gate
remain in force.

## Package and installation

The package was produced from clean source commit
`794dfcdd3a95a3b8351492f1f0833c45f3a2bc70`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows-package.ps1 -BuildDir build\msvc-release -OutputDir build\modern-defaults-package
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\verify-windows-package.ps1 -ZipPath build\modern-defaults-package\OpenJKDF2-AMD-Enhanced-windows-x64.zip
```

- Archive: `build/modern-defaults-package/OpenJKDF2-AMD-Enhanced-windows-x64.zip`
- SHA-256: `165e75c22f620cb8ed26d7de260ef19f0733207fc5e5079975b3e0c0909231d0`
- Files verified: 37
- Manifest valid: yes
- Proprietary findings: 0
- Installed package mismatch count: 0

Installed paths:

- Program: `%LOCALAPPDATA%\Programs\OpenJKDF2 AMD Enhanced`
- Shortcut: `%USERPROFILE%\Desktop\OpenJKDF2 AMD Enhanced.lnk`
- User data: `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced`
- Pre-migration backup:
  `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced\migration-backup-2026-07-16-modern-defaults`

The launcher discovery-only check returned `valid=true`, source `saved`, and the
owned Steam data path above. The shortcut target, working directory, icon, and
target existence were verified.

## Live migration result

The named `georgekgaming` profile was migrated through the engine's opt-in
persistence path after the backup was created:

- `controlpreset=1` (Modern)
- `controlpresetversion=1`
- Modern binding validation: true
- `Window_displayMode=1` (Borderless)
- `Window_isFullscreen=true`
- `Window_defaultsVersion=1`
- Stored Windowed fallback size preserved at 1280x960
- FOV 111, raw mouse enabled, mouse acceleration disabled, and mouse smoothing
  disabled were preserved

The installed executable SHA-256 exactly matched the packaged executable. The
live run selected Borderless at the unchanged 2560x1440 desktop mode, persisted
Modern successfully, disarmed the watchdog on clean exit, and logged
`process_finished`.
