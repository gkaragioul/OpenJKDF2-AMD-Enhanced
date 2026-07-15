# Display Watchdog Production Integration Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Integrate, package, and verify the Windows display-restoration watchdog while keeping Exclusive fail-closed until an exact desktop snapshot can be reapplied.

**Architecture:** A Windows-only coordinator captures the current active display topology, performs a no-op restoration preflight, recaptures and arms the state, launches the standalone helper beside the game executable, and waits for a ready-file handshake. The engine exposes Exclusive only after all steps succeed. Normal exit disarms the state; crash or forced termination lets the helper restore it and write a proof record.

**Tech Stack:** C11, Win32 display/process APIs, SDL display policy, CMake/CTest, Windows PowerShell 5.1, JSON evidence.

## Global Constraints

- Never run Exclusive fullscreen during this milestone.
- Never bypass a denied display API or treat an unchanged desktop as proof that restoration succeeded.
- Borderless remains the default and must never call a physical display-mode API.
- Every launch failure, denied preflight, or missing handshake leaves the guard false.
- Keep state, ready, and proof files in the per-user diagnostics directory.
- Package no proprietary data and commit no raw runtime evidence or private path.
- Require exact Debug and Release Windows gates before updating evidence.

---

### Task 1: Production integration contract

**Files:**
- Create: `scripts/check-display-watchdog-integration.ps1`
- Modify: `cmake/OpenJKDF2Tests.cmake`

- [ ] Add a source/package contract requiring coordinator startup, fail-closed guard assignment, normal-exit disarm, helper ready handshake, build dependency, package inclusion, and package verification.
- [ ] Run the contract and observe RED against the standalone-only implementation.
- [ ] Register the contract in CTest.

### Task 2: Ready-handshake helper protocol

**Files:**
- Modify: `src/Tools/display_watchdog_main.c`
- Modify: `src/Platform/Win32/DisplayRestore.c`
- Modify: `src/Platform/Win32/DisplayRestore.h`

- [ ] Extend `--watch` with an atomic ready-file written only after the parent handle and armed state are valid.
- [ ] Add a read-only armed-state query so invalid or disarmed state never produces a ready signal.
- [ ] Preserve proof output for parent exit, wait failure, restore success, and restore failure.
- [ ] Verify capture/disarm and disposable-parent behavior without invoking Exclusive.

### Task 3: Engine watchdog coordinator

**Files:**
- Create: `src/Platform/Win32/DisplayWatchdog.h`
- Create: `src/Platform/Win32/DisplayWatchdog.c`
- Modify: `src/main.c`
- Modify: `src/Win95/Window.h`
- Modify: `src/Win95/Window.c`

- [ ] Capture the exact current topology and preflight its no-op reapplication.
- [ ] Recapture state, launch the colocated helper with safely quoted absolute paths, and wait with a bounded timeout for the ready handshake.
- [ ] Return false and disarm on every error; report a structured stage/result for diagnostics.
- [ ] Set the Window restoration guard from the coordinator result before any display creation.
- [ ] Disarm on the common clean-exit path before logging `process_finished`.

### Task 4: Build, package, and process validation

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `scripts/build-windows-package.ps1`
- Modify: `scripts/verify-windows-package.ps1`
- Modify: package documentation as needed.

- [ ] Make the game target depend on the watchdog target.
- [ ] Copy `openjkdf2-display-watchdog.exe` as `OpenJKDF2-Display-Watchdog.exe` and require it during package verification.
- [ ] Run the integration contract GREEN.
- [ ] Run a disarmed disposable-parent handshake and verify successful proof without changing display state.
- [ ] Run an armed exact-snapshot preflight; accept readiness only if the Win32 restore call succeeds.

### Task 5: Evidence, gates, and commits

**Files:**
- Modify: `docs/evidence/watchdog-verification.md`
- Modify: `docs/display-safety.md`
- Modify: `docs/evidence/requirements.csv`
- Modify: `FINAL-REPORT.md`

- [ ] Record production lifecycle coverage and the host's exact display-API result.
- [ ] If the host still denies snapshot reapplication, classify the hardware verification as a documented blocker and retain Borderless fallback; do not claim actual forced restoration.
- [ ] Build and verify the portable package contains the helper and no proprietary files.
- [ ] Run exact Debug and Release build/test gates.
- [ ] Audit staged files, privacy, raw artifacts, and `git diff --check`.
- [ ] Commit implementation and evidence separately.

