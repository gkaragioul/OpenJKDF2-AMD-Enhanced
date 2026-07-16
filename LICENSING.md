# Licensing guide

OpenJKDF2 AMD Enhanced is a modified distribution of
[OpenJKDF2](https://github.com/shinyquagsire23/OpenJKDF2). This guide explains
the licensing structure of the source tree and Windows package. It does not
replace any license text and is not legal advice.

## OpenJKDF2 and AMD Enhanced changes

The original OpenJKDF2 code remains copyright the OpenJKDF2 contributors under
the custom permissive permission and warranty terms in [`LICENSE.md`](LICENSE.md).
GitHub currently classifies that file as `Other`/`NOASSERTION`; this project does
not relabel it as MIT, ISC, GPL, or another standard SPDX license.

OpenJKDF2 AMD Enhanced modifications are copyright 2026 George Karagioules and
are offered under the same permission and warranty terms in `LICENSE.md`. This
statement covers only George Karagioules's contributions. It does not relicense
OpenJKDF2 or any third-party component.

## Components with separate terms

The source tree preserves third-party license files beside their components.
The Windows binary incorporates or ships the components below:

| Component | Version/source used | License |
| --- | --- | --- |
| libsmacker | 1.1.1, source in `src/external/libsmacker` | LGPL-2.1-or-later |
| OpenAL Soft | pinned commit `d3875f333fb6abe2f39d82caca329414871ae53b` (1.23.1), `lib/openal` | LGPL-2.0-or-later |
| DrMinGW | [0.9.3](https://github.com/jrfonseca/drmingw/tree/0.9.3), replaceable runtime DLLs | LGPL-2.1 plus bundled zlib/libdwarf notices |
| SDL | pinned submodule in `lib/SDL` | zlib license |
| SDL_mixer | pinned submodule in `lib/SDL_mixer` | zlib license; enabled codec dependencies retain their own notices |
| FreeGLUT, GLEW, zlib, libpng | pinned source/build dependencies | their included permissive licenses |
| libsmusher | source in `src/external/libsmusher` | MIT |
| nativefiledialog-extended | source in `src/external/nativefiledialog-extended` | zlib license |
| nlohmann/json | source in `3rdparty/json` | MIT |

See `packaging/windows/THIRD-PARTY-NOTICES.md` and the `Licenses` directory in
the Windows package for the notices shipped with the binary distribution.

## Corresponding source and relinking

The Windows executable statically incorporates libsmacker and other
source-built libraries. The complete application source, libsmacker source,
build scripts, and pinned dependency revisions used for release `1.0` are
available from the `1.0` tag in this repository. This is the material used to
rebuild the application and relink it with a modified compatible library.

Clone the tagged source and all pinned submodules with:

```powershell
git clone --branch 1.0 --recurse-submodules https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced.git
```

OpenAL Soft and the DrMinGW crash-handler components are shipped as replaceable
DLLs. You may replace them with interface-compatible modified builds. The
distribution terms permit modification for your own use and reverse engineering
for debugging those modifications. No additional restriction is imposed on the
rights granted by their licenses.

Equivalent access to the corresponding source is offered from the same GitHub
Release location as the Windows binary. The exact build procedure is documented
in `docs/building-windows.md`.

## Game content and trademarks

No Lucasfilm game files, assets, levels, textures, models, music, video,
dialogue, scripts, or proprietary game executables are licensed or distributed
by this project. Users must provide their own legally obtained Steam or GOG copy
of *Star Wars: Jedi Knight - Dark Forces II*.

STAR WARS, Jedi Knight, LucasArts, Lucasfilm, and related marks are properties
of Lucasfilm Ltd. AMD and AMD Radeon are trademarks of Advanced Micro Devices,
Inc. This independent project is not affiliated with or endorsed by Lucasfilm,
Disney, AMD, Valve, GOG, or the upstream OpenJKDF2 maintainers.
