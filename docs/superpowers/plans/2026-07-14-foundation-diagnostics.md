# Foundation and Renderer Diagnostics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce reproducible Windows x64 Debug/Release builds with automated unit tests, privacy-safe structured diagnostics, graceful shader failures, and an asset-free windowed renderer smoke-test command.

**Architecture:** Add a small `DiagnosticLog` service with no renderer dependency, then adapt shader compilation to return typed results instead of opening modal dialogs. Add pure command-line parsing and privacy-filtered environment-report helpers that can be unit tested on Windows. A dedicated `openjkdf2-renderer-smoke` executable initializes SDL/OpenGL only in a hidden window and uses generated resources; it never enters gameplay or changes display mode.

**Tech Stack:** C11, CMake 3.20+, CTest, SDL 3 compatibility API used by upstream, OpenGL 3.3/GLEW, Windows MSVC 2022 Build Tools, PowerShell packaging/verification scripts.

## Global Constraints

- Preserve upstream history and attribution from commit `a189787a6180c3132c4a736da0835df046f40c77`.
- Keep every generated file outside the Steam directory.
- Do not launch the game executable in this plan.
- Do not call `SDL_SetWindowFullscreen`, `SDL_SetWindowDisplayMode`, ChangeDisplaySettings, gamma APIs, HDR APIs, or color-profile APIs from tests or smoke mode.
- Do not read or package proprietary game assets; smoke resources are generated in memory.
- Do not report usernames, full user paths, raw command lines, save data, hostnames, or device serial numbers.
- Capability and compatibility conclusions must come from queried API behavior, never GPU-name branching.

---

## File structure

- `src/General/DiagnosticLog.h` / `.c`: lifecycle, typed severity, JSONL/text event emission, path redaction, and process-run marker.
- `src/General/DiagnosticReport.h` / `.c`: privacy-safe build/OS/renderer report assembly.
- `src/General/StartupOptions.h` / `.c`: side-effect-free parsing for `--safe-mode`, `--renderer-smoke-test`, `--diagnostics-dir`, and `--data-dir`.
- `src/Platform/GL/ShaderCompile.h` / `.c`: named-stage compile/link result API and compiler-log capture.
- `src/Tools/renderer_smoke_main.c`: hidden-window OpenGL initialization and generated-resource checks.
- `src/Tests/test_diagnostic_log.c`, `test_diagnostic_report.c`, `test_startup_options.c`, `test_shader_stage.c`: asset-free unit tests.
- `cmake/OpenJKDF2Tests.cmake`: portable test targets and CTest registration.
- `scripts/build-windows.ps1`: deterministic configure/build/test entry point.
- `scripts/capture-baseline.ps1`: privacy-safe machine/build manifest.
- `docs/diagnostics.md` and `docs/building-windows.md`: user/developer contract.

### Task 1: Reproducible Windows build and CTest foundation

**Files:**
- Modify: `.gitmodules`
- Modify: `CMakeLists.txt`
- Create: `cmake/OpenJKDF2Tests.cmake`
- Create: `scripts/build-windows.ps1`
- Create: `docs/building-windows.md`

**Interfaces:**
- Consumes: upstream `PLAT_MSVC`, `TARGET_BUILD_TESTS`, `sith_engine`, and `BIN_NAME` CMake definitions.
- Produces: cache option `OPENJKDF2_BUILD_TESTS:BOOL`; CTest labels `unit` and `renderer-smoke`; `scripts/build-windows.ps1 -Configuration Debug|Release -Test`.

- [ ] **Step 1: Initialize pinned submodules and record their exact commits**

Run:

```powershell
git submodule update --init --recursive
git submodule status --recursive | Set-Content build-evidence/submodules.txt
```

Expected: every top-level submodule line begins with one space, not `-` or `+`; the evidence file contains commit IDs only from the checked-out upstream tree.

- [ ] **Step 2: Add a failing configure assertion for Windows tests**

Create `cmake/OpenJKDF2Tests.cmake` with a guard that fails unless `BUILD_TESTING` is defined, include it under `if(OPENJKDF2_BUILD_TESTS)`, then configure:

```powershell
cmake -S . -B build/msvc-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOPENJKDF2_BUILD_TESTS=ON
```

Expected before the CMake integration: configure fails because `OPENJKDF2_BUILD_TESTS` is unused or no CTest targets exist.

- [ ] **Step 3: Add portable CTest integration**

Add to the root CMake file:

```cmake
option(OPENJKDF2_BUILD_TESTS "Build asset-free OpenJKDF2 tests" OFF)
if(OPENJKDF2_BUILD_TESTS)
    include(CTest)
    include(cmake/OpenJKDF2Tests.cmake)
endif()
```

In `cmake/OpenJKDF2Tests.cmake`, define `openjkdf2_add_unit_test(name source)` to create a target, require C11, add `${PROJECT_SOURCE_DIR}/src`, register `add_test`, and label it `unit`.

- [ ] **Step 4: Add the Windows build driver**

Implement an advanced PowerShell script with validated parameters, `Set-StrictMode -Version Latest`, `$ErrorActionPreference = 'Stop'`, separate `build/msvc-debug` and `build/msvc-release` directories, Ninja configure/build, and optional `ctest --output-on-failure`. The script must never invoke the game target.

- [ ] **Step 5: Verify Debug and Release configuration**

Run:

```powershell
pwsh -File scripts/build-windows.ps1 -Configuration Debug -Test
pwsh -File scripts/build-windows.ps1 -Configuration Release -Test
```

Expected: both builds succeed; CTest reports all registered tests passed; no process named `openjkdf2-64` is launched.

- [ ] **Step 6: Commit**

```powershell
git add .gitmodules CMakeLists.txt cmake/OpenJKDF2Tests.cmake scripts/build-windows.ps1 docs/building-windows.md build-evidence/submodules.txt
git commit -m "build: add reproducible Windows test builds"
```

### Task 2: Structured diagnostic logging

**Files:**
- Create: `src/General/DiagnosticLog.h`
- Create: `src/General/DiagnosticLog.c`
- Create: `src/Tests/test_diagnostic_log.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`
- Create: `docs/diagnostics.md`

**Interfaces:**
- Produces: `DiagLogConfig`, `DiagSeverity`, `diag_log_start(const DiagLogConfig*)`, `diag_log_event(DiagSeverity,const char*,const char*)`, `diag_log_finish(bool)`, `diag_redact_path(const char*,char*,size_t)`.
- Files: `openjkdf2.log`, `openjkdf2.jsonl`, and `run-state.json` under the explicitly supplied diagnostics directory.

- [ ] **Step 1: Write failing logger tests**

Test that JSON strings are escaped; one event produces valid one-line JSON; `C:\Users\alice\Games\JK\episode\JK1.gob` becomes `<data-dir>\episode\JK1.gob`; usernames and hostnames never appear; start creates an unclean marker; clean finish replaces it with a clean result.

- [ ] **Step 2: Run the focused test**

```powershell
cmake --build build/msvc-debug --target test_diagnostic_log
ctest --test-dir build/msvc-debug -R diagnostic_log --output-on-failure
```

Expected: FAIL because the diagnostic API is not defined.

- [ ] **Step 3: Implement minimal logger lifecycle**

Use an opaque static state containing only `FILE*`, normalized allowed roots, start time, and run ID. JSONL fields are `schema`, `timestamp_utc`, `severity`, `subsystem`, `event`, and `fields`. Write to temporary files first where atomic replacement is needed. Never accept `printf` format strings from runtime content.

- [ ] **Step 4: Pass tests and validate emitted JSON**

```powershell
ctest --test-dir build/msvc-debug -R diagnostic_log --output-on-failure
Get-Content build/msvc-debug/test-output/openjkdf2.jsonl | ForEach-Object { $_ | ConvertFrom-Json | Out-Null }
```

Expected: PASS and every line parses as JSON.

- [ ] **Step 5: Commit**

```powershell
git add src/General/DiagnosticLog.* src/Tests/test_diagnostic_log.c cmake/OpenJKDF2Tests.cmake docs/diagnostics.md
git commit -m "feat: add privacy-safe structured diagnostics"
```

### Task 3: Startup options and safe-mode contract

**Files:**
- Create: `src/General/StartupOptions.h`
- Create: `src/General/StartupOptions.c`
- Create: `src/Tests/test_startup_options.c`
- Modify: `src/main.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Produces: `StartupOptions startup_options_parse(int argc,const char* const* argv)` and fields `safe_mode`, `renderer_smoke_test`, `diagnostics_dir`, `data_dir`, `error`.
- Consumes: existing `-path`/`/path` behavior; maintains backward compatibility without logging raw arguments.

- [ ] **Step 1: Write failing parser tests**

Cover long options with separate values, legacy `-path`, missing values, duplicate switches where the last explicit value wins, unknown switches passed through to the legacy parser, and safe-mode defaults of windowed/60/conservative features.

- [ ] **Step 2: Confirm failure**

```powershell
ctest --test-dir build/msvc-debug -R startup_options --output-on-failure
```

Expected: FAIL because `startup_options_parse` is absent.

- [ ] **Step 3: Implement the side-effect-free parser**

Use fixed-size UTF-8 buffers with length validation. Return an error string instead of exiting. Do not resolve or open paths in this module.

- [ ] **Step 4: Integrate parsing before platform startup**

In `main`, parse once before `PHYSFS_init`, initialize diagnostics from the selected safe directory, record only switch presence and redacted path type, and preserve the legacy argument path. `--renderer-smoke-test` must dispatch to the separate smoke entry point in Task 5 rather than enter `Window_Main_Linux`.

- [ ] **Step 5: Run tests and commit**

```powershell
ctest --test-dir build/msvc-debug -R startup_options --output-on-failure
git add src/General/StartupOptions.* src/Tests/test_startup_options.c src/main.c cmake/OpenJKDF2Tests.cmake
git commit -m "feat: add safe startup option parsing"
```

### Task 4: Typed shader compilation and graceful failure

**Files:**
- Create: `src/Platform/GL/ShaderCompile.h`
- Create: `src/Platform/GL/ShaderCompile.c`
- Create: `src/Tests/test_shader_stage.c`
- Modify: `src/Platform/GL/shader_utils.h`
- Modify: `src/Platform/GL/shader_utils.c`
- Modify: `src/Platform/GL/std3D.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Produces: `ShaderStage`, `ShaderCompileResult { bool ok; unsigned object; ShaderStage stage; char name[64]; char source_sha256[65]; char log[4096]; }`, `shader_stage_name`, `shader_compile_named`, and `shader_link_named`.
- Consumes: current embedded shader resource loader and GL function table.

- [ ] **Step 1: Write failing pure tests**

Test enum-to-name mapping, log truncation with an explicit suffix, stable source SHA-256 formatting, and JSON-safe compiler-log normalization without creating a GL context.

- [ ] **Step 2: Confirm failure**

```powershell
ctest --test-dir build/msvc-debug -R shader_stage --output-on-failure
```

Expected: FAIL because typed shader helpers are absent.

- [ ] **Step 3: Implement typed compilation**

Compile with the existing GLSL preamble, always query `GL_COMPILE_STATUS`, query and retain the info log even on success, delete failed objects, and emit `renderer.shader_compile` with logical name, stage, hash, and compiler log. Link behaves analogously and deletes failed programs.

- [ ] **Step 4: Remove modal compiler-error behavior**

Replace `SDL_ShowSimpleMessageBox` in shader utilities with returned results and one startup-level user message that identifies the diagnostics directory. Update every caller in `std3D.c` to stop initialization cleanly and record the selected fallback.

- [ ] **Step 5: Run unit/static checks and commit**

```powershell
ctest --test-dir build/msvc-debug -R shader_stage --output-on-failure
rg -n "SDL_ShowSimpleMessageBox" src/Platform/GL
```

Expected: test passes and the search returns no shader compiler popup calls.

```powershell
git add src/Platform/GL/ShaderCompile.* src/Platform/GL/shader_utils.* src/Platform/GL/std3D.c src/Tests/test_shader_stage.c cmake/OpenJKDF2Tests.cmake
git commit -m "fix: make shader failures diagnostic and recoverable"
```

### Task 5: Privacy-safe report and asset-free renderer smoke tool

**Files:**
- Create: `src/General/DiagnosticReport.h`
- Create: `src/General/DiagnosticReport.c`
- Create: `src/Tests/test_diagnostic_report.c`
- Create: `src/Tools/renderer_smoke_main.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `diag_report_build`, `diag_report_set_renderer`, executable `openjkdf2-renderer-smoke`, and exit codes 0 success, 2 SDL initialization, 3 context creation, 4 loader, 5 shader, 6 framebuffer/readback.
- Consumes: `DiagnosticLog`, `ShaderCompile`, SDL, GLEW/OpenGL, and compile-time project version.

- [ ] **Step 1: Write failing privacy/report tests**

Verify deterministic schema keys, omission of hostname/serial/user fields, normalized OS/build fields, renderer strings only after a current context, and explicit `not_collected` values rather than invented data.

- [ ] **Step 2: Implement the report builder and pass tests**

```powershell
ctest --test-dir build/msvc-debug -R diagnostic_report --output-on-failure
```

Expected: PASS with a report that parses through `ConvertFrom-Json`.

- [ ] **Step 3: Implement the hidden-window smoke tool**

Initialize SDL video, request OpenGL 3.3 core, create a `64x64` window with `SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL`, initialize the GL loader, compile minimal generated vertex/fragment shaders through `ShaderCompile`, create an RGBA8 texture and framebuffer, check completeness, draw a triangle, read one pixel, emit results, destroy all resources, and quit SDL. Do not call any fullscreen or display-mode API.

- [ ] **Step 4: Register but do not automatically run hardware smoke in ordinary unit tests**

Add the executable to CMake. Register it with CTest label `renderer-smoke` and `DISABLED TRUE`; the explicit hardware verification command is:

```powershell
ctest --test-dir build/msvc-release -L renderer-smoke --output-on-failure --overwrite MemoryCheckCommandOptions=
```

Before enabling that test, inspect the binary imports/source to confirm it has no display-mode-changing calls.

- [ ] **Step 5: Commit**

```powershell
git add src/General/DiagnosticReport.* src/Tests/test_diagnostic_report.c src/Tools/renderer_smoke_main.c cmake/OpenJKDF2Tests.cmake CMakeLists.txt
git commit -m "test: add asset-free renderer smoke diagnostics"
```

### Task 6: Baseline evidence capture and acceptance map

**Files:**
- Create: `scripts/capture-baseline.ps1`
- Create: `docs/evidence/requirements.csv`
- Create: `docs/evidence/foundation-verification.md`
- Modify: `docs/baseline/environment-2026-07-14.md`
- Modify: `.gitignore`

**Interfaces:**
- Produces: privacy-safe `build-evidence/baseline.json`, hashes for Debug/Release binaries, CTest XML, smoke JSONL, and requirement status values `proven`, `contradicted`, `incomplete`, `weak`, `missing`, `unverified-hardware`.

- [ ] **Step 1: Add requirement rows for every brief milestone and acceptance criterion**

Each CSV row contains stable ID, exact requirement summary, authoritative evidence type, current status, evidence path, and notes. Initial unknown hardware coverage is `unverified-hardware`, never `proven`.

- [ ] **Step 2: Implement privacy-safe capture**

The script reads WMI/CMake/Git output into ordered objects, excludes username/hostname/serial/path fields, records display resolution/refresh without changing it, hashes build artifacts, parses CTest results, and writes UTF-8 JSON atomically.

- [ ] **Step 3: Run the full non-game verification**

```powershell
pwsh -File scripts/build-windows.ps1 -Configuration Debug -Test
pwsh -File scripts/build-windows.ps1 -Configuration Release -Test
ctest --test-dir build/msvc-release -L renderer-smoke --output-on-failure
pwsh -File scripts/capture-baseline.ps1
git diff --check
```

Expected: Debug/Release builds pass; all unit tests pass; explicit smoke test returns 0 or produces a precise structured failure without changing display state; baseline JSON parses; working tree changes are intentional.

- [ ] **Step 4: Verify display invariance around smoke mode**

Capture `CurrentHorizontalResolution`, `CurrentVerticalResolution`, and `CurrentRefreshRate` immediately before and after smoke execution and compare exact values. Record both samples and the smoke exit code in `foundation-verification.md`.

- [ ] **Step 5: Commit**

```powershell
git add scripts/capture-baseline.ps1 docs/evidence docs/baseline/environment-2026-07-14.md .gitignore
git commit -m "docs: record foundation verification evidence"
```

## Plan self-review

- Every first-stage requirement maps to an independently testable task.
- No gameplay or exclusive-fullscreen launch occurs.
- Renderer smoke uses generated data and hidden/windowed presentation only.
- Hardware smoke results can prove only this RX 7900 XTX/driver/OS configuration.
- Subsequent plans remain required for crash dumps, display watchdog, renderer compliance fixes discovered by evidence, resolution/UI, timing, input, enhancements, packaging, and full gameplay acceptance.
