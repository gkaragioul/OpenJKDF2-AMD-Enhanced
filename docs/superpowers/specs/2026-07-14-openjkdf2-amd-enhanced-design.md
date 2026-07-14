# OpenJKDF2 AMD Enhanced Design

## Purpose

OpenJKDF2 AMD Enhanced is a maintainable Windows 11 source-port fork that makes the existing OpenJKDF2 SDL/OpenGL renderer observable, standards-compliant, and reliable on modern Radeon RDNA hardware while preserving original game behavior and save compatibility. It reads legally owned Steam or GOG game data from a configurable external directory and never redistributes proprietary assets.

The first supported target is the local Radeon RX 7900 XTX at 2560x1440 and 165 Hz. Results from that machine prove only the tested RDNA 3 configuration. RDNA 1, RDNA 2, NVIDIA, and Intel remain explicitly unverified until matching hardware evidence exists.

## Project boundaries

- Preserve upstream Git history and attribution. Development occurs on `amd-enhanced/main` from upstream commit `a189787a6180c3132c4a736da0835df046f40c77`.
- Keep builds, logs, settings, saves, reports, and packages outside the Steam installation.
- Never modify, move, delete, or package original Jedi Knight assets.
- Never change system-wide HDR, color profiles, gamma, driver settings, display topology, or security controls.
- Do not test exclusive fullscreen until the display-restoration safeguard and its independent tests pass.
- Do not infer support from GPU names. Runtime paths depend on API versions, extensions, formats, limits, and successful self-tests.
- Every compatibility or performance statement links to structured diagnostics, test output, or frame-time captures.

## Delivery model

Work is split into independently verifiable stages. Each stage leaves a usable, reviewable artifact and a requirements-evidence update.

1. Repository, environment, build, and evidence foundation.
2. Diagnostics, shader validation, crash reporting, smoke mode, safe mode, and automated tests.
3. Portable OpenGL corrections and capability-based renderer fallbacks.
4. Display-state transaction and restoration watchdog.
5. Arbitrary resolution, aspect handling, borderless/windowed modes, and display UI.
6. Fixed simulation cadence, interpolated rendering, frame limiter, VSync modes, and frame-rate UI.
7. Raw mouse input and modern/classic control presets.
8. Modular enhancement options and diagnostics UI.
9. Hardware validation, packaging, clean-install testing, and final evidence report.

The project stays on the existing SDL/OpenGL backend while evidence shows that portable corrections are feasible. A D3D11 backend becomes a separately reviewed design only if captured shader/compiler/API evidence demonstrates that the OpenGL route cannot meet acceptance criteria without vendor-specific undefined behavior or driver workarounds.

## Architecture

### Platform services

Windows/SDL platform code owns window creation, DPI awareness, focus, cursor capture, swap policy, and display-state transactions. New platform-neutral interfaces expose display modes and frame timing without leaking SDL objects into gameplay code.

`DisplayState` is an immutable snapshot of the selected monitor, desktop bounds, refresh rate, window placement, gamma ramp identity, and mode. A risky change follows prepare, apply, confirm, and rollback states. Borderless mode uses a desktop-sized borderless window and never calls a physical display-mode switch. Exclusive mode is disabled until the watchdog is armed. The watchdog stores only restoration metadata, monitors a heartbeat, and restores the captured mode if the game exits abnormally.

### Diagnostics and recovery

A structured logger writes human-readable text and newline-delimited JSON into a per-user diagnostics directory. Events have timestamp, severity, subsystem, stable event name, and typed fields. The machine-readable report excludes usernames, absolute asset paths, save contents, and proprietary data; paths are reduced to source type and validation status.

Renderer startup records OS build, GPU/driver strings reported by OpenGL, API and GLSL versions, extensions used, limits, framebuffer formats, swap policy, and selected fallback. Shader failure events include logical shader name, stage, source hash, compiler log, and fallback result. They do not dump proprietary assets.

Windows unhandled exceptions produce a minidump and final structured crash event. On startup, an unclean-run marker triggers a safe-mode offer. Safe mode forces windowed rendering, 60 FPS, conservative OpenGL features, original assets, no post-processing, and a known-good configuration. Configuration writes are atomic; the previous successful configuration is retained separately.

### Renderer

The renderer is corrected against the OpenGL 3.3 core and GLSL 3.30 specifications. Shader sources receive offline validation in CI and runtime compile/link checks. Uniforms and attributes use explicit locations or queried bindings consistently. Textures and framebuffers use sized internal formats, validated dimensions, initialized storage, and completeness checks. No shader depends on uninitialized values, implicit narrowing, out-of-range indexing, or sampler/attachment aliasing.

`RendererCapabilities` is populated from queried functionality and active probes. Optional effects declare required capabilities and deterministic fallback order. Startup chooses the highest validated feature set, records the reason for every disabled feature, and can fall back to a conservative OpenGL path without falling back to software rendering.

The smoke-test mode creates a hidden or small window without changing display mode, compiles every shader, uploads representative generated textures, creates each framebuffer class, renders deterministic generated geometry, reads back hashes, and exits with a machine-readable result. It requires no game assets.

### Resolution and presentation

Logical gameplay projection, render-target size, viewport, UI canvas, videos, and mouse coordinates are distinct values. Gameplay supports arbitrary positive desktop resolutions. A configurable aspect policy controls vertical or horizontal FOV behavior. HUD and menus retain authored proportions using safe-area anchors and independent scale. Four-by-three videos use pillarboxing by default, with explicit stretch and crop alternatives.

The default mode is borderless at current desktop resolution and refresh. Windowed mode uses a user-selected client size. Exclusive mode is opt-in and confirmation-timed. Alt+Tab and focus loss release input immediately and suspend risky presentation changes without changing the desktop.

### Timing

Simulation and rendering use separate clocks. The simulation advances at a verified fixed tick compatible with original doors, AI, scripts, physics, animations, dialogue, cutscenes, and multiplayer. Rendering interpolates between simulation states. Accumulated time is bounded after pauses or debugging to prevent a catch-up spiral.

The frame selector supports 30, 40, 50, 60, 72, 90, 100, 120, 144, 165, 180, 200, 240, desktop refresh, unlimited, and direct numeric configuration. A high-resolution deadline scheduler sleeps for the coarse interval and uses only a short bounded final yield/spin, avoiding full-core busy waiting. Frame-time telemetry records CPU frame start, simulation duration, render duration, present duration, target, lateness, and dropped simulation time.

VSync modes are Off, On, Adaptive when the driver/API supports it, and frame-cap-only. Unsupported modes remain visible with an explanation and a safe fallback. No timing setting silently changes; detected instability offers an explicit one-click reset to 60 FPS.

### Input

SDL relative mouse input is the portable default. On Windows, raw input is enabled when available and verified, with SDL relative input as fallback. Deltas are accumulated independently of render resolution and scaled by horizontal/vertical sensitivity. Smoothing defaults off; acceleration is optional. Focus changes, pause, crash handling, and shutdown always release capture.

Modern and Classic presets are named collections of bindings, not gameplay-rule changes. Applying a preset changes only controls and remains individually editable. Mouse wheel weapon selection is event-based so high render rates do not multiply input.

### Configuration and files

Settings and saves live below a stable per-user application-data directory. Portable mode is activated only by an explicit command-line flag or marker next to the executable. A configurable read-only `data_dir` points to Steam or GOG assets. First run probes common install locations, validates required files by name and structural readability, and otherwise offers a browse flow.

Every in-game setting has a documented configuration key. Video changes use Apply, timed Confirm, and Revert. Startup parses the active file into a validated model, then merges defaults without losing recognized user choices. Package upgrades preserve saves and settings.

## Testing and evidence

Pure logic is extracted behind small C interfaces and tested without game data: settings parsing, frame-cap selection, resolution/aspect calculations, save/config paths, fixed-step accumulation, limiter deadlines, capability/fallback selection, display transaction state, last-known-good recovery, and asset validation.

Renderer smoke tests use generated data and compare structured outcomes rather than assuming one vendor-specific pixel hash. Display tests mock the SDL boundary first. Hardware tests snapshot desktop state before and after launch, Alt+Tab, injected renderer failure, clean exit, and forced termination. No exclusive test runs before watchdog unit and process-level restoration tests pass.

Gameplay acceptance covers new game, first door, continued play past the prior crash point, save/load, death/reload, level transition, cutscenes, dialogue, menus, and exit. Timing validation compares recorded event durations at 60 and 120+ render rates to prove simulation invariance. Frame pacing is reported with median, 95th, 99th percentile, worst frame, missed-deadline count, and capture duration.

The compatibility matrix records exact GPU, architecture, driver, Windows build, backend, resolution, refresh, mode, cap, result, and evidence path. Hardware not physically or remotely tested is marked unverified.

## Packaging and legal safety

The Windows x64 portable ZIP is the primary reproducible artifact; an installer may wrap the same manifest. Packaging uses an explicit allowlist and an automated proprietary-asset scan. It includes binaries, open-source runtime dependencies, licenses, attribution, configuration examples, changelog, troubleshooting, checksums, and source/build provenance. It excludes game data, user paths, logs, saves, and enhancement packs.

The uninstaller removes only manifest-owned program files. Saves and settings survive unless the user explicitly selects their removal. The fork is described as an independent community project with no AMD, LucasArts, Disney, or upstream endorsement.

## Completion rule

No milestone or final requirement is complete because code exists or a test command returns zero. Each requirement must have authoritative evidence in the tracked matrix. RX 7900 XTX acceptance additionally requires a hardware OpenGL run at 2560x1440 borderless, no corruption or driver crash, successful first-door progression, invariant gameplay timing, responsive input, save/load coverage, frame-time capture at the selected cap, package-content audit, and verified display restoration after every required exit path.
