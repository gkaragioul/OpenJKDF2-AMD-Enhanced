# Optional enhancement packs

OpenJKDF2 AMD Enhanced does not include third-party remaster assets, original
game assets, or an endorsement of any external pack. Use only files you have
legally obtained and whose license permits your intended use. Keep the pack's
license and attribution documents with the installed files.

## Supported layout

Launch the port once, then close it. Install user-owned files below the stable
writable data directory instead of modifying the Steam or GOG installation:

```text
%LOCALAPPDATA%\OpenJKDF2 AMD Enhanced\
  jkgm\materials\<pack-name>\
    metadata.json
    <high-resolution textures referenced by metadata.json>
```

Portable launches use `UserData` beside the executable instead of
`%LOCALAPPDATA%`. The engine resolves `jkgm/materials` through this writable
overlay and never needs to write into the original installation.

JKGM material packs may provide albedo, emissive, and displacement PNGs using
the upstream JKGM `metadata.json` format. Engine-compatible models and other
standard resource overrides may use their normal relative game paths below the
same writable directory. Follow the pack author's instructions because model
formats and paths vary.

## License checklist

Before installing a pack, verify all of the following:

1. The download came from the author or another authorized source.
2. Its license permits you to possess and use every included texture and model.
3. Required attribution is retained.
4. The pack does not ask you to redistribute LucasArts game data.
5. You keep a backup of saves and configuration before installing large mods.

The project package deliberately contains no optional pack. Its package
manifest and proprietary-asset scan cover only distributable project files.

## Controls and fallback

In **Setup > Display > Advanced**, `Asset Enhancements` independently enables
or disables JKGM replacements, and `Texture Precache` controls eager cache
loading. Texture filtering, anisotropic filtering, mipmap distance, SSAA,
bloom, SSAO, HUD scale, and quality presets remain independently configurable.

Classic quality disables optional asset enhancements and precaching. If a pack
is absent, disabled, malformed, or cannot load a replacement, the renderer uses
the original assets from the legitimate game installation as the guaranteed
fallback. Delete the pack directory to uninstall it; this does not touch the
original assets, saves, or port configuration.
