# Third-party notices

OpenJKDF2 AMD Enhanced is based on OpenJKDF2 and retains its history and
attribution. The source license is included as `LICENSE.md`; the distribution
structure and corresponding source locations are documented in `LICENSING.md`.

The binary incorporates or ships open-source dependencies including SDL,
SDL_mixer, OpenAL Soft, FreeGLUT, GLEW, zlib, libpng, libsmacker, libsmusher,
nativefiledialog-extended, nlohmann/json, and DrMinGW components.

- libsmacker 1.1.1 is LGPL-2.1-or-later and is compiled into the application.
  Its complete corresponding source is in `src/external/libsmacker` at the `1.0`
  source tag, together with the application source and build scripts needed to
  rebuild and relink it.
- OpenAL Soft 1.23.1 is LGPL-2.0-or-later. `OpenAL32.dll` is a replaceable shared
  library built from pinned commit `d3875f333fb6abe2f39d82caca329414871ae53b`.
- DrMinGW 0.9.3 supplies the replaceable `exchndl.dll`, `mgwhelp.dll`, and
  `symsrv.dll` crash-reporting components under LGPL-2.1 and accompanying
  zlib/libdwarf notices. Source: https://github.com/jrfonseca/drmingw/tree/0.9.3
- SDL, SDL_mixer, FreeGLUT, GLEW, zlib, libpng, libsmusher,
  nativefiledialog-extended, nlohmann/json, and the enabled Ogg/Vorbis/Opus
  codec components retain the license texts included in `Licenses`.

The complete tagged corresponding source can be obtained with:

```text
git clone --branch 1.0 --recurse-submodules https://github.com/gkaragioul/OpenJKDF2-AMD-Enhanced.git
```

The distribution permits modification for personal use and reverse engineering
for debugging modifications to LGPL-covered components. No additional
restriction is imposed on rights granted by their licenses.

No LucasArts/Lucasfilm game data, levels, textures, music, video, dialogue, or
other proprietary assets are included.
