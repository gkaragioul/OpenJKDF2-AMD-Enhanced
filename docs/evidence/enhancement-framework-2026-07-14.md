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

The clean-provenance Windows ZIP was subsequently rebuilt from commit
`58959a12`. Package verification found all 36 manifest entries valid, zero
proprietary findings, and the enhancement-pack guide present. Its SHA-256 is
`2dabdeba6c16ba1298a3a2dbe534449e82e4b60e3db2000498dba1f273f732d6`.

## Ultra runtime and sustained-performance probe

The Release renderer was subsequently exercised for 12 seconds at
2560x1440 with the complete Ultra preset applied live: texture filtering, 16x
anisotropy, 0.5 mip bias, bloom, SSAO, 1.5x SSAA, texture precaching, and
optional replacement discovery. A deliberately empty, locally created
metadata pack exercised the legal user-overlay path and guaranteed original
asset fallback without introducing third-party or proprietary content.

Reproduction:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-enhancement-performance.ps1 `
  -DataDir 'D:\SteamLibrary\steamapps\common\Star Wars Jedi Knight' `
  -UserDir 'runtime-evidence\enhancement-performance-2026-07-14-02'
```

Measured RX 7900 XTX results at a 60 FPS cap:

| Metric | Result |
|---|---:|
| Observation duration | 12 seconds |
| Recorded frames | 719 |
| Rolling statistics window | 60 frames |
| Median frame time | 16.6347 ms |
| p95 frame time | 16.9207 ms |
| p99 / worst frame time | 17.2880 ms |
| Screenshot | 2560x1440 |

The screenshot was visually inspected and showed coherent first-level
geometry, lighting, textures, character rendering, and HUD with no obvious
corruption. The process exited cleanly, desktop mode remained
`2560x1440@165`, and all Steam asset metadata remained invariant. The runtime
observer also exposed and corrected a measurement limitation: frame telemetry
now reports the total number of recorded frames independently of its 60-frame
rolling statistics window.

## Boundaries

This proves the modular framework, all Ultra renderer effects, legal optional
pack discovery, original-asset fallback, and sustained 1440p performance on
the local RX 7900 XTX. Installing a third-party high-resolution texture or
model pack is optional rather than a release requirement; none was used or
redistributed. Visual and performance behavior on other GPU families remains
covered by their separately labelled hardware-matrix status and is not
inferred from this run.
