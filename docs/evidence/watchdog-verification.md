# Display watchdog verification

Date: 2026-07-14 (Europe/Athens)

## Production integration update - 2026-07-15

Commit `c5ed5c0c` integrates the helper into the production game lifecycle. Before
any display creation, the engine captures the active topology, attempts an exact
no-op restoration preflight, recaptures the state, starts the colocated helper,
and waits up to five seconds for an armed-state/PID ready handshake. Only a
successful sequence marks the restoration guard ready. Clean exit disarms the
state before the process terminates.

The process contract proved that the helper opens the watched parent, emits the
ready record only for an armed snapshot, waits for parent exit, honors clean
disarm, writes a successful zero-display proof for that disarmed exit, and
rejects a snapshot disarmed before launch without creating a ready record.
Resolution and refresh were `2560x1440@165` before, after the preflight, and
after the process test. Exclusive was never invoked.

A clean-provenance Release package contains the required helper and verified 37
manifest entries with zero proprietary findings. Machine-readable results and
artifact hashes are in `docs/evidence/display-watchdog-2026-07-15.json`.

The standalone watchdog successfully captured one active display, opened and
waited on an unrelated disposable parent process, detected forced termination,
and entered its restoration path. The current Codex desktop host then rejected
all available state-application mechanisms:

- `SetDisplayConfig` with the exact `QueryDisplayConfig` snapshot returned
  `ERROR_ACCESS_DENIED` (5).
- Per-device and global `ChangeDisplaySettingsEx` returned
  `DISP_CHANGE_FAILED` (-1), including no-op reapplication of the current mode.

Resolution and refresh remained 2560×1440 at 165 Hz throughout. Because actual
restoration could not be proven, the engine does not mark the guard ready and
all Exclusive requests continue to resolve to Borderless. Borderless itself
does not invoke a display-mode API and has passed before/after invariance.

The watchdog is production-integrated and packaged, but actual armed
restoration remains blocked on this host by display-configuration authorization.
Exclusive testing remains prohibited until an interactive desktop host passes
the exact-snapshot preflight and forced-parent-exit restoration test.
