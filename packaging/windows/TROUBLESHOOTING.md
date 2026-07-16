# Troubleshooting

- If the launcher reports missing files, select the folder containing `JK.EXE`
  plus `Episode\JK1.GOB`, `Resource\Res1hi.gob`, and `Resource\Res2.gob`.
- Logs and crash diagnostics are under
  `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced\diagnostics` in normal mode or
  `UserData\diagnostics` in portable mode.
- Use `--safe-mode` after the launcher name to force conservative windowed video
  defaults. Exclusive fullscreen remains unsupported unless display restoration
  is available; Borderless is the safe default.
- Do not copy original GOB files into this package. Keep the Steam/GOG install
  separate and pass it through the launcher.
- Do not remove or rename `OpenJKDF2-Display-Watchdog.exe`. If Windows denies
  the exact desktop-state preflight, diagnostics report `preflight_failed` and
  Exclusive remains unavailable; use Borderless, which does not change the
  desktop mode.

# Display options

Use **Setup > Display > Display Options** to select Windowed, Borderless
Fullscreen (Recommended), or Exclusive Fullscreen, plus the target monitor and
Windowed size. Borderless is the default, always uses the selected monitor's
current desktop mode, and does not request a resolution or refresh-rate change.
Confirm the timed prompt to keep a change; cancellation or timeout restores the
complete previous display settings.

Exclusive Fullscreen is intentionally unavailable until the display-restoration
guard reports ready. Do not bypass this safety gate. If a saved display is no
longer connected, startup falls back to safe Windowed settings.

# Controls

New profiles default to Modern controls. If right-click jumps, the active
profile is using Classic or custom legacy bindings. Open
**Setup > Controls > Control Options**, set **Control Style** to Modern, and
select **Apply Control Style**. This maps right-click to secondary fire and
Space to jump. Classic remains available from the same selector.
