# Crash-handler integration verification — 2026-07-14

The Windows startup path now initializes DrMinGW only after the writable user root and diagnostics directory are available. It resolves and validates both required DLL exports and directs reports to `diagnostics/OpenJKDF2-crash.RPT` instead of the executable directory.

Verification performed:

- `test_storage_paths` covers report-path joining, trailing separators, invalid input, and undersized output buffers.
- Debug x64: 23/23 tests passed.
- Release x64: 23/23 tests passed.
- A guarded Release gameplay run at 2560x1440 emitted the structured event `handler=drmingw location=diagnostics`.
- The run exited cleanly, preserved `2560x1440@165`, and left all 75 Steam asset metadata records unchanged.

Gameplay initialization evidence is retained outside source control under `runtime-evidence/crash-handler-2026-07-14`.

## Controlled crash report

An asset-free `openjkdf2-crash-probe.exe` now exercises the same dynamically loaded DrMinGW exports used by the game. `scripts/test-crash-report.ps1` runs it in an isolated directory, waits for the report, terminates any remaining Windows error-dialog process, and validates the output.

Measured Release result:

- unhandled exception: `EXCEPTION_ACCESS_VIOLATION`
- child exit code: `-1`
- report created: yes
- report size: 1,655 bytes
- exception/stack content: present
- privacy inspection: executable/module names, versions, addresses, and stack only; no username, filesystem path, command line, save data, or game asset content

The machine-readable result and report are retained outside source control under `runtime-evidence/crash-report-2026-07-14-01`.
