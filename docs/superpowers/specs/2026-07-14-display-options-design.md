# Safe Display Options Design

Date: 2026-07-14

## Objective

Add a dedicated in-game Display Options submenu that lets players select the
window mode, monitor, resolution, and refresh behavior without relying on an
external utility. Windowed and Borderless modes must never change the physical
display mode. Exclusive Fullscreen remains unavailable unless the existing
restoration guard reports that it is ready, and it is excluded from runtime
testing until restoration has been independently verified.

The feature must remain vendor-neutral. AMD compatibility is achieved through
standards-compliant SDL behavior and the existing renderer abstraction, not
through GPU-vendor branches.

## User Experience

The existing Display menu gains a "Display Options..." entry. The submenu
contains:

- **Mode:** Windowed, Borderless, or Exclusive Fullscreen. Exclusive is visibly
  marked unavailable whenever the restoration guard is not ready. Selecting an
  unavailable mode does not silently substitute another mode.
- **Monitor:** A session-local ordinal and SDL-provided display name, such as
  "1 — Display Name". Display names are for the interactive UI only and are not
  written to logs, diagnostics, or configuration.
- **Resolution:** Enumerated resolutions for the selected monitor, deduplicated
  by width and height, plus a Custom choice. Custom width and height fields are
  editable in Windowed mode. Borderless always shows the selected monitor's
  desktop resolution as locked. Exclusive accepts only an exact enumerated
  mode.
- **Refresh behavior:** "Desktop / Compositor" for Windowed and Borderless.
  Exclusive may select an enumerated refresh rate for its chosen mode. The
  control is read-only where the application cannot safely request a refresh
  rate.
- **Effective settings summary:** A concise line showing the resolved monitor
  ordinal, dimensions, and effective refresh behavior.
- **Apply** and **Cancel:** Apply uses the existing 15-second confirmation
  transaction for every mode, monitor, resolution, refresh, or HiDPI change.
  Cancel closes the submenu without changing runtime or persisted state.

The menu must remain keyboard, mouse, and controller navigable using the
project's existing GUI conventions.

## Display Model

The existing DisplaySettings structure remains the transaction data transfer
object with mode, monitor, width, height, refresh_hz, and hidpi fields.

A small SDL-independent General/DisplaySelection module owns validation and
normalization. It accepts an inventory of monitors and modes supplied by the
platform layer and returns either a resolved DisplaySettings value or a
specific validation error.

Rules:

1. A stale or invalid monitor ordinal resolves to the primary monitor and
   records a non-personal normalization reason.
2. Width and height must each be positive, fit the renderer's integer limits,
   and be at least 640 by 480. Windowed custom sizes are clamped to safe bounds.
3. Borderless resolves width, height, and refresh from the selected monitor's
   current desktop mode. User-entered dimensions and refresh values cannot
   override them.
4. Windowed preserves the selected client dimensions and uses compositor
   refresh behavior. It cannot request a physical display mode.
5. Exclusive requires a ready restoration guard and an exact mode from the
   selected monitor's enumerated inventory. Otherwise validation rejects the
   request without changing the window.
6. Resolution choices are regenerated when the selected monitor changes.
   A stale selection resolves to that monitor's desktop resolution for
   Borderless or the last valid safe Windowed size for Windowed.

## Platform Inventory and Application

The window abstraction gains read-only display inventory functions and one
settings application entry point. The SDL implementation uses SDL3 display
enumeration and mode APIs internally while keeping SDL types out of the menu
and validation layers.

Required capabilities:

- enumerate displays and identify the primary display;
- obtain a UI-only display name;
- obtain desktop bounds and desktop mode;
- enumerate fullscreen modes;
- query the window's effective display settings; and
- apply a validated DisplaySettings value.

Application behavior is mode-specific:

- **Windowed:** remove borderless/fullscreen state, move the window to the
  selected display, and set its client size. Do not call any API that changes
  the display mode.
- **Borderless:** use the selected display's desktop bounds, position the
  window at those bounds, and create a borderless desktop-sized window. Do not
  set a fullscreen display mode and do not request a refresh rate.
- **Exclusive:** enter only through the restoration guard, snapshot the
  pre-change state, select the exact validated SDL mode, and keep the watchdog
  and last-known-good recovery paths active.

Display enumeration failure degrades to a single primary-display entry using
the current desktop mode. It must not leave the menu unusable or invent support
for unavailable modes.

## Persistence and Compatibility

Persist only confirmed settings in the existing per-user configuration:

- display mode;
- monitor ordinal;
- Windowed width and height;
- refresh behavior and requested Exclusive refresh rate; and
- HiDPI preference.

The existing legacy fullscreen and HiDPI keys continue to be read for backward
compatibility. New keys take precedence when valid. Missing or stale values
normalize through the same pure model used by the menu.

Monitor ordinals are intentionally treated as preferences rather than stable
hardware identity. If topology changes, the primary monitor is the safe
fallback. Raw display names, EDID data, serial numbers, adapter identifiers,
and other potentially identifying hardware strings are never persisted.

The current settings become persistent only after the user confirms the timed
Apply transaction. Revert, timeout, failed application, or process recovery
retains the previous confirmed settings.

## Failure and Recovery Behavior

Before applying a change, capture the current effective settings. If validation
or SDL application fails:

1. immediately restore the captured settings;
2. retain the previous persisted settings;
3. show a concise user-facing failure message; and
4. write a structured diagnostic event containing only mode, monitor ordinal,
   dimensions, refresh value, result, and a non-personal reason code.

If the new settings apply successfully, the existing 15-second confirmation
dialog begins. Timeout or explicit Revert restores the snapshot. Confirmation
commits the new settings as last-known-good.

Startup continues to use the existing last-known-good and safe fallback paths.
An invalid saved configuration must not prevent the game from opening in a
safe Windowed mode.

## Safety Invariants

- Windowed and Borderless never change physical resolution, refresh rate,
  gamma, HDR state, color profile, display topology, or operating-system
  settings.
- Borderless always matches the selected monitor's current desktop mode.
- Exclusive never starts through silent fallback or an unvalidated mode.
- No behavior branches on AMD, NVIDIA, Intel, or another GPU vendor.
- Display names and stable hardware identifiers do not enter machine-readable
  diagnostics or persisted configuration.
- Unsupported multi-monitor or refresh combinations are reported as
  unverified, not inferred.
- Exclusive runtime testing remains prohibited until restoration behavior has
  been independently verified.

## Verification Strategy

### Unit tests

The SDL-independent model is tested first for invalid and stale monitor
ordinals; minimum, maximum, and malformed Windowed dimensions; Borderless
resolution and refresh normalization; stale selections after a monitor change;
exact-mode validation for Exclusive; restoration-guard rejection; transaction
difference detection for every field; and confirmed, reverted, timed-out, and
failed persistence outcomes.

### Contract and integration tests

Contract tests verify that the Display menu exposes the submenu and all
controls; changing a monitor rebuilds its resolution choices; locked controls
cannot mutate Borderless physical-mode fields; Apply passes every selected
field into the transaction; Cancel makes no change; only confirmation writes
new persisted values; and legacy configuration still loads through
normalization.

### Runtime verification

Runtime verification uses the production settings path and captures evidence
for:

- Windowed 1920 by 1080;
- Windowed 3840 by 2160 where renderer and desktop bounds permit it;
- Borderless on the primary 2560 by 1440 desktop;
- monitor enumeration and selection on every physically available monitor;
- effective resolution, refresh, and mode diagnostics;
- unchanged Steam installation metadata and display configuration; and
- clean restart from confirmed settings and recovery from an unconfirmed
  change.

Refresh claims at 60, 120, 144, or 165 Hz are made only where the real display
or compositor reports and demonstrates that behavior. Frame-cap validation is
reported separately from physical refresh validation. If the test machine has
only one monitor, multi-monitor behavior remains explicitly unverified.

No Exclusive Fullscreen runtime test is part of this work.

## Out of Scope

- A new renderer backend.
- GPU-vendor-specific paths or driver workarounds.
- Operating-system display configuration.
- HDR, gamma, color-profile, or topology management.
- An external launcher or settings utility.
- Claims for display hardware that is not physically available to verify.
