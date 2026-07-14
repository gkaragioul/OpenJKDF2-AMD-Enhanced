# Renderer diagnostics page — 2026-07-14

The production renderer diagnostics modal was opened from the normal menu rendering context on the representative AMD system. The harness captured the desktop-composited borderless window, dismissed the modal with its normal **OK** shortcut, and observed a clean process exit.

Command:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-diagnostics-page.ps1 -DataDir "D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight" -EvidenceRoot "runtime-evidence\diagnostics-page-2026-07-14-07"
```

Measured result:

- 2560x1440 capture with 7,722 visible samples and 581 page-specific red-text samples
- visible fields: OpenGL Core renderer, AMD Radeon RX 7900 XTX, ATI Technologies Inc., driver, OpenGL/GLSL API, primary core-profile fallback state, VSync, borderless 2560x1440 at 165 Hz, and Unlimited frame cap
- structured `diagnostics_page displayed=true` and `dismissed=true` events
- normal exit code 1 and clean run state
- legitimate Steam asset metadata unchanged
- Exclusive fullscreen was not invoked

The retained visual artifact is `runtime-evidence/diagnostics-page-2026-07-14-07/diagnostics-page.png`. Earlier harness attempts correctly were not accepted: `PrintWindow` returned a stale OpenGL frame, and an immediate compositor capture caught a transition frame. The final harness captures the actual desktop-composited window after a render-settle interval and requires a diagnostics-page-specific red-text signature.
