# Safe Display Options Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Build an in-game Display Options submenu with safe mode, monitor, resolution, refresh selection, timed confirmation, and persistence.

**Architecture:** Add a pure DisplaySelection validator around the existing DisplaySettings DTO, expose SDL3 inventory/application behind Window.h, and have jkGUIDisplay edit a local proposal. Only confirmed transactions reach persisted per-user settings.

**Tech Stack:** C11, SDL3, jkGUI, CMake/CTest, PowerShell verification, Win32 restoration evidence.

## Global Constraints

- Windowed and Borderless never request physical resolution or refresh changes.
- Borderless uses the selected monitor's current desktop bounds.
- Exclusive requires the restoration guard and exact mode validation; do not runtime-test it.
- Never introduce GPU-vendor branches, persist hardware identifiers, write Steam data, or package proprietary assets.
- Claims require measured evidence; unavailable hardware remains unverified.

---

### Task 1: Pure display selection model

**Files:** Create src/General/DisplaySelection.h, src/General/DisplaySelection.c, src/Tests/test_display_selection.c; modify cmake/OpenJKDF2Tests.cmake.

**Interfaces:** Consume DisplaySettings. Produce DisplayInventory, DisplayMonitor, DisplayResolution, DisplaySelectionReason, DisplaySelectionResult, and display_selection_resolve(const DisplayInventory*, DisplaySettings, int).

- [ ] Write failing tests for stale monitor fallback, 640x480 minimum, maximum clamping, Borderless desktop normalization, Windowed refresh zero, exact Exclusive acceptance, guard rejection, and unsupported-mode rejection.
- [ ] Register the test and run the required Debug build; expect missing DisplaySelection symbols.
- [ ] Implement fixed-capacity inventory types (16 monitors, 256 modes each). Return accepted, normalized, reason, and resolved settings. Windowed clamps and clears refresh; Borderless adopts desktop values; Exclusive requires guard and an exact tuple.
- [ ] Run Debug tests; expect all pass. Commit: test: add safe display selection model.

### Task 2: SDL3 inventory and safe application

**Files:** Modify src/Win95/Window.h, src/Win95/Window.c, and src/Tests/test_display_selection.c.

**Interfaces:** Produce Window_GetDisplayInventory, Window_GetDisplayName, Window_GetDisplaySettings, Window_ApplyDisplaySettings, Window_CommitDisplaySettings, and Window_IsRestorationGuardReady.

- [ ] Add compile-contract calls for those signatures and run Debug tests; expect missing declarations.
- [ ] Enumerate via SDL_GetDisplays, SDL_GetPrimaryDisplay, SDL_GetDesktopDisplayMode, SDL_GetFullscreenDisplayModes, and SDL_GetDisplayBounds; deduplicate tuples and free SDL arrays.
- [ ] Track selected ordinal and Windowed size. Windowed moves/resizes without fullscreen APIs. Borderless uses selected desktop bounds without SDL_SetWindowFullscreen/Mode. Exclusive reaches fullscreen APIs only after validation and guard readiness.
- [ ] Query effective settings for snapshots and degrade enumeration failure to primary desktop only. Run Debug and diff checks. Commit: feat: add safe SDL display inventory.

### Task 3: Confirmed persistence

**Files:** Modify src/Win95/Window.c, src/World/jkPlayer.c, src/Tests/test_display_transaction.c, scripts/check-video-menu.ps1.

- [ ] Extend tests for all six fields and exact revert. Require persistence only after display_transaction_confirm; run Debug and contract to observe failure.
- [ ] Load Window_displayMonitor, Window_windowWidth, Window_windowHeight, Window_refreshHz at startup, normalize stale values, and preserve legacy fullscreen/HiDPI reads. Never persist SDL names or IDs.
- [ ] Write preferences only from Window_CommitDisplaySettings after confirmation. Run tests/contracts. Commit: feat: persist confirmed display preferences.

### Task 4: Dedicated submenu

**Files:** Modify src/Platform/SDL2/jkGUIDisplay.c, resource/ui/openjkdf2.uni, scripts/check-video-menu.ps1.

- [ ] Require localized Mode, Monitor, Resolution, Width, Height, Refresh, Effective Settings, Apply, Cancel, and unavailable Exclusive controls; run contract to observe failure.
- [ ] Replace fullscreen checkbox with Display Options. Use sliders/textboxes and UI-only names. Rebuild choices after monitor changes; lock Borderless fields; enable Windowed Custom; mark guarded Exclusive unavailable.
- [ ] Apply complete proposals: snapshot, validate/apply, recreate, confirm 15 seconds, commit on confirmation, otherwise restore exact snapshot. Cancel mutates nothing.
- [ ] Update localization count and shortcuts. Run contract/Debug. Commit: feat: add in-game display options submenu.

### Task 5: Runtime evidence and documentation

**Files:** Create scripts/test-display-options.ps1 and docs/evidence/display-options-2026-07-14.md; modify docs/display-safety.md, docs/evidence/requirements.csv, COMPATIBILITY.md, BENCHMARKS.md, CHANGELOG.md, TROUBLESHOOTING.md, FINAL-REPORT.md.

- [ ] Verify desktop width/height/refresh and Steam metadata before/after Windowed 1920x1080, Windowed 3840x2160 when permitted, primary Borderless, confirmed restart, and unconfirmed recovery. Enumerate real displays/rates, mark unavailable combinations unverified, and never invoke Exclusive.
- [ ] Inspect all evidence for zero desktop/Steam changes; document measured scope and limits. Commit: test: verify safe display options runtime.

### Task 6: Full verification and package refresh

