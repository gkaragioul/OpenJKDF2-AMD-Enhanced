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

Exclusive mode remains unavailable until the separately running restoration
watchdog has captured all active displays and its forced-termination behavior
has passed before/after display-state verification.
