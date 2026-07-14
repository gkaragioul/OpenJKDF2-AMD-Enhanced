# Frame-rate UI persistence — 2026-07-14

The production Display menu was exercised through real foreground mouse and keyboard input against an isolated player profile. The harness selected 120 FPS on the actual slider, captured the rendered label, invoked the normal Apply shortcut, and launched a second process using the same profile.

Command:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-frame-rate-ui.ps1 -DataDir "D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight" -EvidenceRoot "runtime-evidence\frame-rate-ui-2026-07-14-16" -TargetFps 120
```

Measured result:

- first rendered capture visibly shows `120 FPS (timing caution)` and the Apply control
- structured first-run dismissal reports `result=1 fps_limit=120`
- `UserData/player/Validation/openjkdf2.json` contains `"fpslimit": 120`
- a fresh second process reports `display_page displayed=true fps_limit=120`
- the restart capture visibly shows the same 120 FPS slider value
- first and restart processes both exited normally with code 1
- legitimate Steam asset metadata remained unchanged
- Exclusive fullscreen was not invoked

Retained artifacts:

- `runtime-evidence/frame-rate-ui-2026-07-14-16/frame-rate-selected.png`
- `runtime-evidence/frame-rate-ui-2026-07-14-16/frame-rate-reloaded.png`
- `runtime-evidence/frame-rate-ui-2026-07-14-16/frame-rate-ui-result.json`

The harness requires explicit Win32 foreground ownership before interaction and capture. This prevents an obscuring desktop window from being mistaken for the game UI and makes the interaction evidence specific to the OpenJKDF2 window.
