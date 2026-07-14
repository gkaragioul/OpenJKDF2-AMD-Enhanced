# Crash-handler integration verification — 2026-07-14

The Windows startup path now initializes DrMinGW only after the writable user root and diagnostics directory are available. It resolves and validates both required DLL exports and directs reports to `diagnostics/OpenJKDF2-crash.RPT` instead of the executable directory.

Verification performed:

- `test_storage_paths` covers report-path joining, trailing separators, invalid input, and undersized output buffers.
- Debug x64: 23/23 tests passed.
- Release x64: 23/23 tests passed.
- A guarded Release gameplay run at 2560x1440 emitted the structured event `handler=drmingw location=diagnostics`.
- The run exited cleanly, preserved `2560x1440@165`, and left all 75 Steam asset metadata records unchanged.

Runtime evidence is retained outside source control under `runtime-evidence/crash-handler-2026-07-14`. A deliberate unhandled-exception test has not yet been run, so creation and contents of a real `.RPT` remain unproven.
