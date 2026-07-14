# OpenJKDF2 AMD Enhanced - Windows x64

Optional legally obtained visual packs are documented in
`ENHANCEMENT-PACKS.md`; none are bundled with this package.

This is an independent community source port. It is not affiliated with or
endorsed by LucasArts, Lucasfilm, Disney, AMD, Valve, GOG, or upstream OpenJKDF2.
Jedi Knight game data is not included. A legally owned Steam or GOG installation
is required.

Run `OpenJKDF2 AMD Enhanced.cmd`. On first run the launcher searches common Steam
and GOG locations, validates required files, and otherwise asks you to select the
original game folder. The port reads that folder without copying or modifying it.

For portable settings and saves, run `OpenJKDF2 AMD Enhanced Portable.cmd`.
Portable data is stored in `UserData` beside the package. Normal mode stores data
under `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced`.

Run `Install.ps1` to copy the application to your per-user Programs directory and
create one desktop shortcut. Run the installed `Uninstall.ps1` to remove the
application and its shortcut. Uninstall never removes original game files or the
normal per-user save directory. Portable `UserData` must be moved or explicitly
preserved before removing the package directory.
