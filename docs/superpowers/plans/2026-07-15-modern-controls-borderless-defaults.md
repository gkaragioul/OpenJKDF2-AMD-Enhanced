# Modern Controls and Borderless Defaults Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Modern controls and Borderless Fullscreen the effective defaults, provide saved Modern/Classic and display-mode selectors, safely migrate untouched legacy defaults, and update the installed desktop build.

**Architecture:** Keep default and migration decisions in small platform-neutral C helpers, then connect those decisions to the existing profile binding table and Windows/SDL registry startup path. Reuse the current preset application, display transaction, watchdog guard, packaging, and runtime harnesses; do not add a second settings store.

**Tech Stack:** C11, CMake/CTest, SDL2, existing JSON registry/profile persistence, PowerShell 5.1 packaging and runtime harnesses, Windows x64.

## Global Constraints

- `CONTROL_PRESET_MODERN` is the default for every new player profile.
- Modern binds WASD, mouse look, left-click primary fire, right-click secondary fire, Space jump, Ctrl/C crouch, Shift run, E activate, wheel weapon selection, number-key weapon selection, Tab map, and Escape menu.
- Right mouse must not remain bound to Jump after Modern is applied.
- Customized legacy binding tables must not be overwritten automatically.
- Borderless Fullscreen is the recommended and clean-install default.
- Windowed and Borderless Fullscreen never request a physical display-mode change.
- Exclusive Fullscreen remains visible but unavailable unless the existing restoration guard is ready.
- Never run an Exclusive Fullscreen test.
- Original Steam/GOG assets remain read-only and are never packaged or copied.

---

### Task 1: Pure default and migration policy

**Files:**
- Modify: `src/General/ControlPreset.h`
- Modify: `src/General/ControlPreset.c`
- Create: `src/General/DefaultSettingsMigration.h`
- Create: `src/General/DefaultSettingsMigration.c`
- Modify: `src/Tests/test_control_preset.c`
- Create: `src/Tests/test_default_settings_migration.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Produces: `ControlPreset ControlPreset_Default(void)` returning `CONTROL_PRESET_MODERN`.
- Produces: `int default_settings_should_migrate_controls(int stored_version, int preset, int exact_stock_classic)`.
- Produces: `DisplayMode default_settings_migrate_display(int stored_version, DisplayMode stored_mode, int width, int height, int stock_width, int stock_height, int *changed)`.
- Produces: `CONTROL_DEFAULTS_VERSION == 1` and `WINDOW_DEFAULTS_VERSION == 1`.

- [ ] **Step 1: Write failing preset-default assertions**

Add to `src/Tests/test_control_preset.c`:

```c
assert(ControlPreset_Default() == CONTROL_PRESET_MODERN);
assert(ControlPreset_Normalize(-1) == ControlPreset_Default());
assert(ControlPreset_Normalize(99) == ControlPreset_Default());
```

- [ ] **Step 2: Write the failing migration-policy test**

Create `src/Tests/test_default_settings_migration.c` with assertions that:

```c
int changed = 0;
assert(default_settings_should_migrate_controls(0, CONTROL_PRESET_CLASSIC, 1));
assert(!default_settings_should_migrate_controls(0, CONTROL_PRESET_CLASSIC, 0));
assert(!default_settings_should_migrate_controls(1, CONTROL_PRESET_CLASSIC, 1));
assert(!default_settings_should_migrate_controls(0, CONTROL_PRESET_MODERN, 1));
assert(default_settings_migrate_display(0, DISPLAY_MODE_WINDOWED,
    1280, 960, 1280, 960, &changed) == DISPLAY_MODE_BORDERLESS);
assert(changed == 1);
changed = 0;
assert(default_settings_migrate_display(0, DISPLAY_MODE_WINDOWED,
    1600, 900, 1280, 960, &changed) == DISPLAY_MODE_WINDOWED);
assert(changed == 0);
assert(default_settings_migrate_display(1, DISPLAY_MODE_WINDOWED,
    1280, 960, 1280, 960, &changed) == DISPLAY_MODE_WINDOWED);
```

Register the test with `DefaultSettingsMigration.c`, `ControlPreset.c`, and `DisplayMode.c` in `cmake/OpenJKDF2Tests.cmake`.

- [ ] **Step 3: Run the focused tests and confirm RED**

Run:

```powershell
cmake --build build/msvc-debug --config Debug --target test_control_preset test_default_settings_migration
ctest --test-dir build/msvc-debug -C Debug -R "test_(control_preset|default_settings_migration)" --output-on-failure
```

Expected: compilation fails because the new declarations and migration files do not exist.

- [ ] **Step 4: Implement the minimal pure helpers**

Implement `ControlPreset_Default()` and make unknown normalization return it. Implement the migration helpers as pure functions: controls migrate only for version less than `1`, Classic, and exact stock equality; display migrates only for version less than `1`, Windowed, and exact stock dimensions. Set `*changed` deterministically when non-null.

- [ ] **Step 5: Run the focused tests and confirm GREEN**

Run the Step 3 commands. Expected: both tests pass.

- [ ] **Step 6: Commit the policy unit**

```powershell
git add src/General/ControlPreset.* src/General/DefaultSettingsMigration.* src/Tests/test_control_preset.c src/Tests/test_default_settings_migration.c cmake/OpenJKDF2Tests.cmake
git commit -m "feat: define modern default migration policy"
```

---

### Task 2: Engine profile defaults and exact legacy control migration

**Files:**
- Modify: `src/Devices/sithControl.h`
- Modify: `src/Devices/sithControl.c`
- Modify: `src/World/jkPlayer.h`
- Modify: `src/World/jkPlayer.c`
- Modify: `src/Main/jkGame.c`
- Modify: `scripts/check-control-preset-mappings.ps1`
- Modify: `scripts/test-modern-input.ps1`

**Interfaces:**
- Consumes: `ControlPreset_Default()` and `default_settings_should_migrate_controls(...)` from Task 1.
- Produces: `void sithControl_CaptureClassicPresetSnapshot(void)`.
- Produces: `int sithControl_MatchesCapturedClassicPreset(void)` using order-insensitive binding comparison.
- Produces: `int jkPlayer_controlPresetVersion`, persisted as `controlpresetversion`.
- Produces: opt-in `OPENJKDF2_PERSIST_INPUT_PRESET=1` support for the existing input-validation path; when paired with `OPENJKDF2_VALIDATE_INPUT_PRESET=Modern|Classic`, it writes the selected preset to the active profile before the validation process exits.

- [ ] **Step 1: Expand the source-contract test before production edits**

Require these tokens in `scripts/check-control-preset-mappings.ps1`:

```powershell
'ControlPreset_Default()'
'sithControl_ApplyModernPreset();'
'sithControl_CaptureClassicPresetSnapshot'
'sithControl_MatchesCapturedClassicPreset'
'controlpresetversion'
'CONTROL_DEFAULTS_VERSION'
'KEY_MOUSE_B2'
'DIK_SPACE'
'DIK_E'
```

Also reject `jkPlayer_controlPreset = CONTROL_PRESET_CLASSIC` and the Classic cvar default in `src/World/jkPlayer.c`.

- [ ] **Step 2: Run the contract and confirm RED**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-control-preset-mappings.ps1
```

Expected: failure naming the missing modern-default and migration contracts.

- [ ] **Step 3: Implement exact stock-Classic recognition**

Add a static canonical `stdControlKeyInfo[INPUT_FUNC_MAX]` snapshot and validity flag in `sithControl.c`. `sithControl_CaptureClassicPresetSnapshot()` applies the platform's Classic preset before profile bindings are read, then copies the canonical table. `sithControl_MatchesCapturedClassicPreset()` compares every function's entry count and matches each entry order-independently by control id, behavior flags, and axis scale. It returns false if no snapshot exists, any entry differs, or any entry is added/removed.

- [ ] **Step 4: Make Modern the profile default and add one-time migration**

Initialize, register, and reset `jkPlayer_controlPreset` with `ControlPreset_Default()`. Initialize/reset `jkPlayer_controlPresetVersion` to `CONTROL_DEFAULTS_VERSION` for a new profile. In `jkPlayer_CreateConf()`, call `sithControl_ApplyModernPreset()`.

For an existing profile, capture the Classic canonical table immediately before `sithControl_ReadConf()`, compare immediately after it, read a missing `controlpresetversion` as `0`, and call:

```c
if (default_settings_should_migrate_controls(
        jkPlayer_controlPresetVersion,
        jkPlayer_controlPreset,
        sithControl_MatchesCapturedClassicPreset())) {
    sithControl_ApplyModernPreset();
    jkPlayer_controlPreset = CONTROL_PRESET_MODERN;
}
jkPlayer_controlPresetVersion = CONTROL_DEFAULTS_VERSION;
```

Persist the version and migrated binding table after the profile read file is closed. Do not re-apply either preset on ordinary later loads.

- [ ] **Step 5: Add default-preset runtime validation**

Extend `scripts/test-modern-input.ps1` with `Preset = Default`. For Default, do not set `OPENJKDF2_VALIDATE_INPUT_PRESET`; set a separate current-binding validation flag and require diagnostics to report `preset=Modern`, `bindings_valid=true`, Space Jump, right-click Secondary Fire, and no right-click Jump. Retain the explicit Modern and Classic cases.

In the existing opt-in input observer, treat an absent preset environment variable as "validate the current saved/default preset without applying another preset." When `OPENJKDF2_PERSIST_INPUT_PRESET=1` is present with an explicit Modern or Classic request, set `jkPlayer_controlPreset`, set `jkPlayer_controlPresetVersion`, and call `jkPlayer_WriteConf(jkPlayer_playerShortName)` after applying the bindings. This path remains inert when the validation variables are absent.

- [ ] **Step 6: Run control tests and confirm GREEN**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-control-preset-mappings.ps1
ctest --test-dir build/msvc-debug -C Debug -R "control_preset" --output-on-failure
```

Expected: the source contract and pure preset unit test pass.

- [ ] **Step 7: Commit the engine control unit**

```powershell
git add src/Devices/sithControl.* src/World/jkPlayer.* src/Main/jkGame.c scripts/check-control-preset-mappings.ps1 scripts/test-modern-input.ps1
git commit -m "feat: default profiles to modern controls"
```

---

### Task 3: Saved control-style selector and display defaults

**Files:**
- Modify: `src/Gui/jkGUIControlOptions.c`
- Modify: `resource/ui/openjkdf2.uni`
- Modify: `src/Platform/SDL2/jkGUIDisplay.c`
- Modify: `src/Win95/Window.c`
- Modify: `scripts/check-control-preset-mappings.ps1`
- Modify: `scripts/check-video-menu.ps1`
- Modify: `scripts/test-display-options.ps1`

**Interfaces:**
- Consumes: `CONTROL_DEFAULTS_VERSION`, `WINDOW_DEFAULTS_VERSION`, and `default_settings_migrate_display(...)`.
- Produces: a two-position Control Style selector persisted through the existing profile writer.
- Produces: `Window_defaultsVersion = 1` in the existing registry JSON.

- [ ] **Step 1: Write failing UI and display-migration contracts**

Make the scripts require:

```powershell
'GUIEXT_CONTROL_STYLE'
'GUIEXT_CONTROL_STYLE_MODERN'
'GUIEXT_CONTROL_STYLE_CLASSIC'
'GUIEXT_APPLY_CONTROL_STYLE'
'Borderless Fullscreen (Recommended)'
'Exclusive Fullscreen'
'Window_defaultsVersion'
'default_settings_migrate_display'
```

Require the control selector to initialize from `jkPlayer_controlPreset`, and reject the old pair of independent Apply Modern/Apply Classic buttons.

- [ ] **Step 2: Run both contracts and confirm RED**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-control-preset-mappings.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-video-menu.ps1
```

Expected: both fail on missing selector/labels/migration tokens.

- [ ] **Step 3: Implement the Control Style selector**

Replace the two preset buttons with a text label, two-position slider, dynamic `Modern`/`Classic` value text, and `Apply Control Style` button. Initialize it from `jkPlayer_controlPreset`. On Apply, show the existing confirmation dialog, apply the selected preset, set `jkPlayer_controlPresetVersion = CONTROL_DEFAULTS_VERSION`, refresh HUD inventory input, and write the current profile. Cancel does not change bindings.

- [ ] **Step 4: Implement the display labels and registry migration**

Show `Windowed`, `Borderless Fullscreen (Recommended)`, and `Exclusive Fullscreen`; append `(Unavailable)` to Exclusive when the guard is not ready. Before startup resolves saved display settings, read `Window_defaultsVersion`, run the pure migration decision, persist Borderless plus `Window_isFullscreen = true` only for stock legacy Windowed state, and always record `WINDOW_DEFAULTS_VERSION`. Preserve `Window_windowWidth` and `Window_windowHeight`.

- [ ] **Step 5: Run focused tests and confirm GREEN**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-control-preset-mappings.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/check-video-menu.ps1
ctest --test-dir build/msvc-debug -C Debug -R "(default_settings_migration|video_defaults|display_mode|display_selection)" --output-on-failure
```

Expected: all contracts and focused unit tests pass.

- [ ] **Step 6: Commit the UI/display unit**

```powershell
git add src/Gui/jkGUIControlOptions.c resource/ui/openjkdf2.uni src/Platform/SDL2/jkGUIDisplay.c src/Win95/Window.c scripts/check-control-preset-mappings.ps1 scripts/check-video-menu.ps1 scripts/test-display-options.ps1
git commit -m "feat: expose modern controls and fullscreen modes"
```

---

### Task 4: Full verification, package, and installed handoff

**Files:**
- Modify: `packaging/windows/CHANGELOG.md`
- Modify: `packaging/windows/README-PACKAGE.md`
- Modify: `packaging/windows/TROUBLESHOOTING.md`
- Create: `docs/evidence/modern-defaults-2026-07-15.md`
- Generated outside Git: `build/modern-defaults-package/OpenJKDF2-AMD-Enhanced-windows-x64.zip`

**Interfaces:**
- Consumes: all behavior from Tasks 1-3.
- Produces: verified Debug/Release builds, a clean Windows package, refreshed installed files and desktop shortcut, Modern current profile, and Borderless current registry.

- [ ] **Step 1: Update user documentation before the final build**

Document Modern as the new-profile default, the Control Style selector path, the exact core bindings, Borderless Fullscreen as default, all three Display Options labels, and the Exclusive restoration-guard limitation.

- [ ] **Step 2: Run exact Debug and Release gates**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Configuration Debug -Test
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Configuration Release -Test
```

Expected: build success and every runnable CTest passing. If enterprise Code Integrity blocks an unsigned test before startup, record the exact blocked binary/event rather than reporting an assertion failure.

- [ ] **Step 3: Run desktop-safe runtime acceptance**

Run Default and Classic fresh-profile input probes plus Windowed/Borderless display options and lifecycle tests. Require Modern on Default, Classic only when selected, persisted Modern after a fresh process, Borderless desktop geometry, identical display mode before/after, and unchanged Steam metadata. Do not select Exclusive.

- [ ] **Step 4: Build and verify a clean package**

Run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/build-windows-package.ps1 -BuildDir build/msvc-release -OutputDir build/modern-defaults-package
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/verify-windows-package.ps1 -PackageDir build/modern-defaults-package/OpenJKDF2-AMD-Enhanced-windows-x64
```

Expected: valid manifest, zero proprietary findings, and a SHA-256 for the zip.

- [ ] **Step 5: Reinstall and migrate this user's active configuration**

Back up the current `registry.json`, `.plr`, and player `openjkdf2.json` under `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced\migration-backup-2026-07-15`. Run the packaged installer with one-process execution-policy bypass. Launch the installed desktop-safe validation path with `OPENJKDF2_VALIDATE_INPUT_PRESET=Modern` and `OPENJKDF2_PERSIST_INPUT_PRESET=1` so the engine writes the real binding table and profile metadata; set `Window_displayMode = 1`, `Window_isFullscreen = true`, and `Window_defaultsVersion = 1` through the existing registry JSON API while preserving the saved Windowed dimensions. Refresh the desktop shortcut.

- [ ] **Step 6: Verify the installed handoff**

Resolve game data with the installed launcher's `-DiscoveryOnly -NoBrowse`, inspect the shortcut target, confirm the installed executable exists, confirm the profile says Modern/version 1, and launch only the desktop-safe Borderless path. Require no physical display-mode change.

- [ ] **Step 7: Record evidence and commit delivery metadata**

Write `docs/evidence/modern-defaults-2026-07-15.md` with commands, test totals, runtime observations, display invariants, package path/hash, installed paths, and any external policy blocker. Then run `git diff --check`, commit documentation and evidence, and verify a clean working tree.
