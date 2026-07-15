# Display safety

AMD Enhanced models display state as three explicit modes: Windowed,
Borderless, and Exclusive. Legacy `fullscreen=true` settings map to Borderless,
which is the default for a new configuration and uses the current desktop
bounds without requesting a display-mode change.

Safe mode always resolves to Windowed. Exclusive requests are downgraded to
Borderless unless the external restoration guard is armed. The ordinary
Windowed and Borderless creation paths never call `SDL_SetWindowFullscreen` or
set a display mode. Borderless removes the window frame and uses the selected
display's existing bounds, resolution, and refresh rate.

The in-game **Display Options** submenu enumerates SDL displays and modes but
stores only a stable monitor ordinal and the user's Windowed dimensions. Apply
uses a complete settings snapshot and a 15-second confirmation transaction.
Only confirmed settings are written to the per-user registry; timeout, Cancel,
or rejection restores the snapshot. Borderless does not overwrite the saved
Windowed dimensions.

On Windows, the engine captures the active topology and attempts an exact
no-op restoration preflight before it starts the separately running helper. It
recaptures and arms the state, waits for an armed-state/PID ready handshake, and
only then marks the restoration guard ready. Clean exit disarms the state; a
crash or forced parent exit leaves the helper responsible for restoration.

The available desktop token denied the exact preflight, so the production guard
remains false and Exclusive continues to resolve to Borderless on this host.
Measured Windowed, Borderless, multi-monitor, desktop-mode, and Steam-data
invariance evidence is recorded in
`docs/evidence/display-options-2026-07-14.md`. Watchdog integration and the
host blocker are recorded in `docs/evidence/watchdog-verification.md`.
Exclusive was not invoked.
