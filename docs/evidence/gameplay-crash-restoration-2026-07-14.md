# Gameplay crash and recovery verification — 2026-07-14

## Method

The Release executable launched real first-level gameplay in desktop-native
Borderless mode with raw/relative mouse enabled. An opt-in validation hook raised
a Windows access violation three seconds after the player and sector became
live. The harness then measured display/cursor state, validated the DrMinGW
report, relaunched the same profile, accepted the product recovery dialog's
default **Restore and launch safely** action, and required a clean rendered
presentation and shutdown. Exclusive Fullscreen was never invoked.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File scripts\test-gameplay-crash-restoration.ps1 `
  -DataDir "D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight" `
  -EvidenceRoot "runtime-evidence\gameplay-crash-restoration-2026-07-14-03"
```

## Measured result

- production crash trigger logged: yes
- crash exit code: `-1073741819` (`0xC0000005`, access violation)
- DrMinGW report: created, 3,949 bytes, recognizable exception/stack content
- desktop before / after crash / after recovery: `2560x1440@165` throughout
- cursor clip before / after crash / after recovery:
  `-1920,0,4480,1440` throughout
- recovery prompt accepted: yes
- `last_known_good_restored safe_mode=true`: logged
- post-crash Release presentation: captured and cleanly exited
- recursive Steam asset path/size/timestamp snapshot: identical

The raw machine-readable result and crash report are retained outside source
control under the evidence root above. This closes the available-host acceptance
path for launch, play, focus loss, abnormal termination, real gameplay crash,
recovery, and normal exit without a Windows display-mode change.

## Limits

This validates Borderless safety on the available Windows 11 / RX 7900 XTX
three-monitor host. It does not validate Exclusive restoration, other drivers,
or other display topologies.
