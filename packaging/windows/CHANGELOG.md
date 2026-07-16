# Changelog

## 2026-07-15

- Made Modern the default control style for new player profiles: WASD and mouse
  look, left-click primary fire, right-click secondary fire, Space jump,
  Ctrl/C crouch, Shift run, E use, mouse-wheel weapon selection, number-key
  weapon selection, Tab map, and Escape menu.
- Added a saved Modern/Classic Control Style selector under
  **Setup > Controls > Control Options**. Untouched legacy Classic profiles are
  migrated once; customized legacy bindings are preserved.
- Made Borderless Fullscreen the clean-install and stock-settings default.
  Display Options now clearly offers Windowed, Borderless Fullscreen
  (Recommended), and safety-gated Exclusive Fullscreen.

## 2026-07-14

- Added standards-compliant AMD/RDNA renderer diagnostics and fallbacks.
- Added safe borderless/windowed display handling and restoration safeguards.
- Added 2560x1440/arbitrary-resolution layout support and video settings.
- Added configurable frame caps, pacing telemetry, and 60/120 timing evidence.
- Added Modern and Classic controls, raw/relative mouse options, and safe storage.
- Added read-only external game-data overlay, portable mode, packaging, and tests.
- Added an in-game Display Options submenu with monitor, Windowed/Borderless mode,
  arbitrary resolution, effective refresh reporting, timed confirmation, and
  confirmation-only per-user persistence. Exclusive mode remains safety-gated.
