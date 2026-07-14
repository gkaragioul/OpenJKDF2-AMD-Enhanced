# Video settings UX evidence - 2026-07-14

## Scope

This slice verifies that both video settings screens expose an explicit Apply
action and that the guarded display-change, reset, persistence, and recovery
paths remain wired into the production UI.

## Contract coverage

The `video_menu_contract` CTest checks the following production-source
contracts:

- Display and Advanced Video each use the localized `Apply` button.
- Applying a display change starts a 15-second transaction and opens the timed
  confirmation prompt.
- Confirm commits the transaction; cancel or timeout reverts it and logs the
  revert.
- Recommended and safe video reset actions are exposed with confirmation
  prompts.
- Accepted settings are persisted through the player configuration writer.
- Startup snapshots the configuration as the last-known-good copy.

The focused C tests add behavioral coverage: `test_display_transaction` covers
expiry (including tick wraparound), confirmation, cancellation, and result
states; `test_video_defaults` validates the complete recommended and safe
profiles; and `test_config_recovery` validates snapshot, restore, and recovery
offer decisions.

## Verification

The required Windows builds both completed successfully on 2026-07-14:

| Configuration | Result |
|---|---|
| Debug | 26/26 CTest tests passed |
| Release | 26/26 CTest tests passed |

The test was first run red and reported the three intended omissions:
`localization:GUIEXT_APPLY`, `menu:GUIEXT_APPLY`, and
`apply-buttons:display-and-advanced`. It passed after the two labels and
localization entry were added.

## Boundaries

This is source-contract and focused unit evidence. The current non-interactive
desktop session could not give the game a foreground window, so it does not
prove a user-driven timed confirm, timeout revert, injected mode-set failure,
or recovery prompt in a live menu. Exclusive fullscreen also remains gated.
Accordingly, requirement M8-VIDEO-UX remains incomplete rather than proven.
