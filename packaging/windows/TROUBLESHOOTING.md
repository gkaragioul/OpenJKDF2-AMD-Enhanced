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
