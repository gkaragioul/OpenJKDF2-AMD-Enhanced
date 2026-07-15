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
  startup fallback, and a packaged fail-closed restoration watchdog that gates
  Exclusive behind an exact-snapshot preflight and process handshake.
- Frame-cap choices from 30 through 240 FPS plus Desktop Refresh and Unlimited,
  VSync Off/On/Adaptive behavior, high-resolution pacing telemetry, and fixed-step
  gameplay timing protections.
- Relative/raw mouse behavior, independent axes, optional smoothing/acceleration,
  and Modern/Classic control presets.
- Modular quality presets and optional asset overlays with original assets as the
  guaranteed fallback.
- Per-user/portable storage, Steam/GOG discovery, browse validation, legal
  package auditing, safe uninstall, checksums, and Windows build/test scripts.

## Measured results

The requirements ledger at `docs/evidence/requirements.csv` is authoritative.
Key evidence includes:

- RX 7900 XTX one-minute 2560x1440 Ultra hardware run recorded 3,597 frames;
  all 14 GLSL stages and 7 programs passed, all 18 framebuffers were complete,
  and no texture-upload error, software fallback, crash, or obvious corruption
  was observed. Independent evidence also covers the previously failing first door.
- Correct 2560x1440 Borderless gameplay and aspect-correct HUD/menu/video captures.
- 120 FPS frame-cap-only median 8.3232 ms and p95 8.5754 ms; nine production
  gameplay domains had a maximum 60/120 simulation-median delta of 9 ms.
- Production save/load, death/reload, restart, and modern mouse/movement probes.
- Modern and Classic production control presets passed complete live binding
  validation plus actual movement and mouse-turn gameplay probes.
- Exact live Windowed/Borderless sizes at 1080p, 1440p, 4K, and a second monitor,
  with 2560x1440@165 desktop and 75 Steam asset metadata entries invariant.
- Three focus-loss/reacquisition cycles released mouse capture, and forced
  termination restored the exact virtual-desktop cursor clip while preserving
  the 2560x1440@165 desktop mode.
- A real gameplay access-violation crash generated a 3,949-byte DrMinGW report;
  desktop/cursor state remained invariant and the accepted last-known-good
  recovery prompt led to a clean rendered relaunch.
- The production watchdog contract passed armed-state/PID readiness, disarmed
  parent-exit proof, and disarmed-state rejection. A clean Borderless run logged
  `preflight_failed` with Win32 results `5/-1`, disarmed cleanly, and preserved
  `2560x1440@165`; actual armed restoration remains blocked by host authorization.
- Three presentation modes produced complete framebuffer status, sampled
  zero-error texture uploads, and initial swap-state telemetry in structured logs.
- The live Windows display-change dialog was captured and left untouched; after
  16.866 seconds it automatically restored the exact Borderless settings while
  preserving the 2560x1440@165 desktop.
- A clean-provenance 37-file Windows package passed packaged Steam/GOG/browse
  fixtures, real automatic Steam configuration, installed gameplay, two-process
  save restoration, display/asset invariance, uninstall, and user-data
  preservation with zero proprietary findings.

## Unresolved limitations

- RDNA 1, RDNA 2, NVIDIA, and Intel hosts were unavailable and remain unverified.
- Physical 60, 120, and 144 Hz display modes were unavailable on the tested host;
  165 Hz desktop behavior and software frame caps were measured.
- Exclusive Fullscreen remains disabled on the tested host. The watchdog is
  production-integrated and packaged, but `SetDisplayConfig` denied exact
  snapshot reapplication with error 5 and `ChangeDisplaySettingsEx` failed
  with -1, so armed forced-restoration testing is blocked rather than claimed.
- A full-campaign endurance run and multiplayer hardware session remain
  unverified.
- A real GOG installation and human folder-dialog session were unavailable;
  automated GOG common-location and browse behavior passed asset-free fixtures.
- Windows Sandbox, Hyper-V, and a second fresh Windows host were unavailable, so
  the separate-machine acceptance boundary is recorded as a host blocker.
- The final Debug gate passed 45/45 and an earlier exact Release gate passed
  45/45. On the last Release rerun, 44 passed and enterprise Windows Code
  Integrity blocked the unsigned `test_video_defaults.exe` before startup; this
  was a policy launch denial, not a failed assertion.

These limitations are reported rather than inferred away. The safe supported
fallback is Windowed or Borderless with the conservative renderer/settings.

## Reproduction and artifacts

Build Debug or Release exactly as documented in `docs/building-windows.md`.
Architecture and renderer fallback decisions are in `docs/architecture.md`, and
the render/simulation clock design is in `docs/timing-design.md`.
Evidence methods and commands live under `docs/evidence/`; automation is under
`scripts/`. Package contents, provenance checks, install/uninstall results, and
the current artifact checksum are recorded in
`docs/evidence/windows-package-2026-07-14.md`. Watchdog lifecycle results and
the host authorization blocker are in
`docs/evidence/display-watchdog-2026-07-15.json`. Discovery and installed
acceptance results are in
`docs/evidence/discovery-clean-install-2026-07-15.json`.
