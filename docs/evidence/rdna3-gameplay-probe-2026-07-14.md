# RDNA 3 gameplay capture probe

Date: 2026-07-14 (Europe/Athens)

This is bounded evidence from one AMD Radeon RX 7900 XTX system. It does not
establish compatibility for other RDNA generations or complete the gameplay
acceptance matrix.

## Method

- Built the Release x64 target with the repository Windows build harness.
- Staged a private runtime root outside the Steam installation. Read-only
  directory junctions exposed the owner's `Episode`, `Resource`, `MUSIC`, and
  `Controls` directories; configuration, player data, diagnostics, and captures
  remained in the workspace runtime-evidence directory.
- Launched single-player `JK1` / `01narshadda.jkl` with desktop-sized borderless
  rendering and the opt-in validation capture timer.
- Captured the current Windows display mode through `EnumDisplaySettings`
  immediately before launch and after normal shutdown.
- Parsed the structured diagnostic log and run-state marker, then decoded the
  PNG with System.Drawing to verify its dimensions.
- Visually inspected the captured frame. The proprietary capture remains in
  ignored local evidence and is not committed or packaged.

## Result

- Renderer: hardware OpenGL; no software-renderer fallback was reported.
- Resolution: 2560x1440 gameplay capture.
- Window mode: borderless.
- Display before: 2560x1440 at 165 Hz.
- Display after: 2560x1440 at 165 Hz.
- All logged full-game shader compile and link events reported `ok=true`.
- Anisotropic filtering capability was reported with a maximum value of 16.
- The capture event was followed by `process_finished` and a clean run-state
  marker through the ordinary shutdown path.
- The inspected frame contained coherent first-level geometry, textures, and
  HUD with no obvious shader corruption in that frame. The AMD overlay was
  visible in the capture.

## Limits

The automatic capture occurred about five seconds after gameplay began. This
run did not reach the first door and did not validate save/load, death/reload,
level transitions, cutscenes, dialogue completion, frame pacing, 120 Hz
simulation behavior, Alt+Tab, crash restoration, multi-monitor behavior, or
exclusive fullscreen. Exclusive fullscreen remains gated because independent
watchdog restoration is not available on this host. Steam files were not
modified, but clean-install data-directory separation is still unfinished.
