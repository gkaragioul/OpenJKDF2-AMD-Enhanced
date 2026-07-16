# OpenJKDF2 AMD Enhanced 1.0

Release 1.0 is the first public Windows x64 build of this independent OpenJKDF2
fork. It modernizes how *Jedi Knight: Dark Forces II* feels and behaves on a
current desktop while keeping original game content outside the distribution.

## Highlights

- Modern FPS controls are the default for new players: WASD and mouse look,
  left-click primary fire, right-click secondary fire, Space jump, Ctrl/C
  crouch, Shift run, E use, wheel/number-key weapons, Tab map, and Escape menu.
- **Setup → Controls → Options** provides a saved Modern/Classic Control Style
  switch.
- Borderless Fullscreen at the primary monitor's desktop resolution is the
  default.
- **Setup → Display → Display Options** provides working monitor, mode,
  resolution, and refresh controls with confirmation and restoration safeguards.
- The OpenGL 3.3 renderer includes standards fixes, capability-driven fallbacks,
  shader/framebuffer diagnostics, configurable effects, and frame-pacing tools.
- Steam/GOG discovery, external read-only game data, isolated user storage,
  portable mode, crash diagnostics, and a display watchdog are included.

## Requirements

- 64-bit Windows 11.
- A GPU/driver with OpenGL 3.3 core support.
- A legally owned Steam or GOG installation of *Star Wars: Jedi Knight - Dark
  Forces II*.

This release contains the enhanced engine only. **No original game files or
optional third-party enhancement packs are included.**

## Install

1. Download `OpenJKDF2-AMD-Enhanced-windows-x64-1.0.zip` and extract it fully.
2. Right-click `Install.ps1` and choose **Run with PowerShell**, or run:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1
   ```

3. Launch the desktop shortcut. The launcher discovers common Steam/GOG
   locations or asks you to select the original game folder.

Portable users can run `OpenJKDF2 AMD Enhanced Portable.cmd` without installing.

## Integrity

Windows ZIP SHA-256:

```text
__ZIP_SHA256__
```

Compare this value with `SHA256SUMS.txt`. The ZIP also contains a generated file
manifest and build provenance. Source corresponding to this binary is the exact
[`1.0` tag](https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced/tree/1.0),
including pinned submodule revisions and build scripts.

## Licensing and independence

See [LICENSE.md](https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced/blob/1.0/LICENSE.md),
[LICENSING.md](https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced/blob/1.0/LICENSING.md),
and [THIRD-PARTY-NOTICES.md](https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced/blob/1.0/packaging/windows/THIRD-PARTY-NOTICES.md).
The engine changes are distributed under those documented terms; third-party
components retain their respective licenses.

This is an independent community project and is not affiliated with or endorsed
by Lucasfilm, Disney, AMD, Valve, GOG, or the upstream OpenJKDF2 maintainers.
