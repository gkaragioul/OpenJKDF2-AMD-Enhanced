# OpenJKDF2 AMD Enhanced technical report

## Scope and identity

This is a community fork of OpenJKDF2. It is not affiliated with or endorsed by
LucasArts, Disney, AMD, or the upstream OpenJKDF2 project. The distributable
package contains engine code and redistributable runtimes only; users supply a
legitimate Jedi Knight installation.

## Implemented systems

- Standards-compliant hardware OpenGL shader compilation, capability reporting,
  structured diagnostics, safe fallback selection, crash reports, and renderer
  smoke/runtime probes.
- Arbitrary-resolution presentation with independent gameplay, menu, HUD, and
  video aspect policies.
- Safe Windowed and desktop-native Borderless modes, SDL display enumeration,
  multi-monitor selection, confirmation-only persistence, timed reversion, safe
  startup fallback, and Exclusive safety gating.
- Frame-cap choices from 30 through 240 FPS plus Desktop Refresh and Unlimited,
  VSync Off/On/Adaptive behavior, high-resolution pacing telemetry, and fixed-step
  gameplay timing protections.
- Relative/raw mouse behavior, independent axes, optional smoothing/acceleration,
  and Modern/Classic control presets.
- Modular quality presets and optional asset overlays with original assets as the
  guaranteed fallback.
- Per-user/portable storage, Steam discovery, asset validation, legal package
  auditing, safe uninstall, checksums, focused tests, and Windows build scripts.

## Measured results

The requirements ledger at `docs/evidence/requirements.csv` is authoritative.
Key evidence includes:

- RX 7900 XTX hardware-rendered first-level and first-door probes without the
  prior shader failure or driver crash.
- Correct 2560x1440 Borderless gameplay and aspect-correct HUD/menu/video captures.
- 120 FPS frame-cap-only median 8.3232 ms and p95 8.5754 ms; 60/120 first-door
  traversal differed by 7 ms.
- Production save/load, death/reload, restart, and modern mouse/movement probes.
- Exact live Windowed/Borderless sizes at 1080p, 1440p, 4K, and a second monitor,
  with 2560x1440@165 desktop and 75 Steam asset metadata entries invariant.
- Three focus-loss/reacquisition cycles released mouse capture, and forced
  termination restored the exact virtual-desktop cursor clip while preserving
  the 2560x1440@165 desktop mode.
- A real gameplay access-violation crash generated a 3,949-byte DrMinGW report;
  desktop/cursor state remained invariant and the accepted last-known-good
  recovery prompt led to a clean rendered relaunch.
- Three presentation modes produced complete framebuffer status, sampled
  zero-error texture uploads, and initial swap-state telemetry in structured logs.
- A clean-provenance asset-free Windows package and uninstall preservation test.

## Unresolved limitations

- RDNA 1, RDNA 2, NVIDIA, and Intel hosts were unavailable and remain unverified.
- Physical 60, 120, and 144 Hz display modes were unavailable on the tested host;
  165 Hz desktop behavior and software frame caps were measured.
- Exclusive Fullscreen remains disabled because the independent restoration
  watchdog has not passed host-authorized forced-restoration testing.
- Live display-change timed-dialog interaction, broader
  campaign/cutscene/dialogue/transition coverage, real GOG discovery, and a
  separate clean Windows machine remain incomplete in the requirements ledger.

These limitations are reported rather than inferred away. The safe supported
fallback is Windowed or Borderless with the conservative renderer/settings.

## Reproduction and artifacts

Build Debug or Release exactly as documented in `docs/building-windows.md`.
Evidence methods and commands live under `docs/evidence/`; automation is under
`scripts/`. Package contents, provenance checks, install/uninstall results, and
the current artifact checksum are recorded in
`docs/evidence/windows-package-2026-07-14.md`.
