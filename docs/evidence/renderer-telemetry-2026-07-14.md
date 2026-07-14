# Renderer telemetry verification — 2026-07-14

The Release renderer was exercised through three production presentation runs
(VSync Off, On, and Adaptive) at desktop-native 2560x1440 Borderless. The
source contract `renderer_telemetry_contract` requires structured framebuffer,
texture-upload/failure, and swap events.

Runtime evidence root:
`runtime-evidence/renderer-telemetry-2026-07-14-01` (ignored because it includes
proprietary gameplay captures).

Measured across the three fresh processes:

- `framebuffer_created`: 12 events; scene status `0x8cd5`
  (`GL_FRAMEBUFFER_COMPLETE`)
- sampled `texture_upload`: 42 events with dimensions, source format, alpha,
  upload count, and `gl_error=0x0`
- `texture_upload_failed`: 0 events
- `swap_started`: 3 events with mode, resolution, and initial applied VSync
- presentation pacing: Off/120, On/60, and Adaptive/60 all passed
- display: `2560x1440@165` before and after
- Steam asset metadata: invariant

Texture telemetry is intentionally sampled for successful uploads (first eight
and power-of-two counts) while failures are always logged, preventing ordinary
game logs from becoming unbounded.
