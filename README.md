# OpenJKDF2 AMD Enhanced

A Windows 11-focused OpenJKDF2 fork that makes *Star Wars: Jedi Knight - Dark
Forces II* comfortable on a modern PC: modern FPS controls by default,
native-resolution Borderless Fullscreen, safer display switching, improved frame
pacing, and observable OpenGL 3.3 rendering on modern AMD Radeon hardware.

**[Download release 1.0](https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced/releases/tag/1.0)** ·
**[Release notes](docs/RELEASE-1.0.md)** · **[Licensing guide](LICENSING.md)**

> [!IMPORTANT]
> This download contains the enhanced engine only. It includes **no game files**.
> You need a legally owned installation of *Jedi Knight: Dark Forces II* from
> [Steam](https://store.steampowered.com/app/32380/) or
> [GOG](https://www.gog.com/en/game/star_wars_jedi_knight_dark_forces_ii).

## Screenshots

| Gameplay | Modern controls |
| --- | --- |
| [![Gameplay at 2560×1440](docs/images/amd-enhanced-gameplay.png)](docs/images/amd-enhanced-gameplay.png) | [![Modern control style selected](docs/images/amd-enhanced-modern-controls.png)](docs/images/amd-enhanced-modern-controls.png) |
| **Borderless at native resolution** | **Built-in diagnostics** |
| [![Borderless Fullscreen at desktop resolution](docs/images/amd-enhanced-display-options.png)](docs/images/amd-enhanced-display-options.png) | [![In-game renderer diagnostics](docs/images/amd-enhanced-diagnostics.png)](docs/images/amd-enhanced-diagnostics.png) |

Screenshots use a locally owned game installation for technical demonstration;
original game content is not included in this repository or its downloads.

## What this fork adds

- Modern FPS controls for new players, plus an in-game Modern/Classic switch.
- Borderless Fullscreen at the primary monitor's native resolution by default.
- Working monitor, mode, resolution, and refresh-behavior controls under Display
  Options, with confirmation and restoration safeguards.
- Standards-compliant OpenGL 3.3 fixes, shader/framebuffer diagnostics, renderer
  fallbacks, and hardware evidence for the tested Radeon RX 7900 XTX system.
- Configurable frame caps, VSync, frame-pacing telemetry, FOV, HUD scale, SSAA,
  bloom, SSAO, filtering, and optional user-owned enhancement packs.
- Read-only discovery of Steam/GOG game data, isolated per-user saves/settings,
  portable mode, crash diagnostics, and a Windows display watchdog.

AMD hardware is not required. AMD/RDNA is the tested optimization and diagnostic
focus; untested GPUs use standards-based capability checks and fallbacks.

## Requirements

- 64-bit Windows 11 (the supported and tested release target).
- A GPU and driver supporting an OpenGL 3.3 core profile.
- A legal Steam or GOG installation of *Jedi Knight: Dark Forces II*.
- About 150 MB for the engine package, separate from the original game.

## Install in three steps

1. Download `OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip` from the
   [1.0 release](https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced/releases/tag/1.0)
   and extract the **entire** ZIP to a normal writable folder.
2. In the extracted folder, right-click `Install.ps1` and choose **Run with
   PowerShell**. If Windows blocks scripts, open PowerShell in that folder and run:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1
   ```

3. Start **OpenJKDF2 AMD Enhanced** from the new desktop shortcut. The launcher
   searches common Steam and GOG locations. If it cannot find the game, select
   the folder that contains your legal installation when prompted.

The engine reads the original installation in place; it does not copy or modify
your game files.

### Portable mode and uninstall

- Run `OpenJKDF2 AMD Enhanced Portable.cmd` from the extracted package to keep
  saves and settings in its local `UserData` folder.
- Run the installed `Uninstall.ps1` to remove the app and shortcut. Original game
  files and normal saves under `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced` are never
  deleted by the uninstaller.

## Modern controls

Modern is the default for new player profiles:

| Action | Default input |
| --- | --- |
| Move / look | WASD / mouse |
| Primary / secondary fire | Left mouse / right mouse |
| Jump / crouch | Space / Ctrl or C |
| Run / use | Shift / E |
| Weapons | Mouse wheel or number keys |
| Map / menu | Tab / Escape |

Switch at any time through **Setup → Controls → Options → Control Style**. Choose
**Modern** or **Classic**, then select **Apply Control Style**. The choice is
saved to the active player profile.

## Display modes

Open **Setup → Display → Display Options** to choose:

- **Borderless Fullscreen (Recommended):** the default; uses the selected
  monitor's desktop pixel count and refresh behavior without a title bar or
  taskbar.
- **Windowed:** editable width and height for desktop use.
- **Exclusive Fullscreen:** shown only when the exact desktop-state restoration
  watchdog has completed its safety preflight.

Changes require confirmation and fall back to the last known-good display state
if confirmation fails.

## Verify the download

The release includes `SHA256SUMS.txt`. From the download folder, run:

```powershell
Get-FileHash .\OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip -Algorithm SHA256
Get-Content .\SHA256SUMS.txt
```

The two SHA-256 values must match. Build provenance and a full package manifest
are also included inside the ZIP.

## Troubleshooting

- **Game not found:** run the launcher again and select the installation folder
  containing the original game's `Resource` data.
- **Display changed unexpectedly:** restart normally; the watchdog and recovery
  snapshot restore the last known-good desktop configuration.
- **Settings or saves:** normal mode uses `%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced`;
  portable mode uses `UserData` beside the package.
- **Renderer problem:** open **Setup → Display → Advanced → Diagnostics**, then
  include the diagnostics files when filing an issue.

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) and
[COMPATIBILITY.md](COMPATIBILITY.md) for detailed limitations and evidence.

## Build from source

Clone the exact release, including pinned submodules:

```powershell
git clone --branch 1.0 --recurse-submodules https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced.git
cd OpenJKDF2-AMD-Enhanced
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-windows.ps1 -Configuration Release -Test
```

The required Visual Studio 2022 Build Tools, CMake, Ninja, Python/cogapp setup,
and Debug build command are documented in
[docs/building-windows.md](docs/building-windows.md).

## Source, licenses, and attribution

This project preserves the history and attribution of
[OpenJKDF2](https://github.com/shinyquagsire23/OpenJKDF2). The original project
and George Karagioules's modifications are distributed under the custom
permission and warranty terms in [LICENSE.md](LICENSE.md); third-party components
retain their own licenses. Read [LICENSING.md](LICENSING.md),
[THIRD-PARTY-NOTICES.md](packaging/windows/THIRD-PARTY-NOTICES.md), and the
package's `Licenses` directory before redistributing binaries.

No Lucasfilm game files or game assets—including levels, textures, models,
music, video, dialogue, scripts, or proprietary executables—are distributed.
STAR WARS, Jedi Knight, LucasArts, Lucasfilm, and related marks belong to
Lucasfilm Ltd. AMD and AMD Radeon are trademarks of Advanced Micro Devices,
Inc. This independent community fork is not affiliated with or endorsed by
Lucasfilm, Disney, AMD, Valve, GOG, or the upstream OpenJKDF2 maintainers.
