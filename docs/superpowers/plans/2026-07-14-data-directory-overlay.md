# Data Directory Overlay Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Launch directly against a legitimate read-only Steam or GOG game-data directory while keeping every user write in a stable separate root, including when paths contain spaces.

**Architecture:** Introduce a small `PathOverlay` module that owns normalized asset and writable roots and resolves relative reads with writable override precedence followed by the read-only asset root. Startup changes the process working directory to the writable root and removes consumed long options from the legacy command line without translating `--data-dir` to mod-oriented `-path`; engine opens, directory enumeration, cutscenes, music, and enhancement discovery share the resolver.

**Tech Stack:** C11, Win32 known-folder APIs, existing MSVC/Ninja/CMake harness, SDL3/SDL_mixer, CTest.

## Global Constraints

- Never modify the installed Steam or GOG game data.
- Never package or commit proprietary game assets or runtime captures.
- Writable user files take precedence over base assets so legal enhancement packs and mods remain possible.
- Absolute paths remain unchanged; write-capable opens always target the writable root.
- `--portable` is explicit and uses a directory beside the executable; default storage uses `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced`.
- Exclusive fullscreen remains gated and must not be exercised by these tests.

---

### Task 1: Pure overlay and startup-option contracts

**Files:**
- Create: `src/General/PathOverlay.h`
- Create: `src/General/PathOverlay.c`
- Create: `src/Tests/test_path_overlay.c`
- Modify: `src/General/StartupOptions.h`
- Modify: `src/General/StartupOptions.c`
- Modify: `src/Tests/test_startup_options.c`
- Modify: `cmake/OpenJKDF2Tests.cmake`

**Interfaces:**
- Produces: `path_overlay_configure(const char*, const char*)`, `path_overlay_resolve_read(const char*, char*, size_t)`, `path_overlay_resolve_write(const char*, char*, size_t)`, `path_overlay_resolve_asset(const char*, char*, size_t)`, `path_overlay_asset_root()`, and `path_overlay_writable_root()`.
- Produces: `StartupOptions.user_dir` and `StartupOptions.portable` parsed from `--user-dir` and `--portable`.

- [ ] Write tests proving paths with spaces are retained, absolute paths are untouched, traversal outside either root is rejected, writable overrides win, asset fallback works, and writes never resolve below the asset root.
- [ ] Run the isolated test targets and confirm the expected missing-symbol or assertion failures.
- [ ] Implement the minimum pure path logic and startup parsing.
- [ ] Run the isolated tests and the full Debug suite.
- [ ] Commit as `feat: add read-only data overlay contracts`.

### Task 2: Startup and file-access integration

**Files:**
- Create: `src/Platform/Win32/UserDataRoot.h`
- Create: `src/Platform/Win32/UserDataRoot.c`
- Modify: `src/main.c`
- Modify: `src/Win95/Window.c`
- Modify: `src/stdPlatform.c`
- Modify: `src/General/stdFileUtil.h`
- Modify: `src/General/stdFileUtil.c`
- Modify: `src/Main/jkCutscene.c`
- Modify: `src/Win95/stdMci.c`
- Modify: `src/Platform/GL/jkgm.cpp`
- Modify: the main target source list in `CMakeLists.txt` or its included CMake module.

**Interfaces:**
- Consumes: all Task 1 `PathOverlay` functions.
- Produces: `user_data_root_select(const StartupOptions*, const char*, char*, size_t)` and a legacy-argument builder that omits consumed `--data-dir`, `--user-dir`, `--portable`, `--diagnostics-dir`, and `--safe-mode` tokens without flattening their values.

- [ ] Write failing tests for writable-root selection and exact legacy argument filtering, including quoted legacy arguments and long-option values containing spaces.
- [ ] Run the isolated targets and confirm failures for the missing integration functions.
- [ ] Configure the overlay before diagnostics, create/chdir to the writable root, and make diagnostics relative to that root by default.
- [ ] Route engine reads, media reads, enhancement discovery, and directory searches through writable-first/asset-second resolution while keeping all writes writable-only.
- [ ] Run Debug and Release builds with all tests.
- [ ] Commit as `feat: separate game assets from user data`.

### Task 3: Real-install verification without junctions

**Files:**
- Create: `scripts/test-data-overlay.ps1`
- Create: `docs/evidence/data-overlay-verification-2026-07-14.md`
- Modify: `docs/evidence/requirements.csv`
- Modify: `docs/diagnostics.md`

**Interfaces:**
- Consumes: Release executable, a legitimate external `--data-dir`, and a fresh workspace-local `--user-dir`.
- Produces: a privacy-safe JSON result containing launch result, asset validation, screenshot dimensions, clean shutdown, writable-file inventory, Steam before/after metadata comparison, and display-mode invariance.

- [ ] Write the verification script with explicit root-containment guards and no delete/write operation against the asset root.
- [ ] Run Release directly against the Steam path containing spaces, with no junctions or copied proprietary assets.
- [ ] Verify actual gameplay capture and all generated configuration/profile/diagnostic files are under the writable root.
- [ ] Compare Steam file count, sizes, and last-write timestamps before/after; require no changes.
- [ ] Run fresh Debug and Release harnesses, `git diff --check`, and a package-boundary scan.
- [ ] Document bounded evidence and update only requirements directly supported by the run.
- [ ] Commit as `test: verify read-only game data overlay`.

## Self-review

- Spec coverage: covers path spaces, default and explicit writable roots, portable mode, writable precedence, engine/media/enhancement reads, enumeration, real Steam launch, no Steam writes, and evidence. First-run browse UI and GOG auto-discovery remain intentionally separate packaging tasks.
- Placeholder scan: no implementation placeholders or deferred steps are present in this plan.
- Type consistency: Task 2 and Task 3 consume the exact `PathOverlay` and `StartupOptions` interfaces introduced by Task 1.
