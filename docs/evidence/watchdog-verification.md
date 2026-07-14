# Display watchdog verification

Date: 2026-07-14 (Europe/Athens)

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

The watchdog source and executable are retained for verification in an
interactive desktop environment with display-configuration access. Exclusive
testing remains prohibited until that verification returns success.
