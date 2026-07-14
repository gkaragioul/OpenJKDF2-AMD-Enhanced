# Architecture decisions

## Scope

OpenJKDF2 AMD Enhanced is a focused Windows 11 community fork built on the
upstream OpenJKDF2 SDL/OpenGL engine. It retains upstream history and the
function-by-function engine structure instead of placing a translation wrapper
around the executable. The fork does not include or modify proprietary game
assets; a read-only data directory supplies them at runtime while configuration,
saves, logs, screenshots, and optional enhancement files live in a separate
user directory.

## Renderer

The renderer remains OpenGL 3.3 because the measured failure was addressable in
the existing backend and did not justify a Vulkan or Direct3D rewrite. Shader
compilation is standards-oriented: every stage has a stable name and content
hash, compiler/link logs are captured, framebuffer completeness is checked, and
texture-upload errors are sampled. Optional effects are selected from queried
capabilities and extensions, never from a GPU-name allowlist.

Capability evaluation selects the normal hardware path when required features
exist. Missing optional features disable only the affected enhancement. Safe
mode forces conservative settings and Windowed presentation; original assets
remain the guaranteed content fallback. The in-game diagnostics page exposes
the actual renderer, GPU, driver/API, selected fallback, VSync, display mode,
refresh, and frame cap.

## Display lifecycle

Presentation is modeled as Windowed, desktop-native Borderless, or gated
Exclusive. Borderless uses the current desktop mode and never calls an OS mode
switch. Resolution/aspect policy is separate from physical display selection so
gameplay, menus, HUD, and video can each preserve their intended geometry.

Risky settings use a transaction: apply in memory, recreate the SDL window,
commit only after confirmation, or restore the exact prior settings on cancel or
timeout. Startup snapshots and last-known-good recovery protect configuration.
The independent Windows restoration helper owns the fail-safe path required
before Exclusive can be enabled. Until that helper is independently proven to
apply display state on the host, Exclusive remains unavailable.

## Storage and packaging

`--data-dir` is the immutable licensed-asset root. `--user-dir` is the writable
overlay for settings, profiles, saves, logs, and optional packs. Portable mode
selects an explicit package-local user root. Package assembly starts from a clean
manifest, allows only redistributable engine/runtime files, scans for prohibited
asset extensions and known game paths, and emits SHA-256 checksums. Uninstall
removes package-owned files and shortcuts but preserves game data and user data
unless the user explicitly requests otherwise.

## Diagnostics and validation

Structured JSON Lines events mark renderer initialization, shaders, textures,
framebuffers, swap behavior, display/focus transitions, input capture, timing,
and controlled validation routes. Runtime validation hooks are opt-in through
`OPENJKDF2_VALIDATE_*` variables and are inert in normal play. They exercise
production code paths and write evidence only outside the game-data tree.
