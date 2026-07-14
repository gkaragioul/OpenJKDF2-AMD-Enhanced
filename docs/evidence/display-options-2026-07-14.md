# Display options runtime verification — 2026-07-14

## Scope

Release build `build/msvc-release/openjkdf2-64.exe` was launched through the normal startup path against the legitimate, read-only Steam data directory. The automated verifier seeded a fresh per-user `registry.json` for each case, measured the live Win32 client rectangle, captured the rendered presentation, inspected structured diagnostics, and compared the Windows desktop mode and all Steam asset file metadata before and after each run.

Command:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-display-options.ps1 `
  -DataDir "D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight" `
  -EvidenceRoot "runtime-evidence\display-options-2026-07-14-02"
```

Machine-readable result: `runtime-evidence/display-options-2026-07-14-02/display-options-result.json` (local evidence, intentionally excluded from release packages).

## Results

The host exposed three displays. Its primary desktop remained `2560x1440@165` before and after every case.

| Case | Requested / effective client | Capture | Monitor | Result |
| --- | --- | --- | --- | --- |
| Windowed 1080p | 1920x1080 | 1920x1080 | 1 | Pass |
| Windowed 4K | 3840x2160 | 3840x2160 | 1 | Pass |
| Borderless | 2560x1440 desktop | 2560x1440 | 1 | Pass |
| Windowed second display | 1280x720 | 1280x720 | 2 | Pass |

All four runs reported a clean run state and `process_finished`, returned the verifier's expected validation exit code, contained no shader error event, and persisted the requested mode, monitor, and last Windowed dimensions. Borderless preserved its seeded `800x600` last-Windowed size rather than overwriting it with desktop dimensions.

The Steam source contained 75 files before and after testing; recursive relative path, byte length, and UTC modification timestamp snapshots were identical. No test wrote into the Steam directory.

## Safety and coverage limits

- Exclusive Fullscreen was not invoked. It remains unavailable until the independent restoration guard is ready.
- The physically available desktop refresh was 165 Hz. The verifier did not claim 60, 120, or 144 Hz display-mode coverage.
- This run proves behavior on this Windows 11 / RX 7900 XTX host only; it does not establish RDNA-wide compatibility.
- Live timed-confirmation interaction and forced-termination/Alt+Tab display tests remain separate acceptance work.
