# Read-only game-data overlay verification

Date: 2026-07-14 (Europe/Athens)

The Release x64 executable was launched directly against the owner's Steam
installation using `--data-dir`. The Steam path and fresh `--user-dir` both
contained spaces. No junctions were used and no proprietary asset was copied.

The guarded `scripts/test-data-overlay.ps1` probe rejects overlapping roots,
requires a fresh user root, snapshots every asset file's relative path, size,
and UTC last-write timestamp, samples the Windows display mode, and requires a
clean diagnostic shutdown after a real gameplay capture.

## Result

- Release startup returned the engine's successful value of 1.
- Diagnostics recorded `path_overlay active=true writable=user`.
- `JK1` / `01narshadda.jkl` produced a decoded 2560x1440 gameplay PNG.
- Visual inspection showed coherent geometry, textures, and HUD with no
  obvious shader corruption in that single frame.
- The display was 2560x1440 at 165 Hz before and after.
- The asset snapshot contained 75 files before and after; relative paths,
  sizes, and UTC last-write timestamps were identical.
- Configuration, bindings, cvars, diagnostics, capture, and the
  `_JKAUTO_01narshadda.jks` autosave were all below the writable user root.
- Diagnostics recorded the validation event, `process_finished`, and clean
  run-state.

This is bounded evidence for one Steam installation and one short run on the
available RX 7900 XTX. It does not prove GOG discovery, first-run browsing,
portable packaging, upgrades, uninstall behavior, first-door stability,
save/load behavior, or other GPUs. File contents were not hashed because the
probe uses non-invasive path/size/timestamp mutation detection; distributable
package artifacts will receive content checksums.
