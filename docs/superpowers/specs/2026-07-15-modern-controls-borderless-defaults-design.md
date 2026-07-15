# Modern Controls and Borderless Defaults Design

## Goal

Make OpenJKDF2 AMD Enhanced start with familiar modern FPS controls and
desktop-native Borderless Fullscreen, while retaining an explicit Classic
control option and the existing fail-closed protection around Exclusive
Fullscreen.

## User-visible outcome

Modern is the default control style for every newly created player profile.
The default bindings are:

- W, A, S, and D for movement;
- mouse movement for looking, without acceleration or smoothing;
- left mouse for primary fire and right mouse for secondary fire;
- Space for jump;
- left or right Ctrl, plus C where the existing binding system permits an
  additional crouch binding, for crouch;
- left or right Shift for run;
- E for activate/use;
- mouse wheel for previous and next weapon;
- number keys for direct weapon selection;
- Tab for the map and Escape for the menu.

Right mouse must never remain bound to Jump after the Modern preset is
applied. The original bindings remain available through a saved `Control
Style: Modern / Classic` selector in Setup > Controls > Control Options.
Changing the selector replaces the current bindings only after the user
confirms the change, then immediately persists the selected style and binding
table in the current player profile.

The game starts in Borderless Fullscreen on the selected primary display at
that display's current desktop resolution and refresh rate. Setup > Display >
Display Options exposes these labels:

1. Windowed
2. Borderless Fullscreen (Recommended)
3. Exclusive Fullscreen

Exclusive Fullscreen remains visible but unavailable when the display
restoration guard cannot arm. Selecting or testing Exclusive must never bypass
that guard. Windowed and Borderless Fullscreen remain desktop-safe and never
request a physical display-mode change.

## Control preset model

`CONTROL_PRESET_MODERN` becomes the initial value, cvar default, reset value,
and new-profile binding set. New profile creation applies
`sithControl_ApplyModernPreset()` instead of the legacy base initializer.

The control-options page replaces the two independent Modern and Classic
buttons with one two-position selector and one Apply Preset action. Opening the
page reflects the current saved style. Cancel leaves both the bindings and
saved style unchanged. Applying Modern or Classic uses the existing preset
functions, refreshes HUD inventory input, writes the profile, and makes the
result active without restarting the game.

Manual changes made through Keyboard or Mouse settings remain supported. The
saved style identifies the preset that the customized layout was based on; it
does not cause the preset to be re-applied on every launch.

## One-time legacy migration

Profiles gain a `controlpresetversion` integer. Version `1` means the modern
default migration has been considered. On first load of an older profile:

- if its binding table exactly matches the complete stock Classic binding
  table, apply Modern, save `controlpreset = Modern`, and save
  `controlpresetversion = 1`;
- if any keyboard, mouse, axis, or joystick binding differs from the complete
  stock Classic table, preserve the binding table and current style, and save
  only `controlpresetversion = 1`.

The stock comparison must be order-insensitive and compare function, control,
mapping flags relevant to behavior, and axis scale. A partial "required keys"
check is insufficient because it could overwrite a customized profile.
Migration runs once and never re-applies a preset on later launches.

The currently installed `georgekgaming` profile is explicitly switched to
Modern during the package handoff, so it does not depend on detection of its
legacy state.

## Display defaults and migration

The existing recommended video defaults remain authoritative and continue to
specify `DISPLAY_MODE_BORDERLESS`. Fresh user data must therefore create
`Window_displayMode = Borderless` and `Window_isFullscreen = true`.

The global registry gains `Window_defaultsVersion = 1`. When an older registry
has no version and still contains the legacy stock Windowed geometry and mode,
startup migrates it once to Borderless Fullscreen. A non-stock Windowed size or
an already persisted Borderless/Exclusive choice is treated as deliberate and
preserved. The migration always records version `1` after making that decision.

The currently installed registry is explicitly set to Borderless Fullscreen
during package handoff. The last Windowed width and height remain stored so
switching back to Windowed restores the user's prior size.

Display Options keeps the existing monitor, resolution, effective mode, and
refresh information. Borderless Fullscreen locks resolution and refresh to the
selected display's desktop values. Exclusive Fullscreen remains an exact-mode
request that is accepted only after watchdog preflight and readiness succeed.

## Persistence and error handling

Profile and registry writes continue through the existing JSON and player
configuration paths. Unknown control preset values normalize to Modern.
Unknown display-mode values normalize to Borderless Fullscreen. If a display
change fails or is not confirmed, the existing display transaction restores
the previous settings and does not persist the rejected selection.

A failed profile migration leaves the original player file usable and reports
the failure through diagnostics. It must not repeatedly reset controls on
subsequent launches. Package installation and uninstall continue to preserve
user saves and configuration.

## Verification

Automated tests must prove:

- Modern is the control preset normalization, cvar, reset, and new-profile
  default;
- the full Modern binding table contains the documented keys and excludes
  right-mouse Jump, Space Activate, and Ctrl Primary Fire;
- the selector applies and persists both Modern and Classic;
- exact stock Classic profiles migrate once to Modern;
- any customized Classic binding prevents automatic binding replacement;
- recommended and clean-install display defaults are Borderless Fullscreen;
- legacy stock Windowed registry state migrates once, while deliberate
  Windowed state is preserved;
- Display Options uses the three user-facing labels and keeps Exclusive
  safety-gated;
- Windowed and Borderless tests leave the physical desktop mode unchanged;
- the packaged launcher, installed shortcut, game-data discovery, save/load,
  and uninstall acceptance tests continue to pass.

Runtime acceptance uses a fresh profile and the installed user profile. It
must observe Space Jump, right-click Secondary Fire, WASD movement, E Activate,
mouse look, Modern persistence after restart, Borderless desktop geometry, and
an unchanged Windows display mode. No Exclusive Fullscreen runtime test is
authorized by this design.

## Delivery

After Debug and Release verification, rebuild and verify the clean Windows x64
package, reinstall it to the per-user Programs directory, refresh the existing
desktop shortcut, migrate the current installed profile to Modern, set the
current installed display preference to Borderless Fullscreen, and run the
desktop-safe launch/discovery checks. The original Steam or GOG data remains
read-only and is never copied into the package.
