# Foundation verification

Date: 2026-07-14 (Europe/Athens)

The Windows x64 foundation was verified without launching the game or reading
proprietary assets. Debug and Release are built with MSVC 19.44 and Ninja from
the recursively pinned source tree. Unit tests cover C11 portability,
privacy-safe structured logging, startup parsing and safe-mode defaults, typed
shader metadata and SHA-256, and deterministic diagnostic reports.

## RX 7900 XTX renderer smoke

The explicit smoke executable creates only a hidden 64×64 OpenGL 3.3 core
window. It uses generated shaders and geometry, compiles and links both stages,
creates an RGBA8 framebuffer, draws a triangle, and validates pixel readback.
It contains no fullscreen, display-mode, gamma, HDR, color-profile, or topology
calls.

- Result: exit code 0.
- Vendor: ATI Technologies Inc.
- Renderer: AMD Radeon RX 7900 XTX.
- OpenGL: 3.3.0 Core Profile Context 26.6.4.260624.
- GLSL: 4.60.
- Shader compile/link: passed.
- Framebuffer/draw/readback: passed.
- Display before: 2560×1440 at 165 Hz.
- Display after: 2560×1440 at 165 Hz.
- Run marker: clean.

This proves the generated-resource OpenGL path on this single RDNA 3 machine.
It does not prove full-game correctness, RDNA-wide compatibility, borderless
behavior, frame pacing, or the first-door crash fix. Those remain explicitly
incomplete in `requirements.csv`.

## Borderless creation smoke

The same generated-resource test was repeated with a hidden borderless window
sized from the primary display's current bounds. SDL reported the borderless
flag and exact desktop geometry, rendering/readback passed, and the display was
2560×1440 at 165 Hz both before and after. This path contains no fullscreen or
display-mode call. Full gameplay and multi-monitor borderless validation remain
pending.
