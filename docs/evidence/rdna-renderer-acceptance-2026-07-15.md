# RX 7900 XTX renderer acceptance -- 2026-07-15

## Scope

This closes the renderer acceptance path only for the tested AMD Radeon RX 7900 XTX, driver `32.0.31021.5001`, on Windows 11. It does not promote RDNA 1, RDNA 2, NVIDIA, or Intel from their explicit unverified-hardware states.

The authoritative upstream v0.9.9 binary had previously terminated after 18.666 seconds with Windows exception `0xC00000FD` in `atio6axx.dll`, before a playable frame. The current Release executable was tested through the hardware OpenGL path; no vendor name selects renderer behavior.

## Standards and source audit

The renderer contract is grounded in the Khronos [OpenGL 3.3 Core specification](https://registry.khronos.org/OpenGL/specs/gl/glspec33.core.pdf) and [GLSL 3.30 specification](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.3.30.pdf). The automated source audit passed these exact conditions:

- the resource manifest contains fourteen GLSL stages loaded as seven programs: default, menu, UI, texture-FBO, blur, SSAO, and SSAO mix;
- compile and link status plus normalized compiler/linker logs are always collected;
- framebuffer creation queries `glCheckFramebufferStatus` and requires `GL_FRAMEBUFFER_COMPLETE`;
- texture storage uses sized internal formats;
- anisotropy is enabled only through the queried extension;
- texture gather has a shader fallback based on extension availability;
- SDL context fallback tiers depend on successful API context creation;
- resource-initialization failure records the conservative compatibility fallback;
- no AMD, ATI, Radeon, NVIDIA, or Intel name appears in a renderer-selection conditional.

Reproduction:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\check-rdna-renderer-acceptance.ps1
```

Result: `PASS: renderer source matches the fourteen-stage, seven-program standards and capability contract.`

## Production method

The final Release x64 executable ran first-level gameplay for one minute at desktop-native 2560x1440 Borderless. The complete Ultra preset was applied live: filtering, 16x anisotropy, 0.5 mip bias, bloom, SSAO, 1.5x SSAA, texture precaching, and optional replacement discovery. An empty local metadata pack exercised original-asset fallback without adding third-party content.

```powershell
$captureRoot = Join-Path (Get-Location) `
  ('build\rdna-renderer-acceptance-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-enhancement-performance.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -UserDir $captureRoot `
  -DurationMilliseconds 60000 -TimeoutSeconds 100
```

The retained raw profile is ignored because normal gameplay creates a proprietary autosave. The committed JSON is a privacy-safe transcription with no local path or game asset.

## Measured result

- Capture: `2026-07-15T12:52:23.2493863Z` through `2026-07-15T12:53:32.0047634Z`
- Release SHA-256: `C64083B15FA2D039487992A1F2F2D7720D3BEB83856256FFE02BBA64CA0235B7`
- Process: historical success code `1`, clean run-state, ordinary `process_finished`
- Display: `2560x1440@165` before and after
- Steam asset metadata: invariant
- Windows Application Errors: `0`
- Shader compiles: `28` events, `14` distinct stages, `0` failures
- Shader links: `14` events, `7` distinct programs, `0` failures
- Framebuffers: `18` creation events, `0` incomplete
- Sampled texture uploads: `14`, GL errors `0`
- Diagnostic error severity: `0`
- Software renderer markers: `0`
- Exclusive fullscreen markers: `0`

| Pacing metric | Result |
|---|---:|
| Total recorded frames | 3,597 |
| Duration-scaled minimum | 3,240 |
| Median | 16.5683 ms |
| p95 | 18.6066 ms |
| p99 / worst | 22.9111 ms |
| Gate | pass |

Manual inspection of the full 2560x1440 capture found coherent first-level geometry, textures/materials, character rendering, lighting, and HUD with no obvious shader corruption. The screenshot remains ignored and is not packaged.

## Combined RX 7900 acceptance

The one-minute renderer run is combined with existing independent production evidence:

| Criterion | Evidence |
|---|---|
| Reproduce the prior failure | `upstream-baseline-2026-07-14.md` |
| Hardware 1440p renderer and sustained Ultra load | this report |
| Progress through the previously failing first door | `timing-invariance-2026-07-14.md` |
| 60/120 gameplay-domain invariance | `timing-domains-2026-07-14.md` |
| Responsive Modern and Classic controls | `control-presets-2026-07-14.md` |
| Save/load, death/reload, and restart | `save-load-lifecycle-2026-07-14.md`; `death-reload-2026-07-14.md` |
| Crash, forced termination, recovery, and display safety | `gameplay-crash-restoration-2026-07-14.md` |
| Asset-free package audit | `windows-package-2026-07-14.md` |

Together these records close `M3-RDNA` as a standards/capability implementation and `AC-RX7900` for the tested host. They do not prove every card, driver, campaign level, optional pack, multiplayer session, or full-campaign playthrough.

## Boundaries

- RDNA 1, RDNA 2, NVIDIA, and Intel hardware remain unverified.
- Full-campaign endurance remains unverified; the accepted scope combines the sustained renderer run with first-door and lifecycle probes.
- Exclusive fullscreen remains disabled and was not invoked.
- No driver, Windows security, HDR, gamma, color-profile, topology, or physical display-mode setting was changed.
