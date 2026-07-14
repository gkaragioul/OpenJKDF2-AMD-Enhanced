# Death and autosave reload verification

Date: 2026-07-14 (Europe/Athens)

The guarded `scripts/test-death-reload.ps1` harness launched the real gameplay
executable against the owner's read-only Steam assets and a fresh user tree.
It exercised the production actor damage, player death, death-delay, autosave,
and autosave-restoration paths.

## Method

After the initial level autosave completed, the validation hook wrote a normal
checkpoint into the active `_JKAUTO_` slot with distinctive health and recorded
player position. It then applied lethal fall damage through
`sithActor_DamageActor`, required the engine's dead flag, waited the normal
3,000 milliseconds of game time, and invoked `sithPlayer_debug_loadauto`, the
same restoration function used by the normal restart input path. Completion
required the player to be alive with both health and three-dimensional position
restored to the checkpoint values.

Impact damage was initially suppressed because the level's player actor had
`SITH_AF_INVULNERABLE`; diagnostics showed zero applied damage and no dead
state. Fall damage was then used because the production damage path explicitly
allows it through that flag, corresponding to an ordinary gameplay death mode.

## Measured result

- Debug and Release each passed all 23 asset-free tests.
- Debug and Release runtime probes both completed successfully.
- The Release process returned the engine's successful value of 1.
- The autosave was 360,799 bytes under the writable user root.
- Lethal damage, dead-state delay, autosave reload, and restored health and
  position were recorded.
- The display was `2560x1440@165` before and after.
- Steam asset path/size/timestamp metadata was invariant.
- Final run state was clean and `process_finished` was recorded.

Together with `save-load-lifecycle-2026-07-14.md`, this verifies save, load,
death, reload, and fresh-process restart preservation on the available machine.
It does not verify first-door progression, level transition, or other hardware.
