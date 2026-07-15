# RDNA Renderer Acceptance Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the standards-oriented renderer and close the tested RX 7900 XTX acceptance path without claiming unavailable hardware.

**Architecture:** Keep OpenGL 3.3 and validate all seven production shader programs, capability-driven context and anisotropy paths, framebuffer formats, and resource telemetry. A Windows PowerShell-compatible launcher collects a one-minute Ultra run; strict source and runtime contracts reject shader, texture, framebuffer, software-renderer, driver-crash, display, asset, or pacing failures.

**Tech Stack:** C11, OpenGL 3.3/GLSL 3.30, GLEW, SDL, CMake/CTest, Windows PowerShell 5.1, JSON Lines.

## Global Constraints

- Never test Exclusive fullscreen; it remains safety-gated.
- Read the Steam asset tree without modifying or packaging it.
- Treat only the RX 7900 XTX / driver `32.0.31021.5001` / Windows 11 host as hardware-verified.
- Keep RDNA 1, RDNA 2, NVIDIA, and Intel `unverified-hardware`.
- Select paths from API/context creation, extensions, formats, and runtime results, never GPU names.
- Commit no raw profile, save, screenshot, log, private path, or proprietary asset.
- Require exact Debug and Release Windows gates.

---

### Task 1: Windows PowerShell-compatible validation launchers

**Files:**
- Create: `scripts/check-powershell-compatibility.ps1`
- Modify: `cmake/OpenJKDF2Tests.cmake`
- Modify: every validation launcher that indexes `ProcessStartInfo.Environment`.

**Interfaces:**
- Consumes: process-scoped `OPENJKDF2_*` variables.
- Produces: CTest `powershell_harness_compatibility` and launchers resilient to duplicate `Path` / `PATH` entries.

- [x] **Step 1: Add a failing compatibility contract**

```powershell
if ($contents -match '\.Environment(?:Variables)?\s*\[') {
    $failures.Add("$($_.Name) indexes ProcessStartInfo environment dictionaries")
}
```

- [x] **Step 2: Run the contract and observe RED**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\check-powershell-compatibility.ps1`

Expected before fix: FAIL naming `test-aspect-correction.ps1`; live capture fails with `Cannot index into a null array`.

- [x] **Step 3: Set variables in the harness process**

```powershell
[Environment]::SetEnvironmentVariable(
    'OPENJKDF2_VALIDATE_ENHANCEMENTS_MS',
    [string]$DurationMilliseconds,
    [EnvironmentVariableTarget]::Process)
```

- [x] **Step 4: Verify GREEN**

Expected: `PASS: PowerShell process harnesses use the Windows PowerShell 5.1-compatible environment API.`

### Task 2: Renderer standards and capability contract

**Files:**
- Create: `scripts/check-rdna-renderer-acceptance.ps1`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Consumes: `resource/shaders/*.glsl`, `ShaderCompile.c`, `std3D.c`, and `Window.c`.
- Produces: CTest `rdna_renderer_acceptance_contract`.

- [ ] **Step 1: Write the source contract**

Require these fourteen stages and seven program loads:

```powershell
$stages = @(
  'blur_f.glsl','blur_v.glsl','default_f.glsl','default_v.glsl',
  'menu_f.glsl','menu_v.glsl','ssao_f.glsl','ssao_v.glsl',
  'ssao_mix_f.glsl','ssao_mix_v.glsl','texfbo_f.glsl','texfbo_v.glsl',
  'ui_f.glsl','ui_v.glsl')
$programs = @('default','menu','ui','texfbo','blur','ssao','ssao_mix')
```

Also require compile/link status checks, sized formats, framebuffer completeness, context fallback telemetry, extension-gated anisotropy, and compatibility-fallback logging. Reject vendor-string conditionals.

- [ ] **Step 2: Run the contract and verify RED**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\check-rdna-renderer-acceptance.ps1`

Expected: FAIL until every promised invariant has an exact assertion.

- [ ] **Step 3: Make only audit-driven corrections**

Use exact positive and negative source patterns. Do not add a backend or vendor workaround. If a production check is missing, add a focused failing C test before its minimal implementation.

- [ ] **Step 4: Verify focused tests**

Run: `ctest --test-dir build/msvc-release -R 'shader|renderer|quality|rdna' --output-on-failure`

Expected: every selected test passes.

### Task 3: Strict one-minute production verifier

**Files:**
- Modify: `scripts/test-enhancement-performance.ps1`
- Modify: `scripts/check-enhancement-runtime-telemetry.ps1`

**Interfaces:**
- Consumes: JSONL diagnostics, run-state, screenshot, Windows Application log, and Steam/display snapshots.
- Produces: `enhancement-performance-result.json` with strict renderer fields.

- [ ] **Step 1: Extend the source contract**

Require 28 compile events / 14 distinct stages, 14 links / 7 distinct programs, zero incomplete FBOs, zero texture errors, zero diagnostic errors, zero software-renderer markers, and zero Application Errors.

- [ ] **Step 2: Verify RED**

Run: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\check-enhancement-runtime-telemetry.ps1`

Expected: FAIL because the old harness only validates pacing and dimensions.

- [ ] **Step 3: Parse structured evidence**

Parse each JSONL record, compare unique logical names to the declared lists, and query Application Error only between launch and exit for `openjkdf2-64.exe`.

- [ ] **Step 4: Scale the pacing floor**

```powershell
$minimumFrames = [Math]::Floor(($DurationMilliseconds / 1000.0) * 60.0 * 0.90)
$pacingPass = $totalFrames -ge $minimumFrames -and
  [Math]::Abs($medianMs - (1000.0 / 60.0)) -le (1000.0 / 60.0) * 0.05 -and
  $p95Ms -le (1000.0 / 60.0) * 1.15
```

- [ ] **Step 5: Verify GREEN**

Expected: the runtime telemetry contract passes.

### Task 4: RX 7900 XTX capture and evidence

**Files:**
- Create: `docs/evidence/rdna-renderer-acceptance-2026-07-15.json`
- Create: `docs/evidence/rdna-renderer-acceptance-2026-07-15.md`
- Modify: `COMPATIBILITY.md`
- Modify: `docs/evidence/requirements.csv`
- Modify: `FINAL-REPORT.md`

**Interfaces:**
- Consumes: strict raw capture plus first-door, timing, input, save/load, crash-restoration, and package evidence.
- Produces: privacy-safe proof for `M3-RDNA` and `AC-RX7900` only.

- [ ] **Step 1: Run the final capture**

```powershell
$root = Join-Path (Get-Location) ('build\rdna-renderer-final-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-enhancement-performance.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -UserDir $root -DurationMilliseconds 60000 -TimeoutSeconds 100
```

Expected: 2560x1440 Borderless, clean exit, display/Steam invariant, at least 3,240 frames, and all strict checks pass.

- [ ] **Step 2: Inspect the screenshot**

Confirm coherent geometry, textures, character model, lighting, and HUD without obvious shader corruption. Keep it ignored.

- [ ] **Step 3: Curate evidence**

Record timestamps, executable hash, privacy-safe host labels, event counts, pacing, safety invariants, visual result, and cross-evidence links; omit raw paths.

- [ ] **Step 4: Update claims conservatively**

Set `M3-RDNA` and `AC-RX7900` proven. Keep unavailable hardware unverified and full-campaign coverage a limitation.

### Task 5: Gates, review, and commits

**Files:** all files above.

**Interfaces:**
- Consumes: final tree and raw capture.
- Produces: focused implementation and evidence commits.

- [ ] **Step 1: Run exact gates**

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Debug -Test
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Release -Test
```

Expected: all tests pass in both configurations.

- [ ] **Step 2: Audit equality/privacy**

Require exact counts, pacing, executable hash, safety state, no private paths, and no staged raw files.

- [ ] **Step 3: Commit separately**

Implementation: `test: harden Windows renderer validation`.

Evidence: `docs: prove RX 7900 XTX renderer acceptance`.
