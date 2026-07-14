# Save/load lifecycle verification

Date: 2026-07-14 (Europe/Athens)

The guarded `scripts/test-save-load-lifecycle.ps1` harness launched the real
gameplay executable against the owner's read-only Steam asset tree and a fresh
writable user tree. It exercised the production `sithGamesave_Save`,
`sithGamesave_Process`, and `sithGamesave_Restore` paths rather than a mock or
save-file parser.

## Method

The first process started `JK1` / `01narshadda.jkl`, set player health to a
distinct non-default fraction, requested a normal save, waited for the queued
serializer, changed health again, requested a normal restore, and required the
saved health to return. The second process started the level from a clean
engine process, restored the same file from the same user tree, and required
the distinctive saved health to return from disk.

The harness also snapshots every Steam asset file's relative path, size, and
UTC write timestamp before and after both launches, captures the Windows
display mode, requires clean diagnostic shutdown, and requires all validation
phase events. It rejects a user directory that overlaps the asset directory.

## Measured result

- Debug and Release each passed all 23 asset-free tests.
- Debug and Release two-process runtime probes both passed.
- Both processes returned the engine's successful exit value of 1.
- The validation save was 360,799 bytes and remained below the user root.
- Save request, deliberate state change, same-process restore, and fresh-process
  restore were all recorded; both restores reproduced the saved state.
- The display was `2560x1440@165` before and after.
- Steam asset path/size/timestamp metadata was invariant.
- Final run state was clean and `process_finished` was recorded.

This evidence verifies new-game save/load and restart restoration on the
available machine and asset set. It does not yet verify death/reload, level
transition, first-door progression, or compatibility with original retail
save files and other OpenJKDF2 versions.
