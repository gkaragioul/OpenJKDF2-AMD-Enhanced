# Enhancement framework evidence - 2026-07-14

## Scope

This slice verifies modular renderer quality settings, safe supersampling,
writable-overlay discovery for optional material packs, and guaranteed fallback
to the owner's original game assets. No third-party or proprietary remaster
asset was used or added to the repository.

## Implemented contracts

- Classic, Balanced, High, Ultra, and Custom presets now account for texture
  filtering, anisotropy, mip distance, bloom, SSAO, SSAA, texture precaching,
  and optional asset replacements.
- Classic explicitly disables optional replacements and precaching, preserving
  the original assets as the baseline.
- High and Ultra expose bounded 1.25x and 1.5x SSAA respectively. Profile, UI,
  and renderer paths clamp SSAA to 1.0x-2.0x and reject non-finite values,
  preventing malformed settings from allocating pathological framebuffers.
- Selecting a preset updates every governed control; changing a governed value
  makes the active preset Custom.
- JKGM material discovery now resolves `jkgm/materials` through the writable
  per-user overlay before the read-only game-data root.
- Path and hash-signature replacements retain the matched cache entry. The old
  hash path incorrectly indexed the path map and could create an empty override.
- The packaged `ENHANCEMENT-PACKS.md` guide documents legal acquisition,
  licenses, attribution, install layout, removal, model overrides, and fallback.

## Automated verification

The feature was developed through observed red/green tests. The initial quality
test failed on the absent preset fields, expanded matching API, and SSAA clamp.
The framework contract then failed on the absent overlay loader, fallback
telemetry, legal guide, package inclusion, and path/hash cache selection.

Final required Windows results:

| Configuration | Result |
|---|---|
| Debug | 27/27 CTest tests passed |
| Release | 27/27 CTest tests passed |

## Release runtime fallback probe

Reproduction:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-data-overlay.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -UserDir 'runtime-evidence\enhancement-overlay-2026-07-14-01' `
  -EmptyEnhancementPack -TimeoutSeconds 40
```

The harness created an empty JKGM pack only in the fresh writable user tree.
The Release engine discovered that overlay, logged
`original_asset_fallback reason=no_matching_override`, rendered a visible
2560x1440 first-level frame from the original assets, and exited cleanly.

Measured results:

- Process result: expected success value 1
- Screenshot: 2560x1440, mean luminance 38.78, lit fraction 0.8541
- Desktop: 2560x1440@165 before and after
- Steam tree: all 75 file metadata records invariant
- Run state: clean, with `process_finished`

Visual inspection found coherent geometry, original textures, character model,
and both HUD corners, with no obvious corruption in the captured frame.

## Boundaries

This proves modular source contracts and the no-match original-asset fallback
on the local RX 7900 XTX. It does not validate a third-party high-resolution
texture or model pack, every quality preset visually, sustained performance of
bloom/SSAO/SSAA, or other GPU families. M7 therefore remains incomplete rather
than proven.
