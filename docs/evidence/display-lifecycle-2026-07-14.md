# Display and input lifecycle verification — 2026-07-14

## Method

The Release executable was launched against the legitimate Steam data in
desktop-native Borderless mode. `scripts/test-display-lifecycle.ps1` waited for
the production presentation-ready diagnostic event, transferred foreground
focus between the game and a probe window three times, then ran a second game
process and forcibly terminated it. Exclusive Fullscreen was never requested.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File scripts\test-display-lifecycle.ps1 `
  -DataDir "D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight" `
  -EvidenceRoot "runtime-evidence\display-lifecycle-2026-07-14-02"
```

Machine-readable output is retained outside source control at
`runtime-evidence/display-lifecycle-2026-07-14-02/display-lifecycle-result.json`.

## Measured result

- focus transfers requested: 3
- structured focus-lost events: 3
- structured focus-gained events: 5 (including startup/reacquisition)
- structured mouse-capture release events: 3
- normal validation process exit: clean
- forced process termination: completed
- Windows display before/after: `2560x1440@165` / `2560x1440@165`
- cursor clip before/after: `-1920,0,4480,1440` / `-1920,0,4480,1440`
- recursive Steam asset path/size/timestamp snapshot: identical

This proves Borderless focus-loss and forced-termination invariance on the
available three-monitor Windows 11 host. It also proves that each tested focus
loss reached the production cursor-capture release path.

## Limits

This does not authorize or validate Exclusive Fullscreen. A real in-process
gameplay crash with an active display window, mixed-DPI interaction, and other
GPU/monitor configurations remain separate coverage. The controlled DrMinGW
crash-report probe is documented independently.
