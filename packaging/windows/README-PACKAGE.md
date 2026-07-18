# OpenJKDF2 AMD Enhanced - Windows x64

Optional legally obtained visual packs are documented in
`ENHANCEMENT-PACKS.md`; none are bundled with this package.

This is an independent community source port. It is not affiliated with or
endorsed by LucasArts, Lucasfilm, Disney, AMD, Valve, GOG, or upstream OpenJKDF2.
No game files are included. A legally owned Steam or GOG installation is
required.

> **Windows 11 Smart App Control:** the current 1.0 community binaries are not
> yet code-signed. Smart App Control may block an unsigned executable even when
> its package hash is correct, and Windows does not provide a per-app exception.
> Do not change a system-wide security setting merely to bypass an unexpected
> download. First verify `SHA256SUMS.txt`, confirm that the package came from the
> official GitHub release, and read `TROUBLESHOOTING.md`.

Extract this entire package into its own writable folder and keep the package
layout intact. Do not merge it into the original Jedi Knight directory. Run
`OpenJKDF2 AMD Enhanced.cmd`. On first run the launcher searches common Steam and
GOG locations, validates required files, and otherwise asks you to select the
original folder containing `JK.EXE`, `Episode\JK1.GOB`, `Resource\Res1hi.gob`, and
`Resource\Res2.gob`. The port reads that folder without copying or modifying it.

New player profiles use Modern controls by default: WASD and mouse look,
left-click primary fire, right-click secondary fire, Space jump, Ctrl/C crouch,
Shift run, E use, the mouse wheel and number keys for weapons, Tab for the map,
and Escape for the menu. To switch styles, open
**Setup > Controls > Control Options**, choose Modern or Classic under
**Control Style**, then select **Apply Control Style**. The choice is saved to
the active player profile. Imported or customized profiles keep their existing
bindings. If vertical mouse look is inverted, or right-click jumps, apply the
Modern style once for that profile.

The game opens in Borderless Fullscreen by default. Use
**Setup > Display > Display Options** to choose Windowed, Borderless Fullscreen
(Recommended), or Exclusive Fullscreen. Exclusive is shown but remains
unavailable unless the display-restoration safety guard is ready.

`OpenJKDF2-Display-Watchdog.exe` is a required safety helper and must remain
beside the game executable. The launcher starts the game normally; the engine
arms the helper itself only after an exact desktop-state restoration preflight
and ready handshake succeed.

For portable settings and saves, run `OpenJKDF2 AMD Enhanced Portable.cmd`.
Portable data is stored in `UserData` beside the package. Normal mode stores data
under `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced`.

Run `Install.ps1` to copy the application to your per-user Programs directory and
create one desktop shortcut. Run the installed `Uninstall.ps1` to remove the
application and its shortcut. Uninstall never removes original game files or the
normal per-user save directory. Portable `UserData` must be moved or explicitly
preserved before removing the package directory.

See `LICENSING.md`, `THIRD-PARTY-NOTICES.md`, and the `Licenses` directory for
source and third-party terms. The matching tagged source is available at
https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced/tree/1.0.
