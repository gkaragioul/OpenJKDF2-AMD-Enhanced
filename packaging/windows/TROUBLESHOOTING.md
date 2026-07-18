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
- Do not extract or merge this package into the original Jedi Knight directory.
  Keep the engine package in its own folder; the launcher reads the original
  installation through the read-only data overlay.
- Do not remove or rename `OpenJKDF2-Display-Watchdog.exe`. If Windows denies
  the exact desktop-state preflight, diagnostics report `preflight_failed` and
  Exclusive remains unavailable; use Borderless, which does not change the
  desktop mode.

# Smart App Control blocks the game

The current 1.0 community release is not yet Authenticode-signed. On a Windows
11 PC where Smart App Control is **On**, starting the game may fail before the
engine creates a log and PowerShell may report:

```text
An Application Control policy has blocked this file.
```

This message is produced by Windows Code Integrity, not by the game-data
launcher, AMD graphics code, or the original Jedi Knight installation. Smart App
Control asks Microsoft's app-intelligence service about an executable; unknown
unsigned code can be blocked even when it is legitimate. Microsoft currently
provides no per-app **Allow** button for Smart App Control.

Before changing anything, verify that the ZIP came from the official GitHub
release and that its SHA-256 matches `SHA256SUMS.txt`. You can inspect the local
state and signature from PowerShell:

```powershell
Get-MpComputerStatus | Select-Object SmartAppControlState
Get-AuthenticodeSignature .\OpenJKDF2-AMD-Enhanced.exe |
    Select-Object Status, StatusMessage
```

If Smart App Control is **On** and the signature status is `NotSigned`, the
durable publisher-side solution is a future release signed with a certificate
from a trusted certificate authority. The user-side alternative is turning
Smart App Control off for the entire PC under **Windows Security > App & browser
control > Smart App Control**. That is a system-wide security tradeoff, not an
exception for this game. Leave Microsoft Defender antivirus, real-time
protection, Windows Firewall, User Account Control, and reputation-based
protection enabled. Current Windows 11 updates allow Smart App Control to be
enabled again from Windows Security.

Official Microsoft references:

- [Smart App Control frequently asked questions](https://support.microsoft.com/windows/smart-app-control-frequently-asked-questions-285ea03d-fa88-4d56-882e-6698afdb7003)
- [Smart App Control overview](https://learn.microsoft.com/windows/apps/develop/smart-app-control/overview)
- [Code signing for Smart App Control](https://learn.microsoft.com/windows/apps/develop/smart-app-control/code-signing-for-smart-app-control)

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

New profiles default to Modern controls. Imported or customized profiles keep
their existing bindings. If vertical mouse look is inverted or right-click
jumps, the active profile is using Classic or custom legacy bindings. Open
**Setup > Controls > Control Options**, set **Control Style** to Modern, and
select **Apply Control Style**. This maps right-click to secondary fire and
Space to jump, corrects the Modern mouse-look mapping, and saves the choice to
that profile. Classic remains available from the same selector.
