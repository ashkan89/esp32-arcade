# On-device acceptance

No serial device was detected on October 28, 2026. Earlier physical results are recorded in
VERIFICATION.md; this revision has not been flashed. Record new results on the fitted console.

- Check the existing pin map (B=4, Menu=5, Back=15), button-to-ground wiring, 3.3 V logic and display
  power rating. All eight buttons use internal pull-ups.
- Keep Menu/Back released during power-on/reset; confirm normal startup.
- Identify the board and obtain upload authorization before flashing. Preserve NVS; use a full
  bootloader/partition upload only for first installation or a deliberate partition migration.
- Confirm serial detects 16 MB flash / 8 MB physical PSRAM and check mapped heap.
  Wrong-capacity hardware must halt startup; no frame-history allocation should be logged.
- Confirm portrait orientation, RGB colors, clipping and inversion. Record full-frame/tile time,
  actual frame-time buckets, update time, input gap, and minimum internal heap.
- Exercise all eight buttons independently, menu repeats, held-button transitions and pause/resume.
- Play all 52 games at Easy/Medium/Hard/Nightmare (Keystone uses original fixed rules);
  complete the first fifty campaigns and each boss at stages 4/8/12
  (5/10/12 for Sky Invaders, Star Assault and Meteor Patrol). Record stage clear, loss, pause,
  restart, exit and victory. Test held and rapidly tapped actions, edges and resource exhaustion.
- Measure every game's worst-case render/update costs, especially full tubes/swarms, bombs,
  explosions, moving brick sentinel and full block/puzzle boards. Record achieved FPS and latency.
- Confirm bosses remain defeatable when limited consumables are exhausted: regenerated City
  Shield ammunition and Burger Works pepper, non-consumable gun/pump/bubble/snow attacks.
- Save scores/settings/favorites, leave a safe menu open for 60 seconds after the final change,
  reboot, and confirm persistence. Check the system menu has six entries including Library.
- Browse each library filter and sorting mode, remove the last favorite, launch from Recent,
  restart and switch games repeatedly. Reboot and confirm the eight-entry recent order.
- Test both reset actions: Cancel and B must preserve data; confirmed preferences reset keeps
  scores; confirmed score reset keeps preferences. Both keep favorites, recents and Wi-Fi.
- Toggle a setting and revert it before saving; verify no save message. Make repeated edits,
  confirm no more than one state save per minute, and verify the newest state survives reboot.
- Submit unchanged Wi-Fi setup without a config write. Repeat changed setup within a minute:
  expect HTTP 429; wait and retry. Test failed-save retry behavior and power loss before saving.
- Update from older firmware with existing records; confirm Sky Invaders scores, settings and
  favorites survive the namespace migration, and that the prior firmware still boots on rollback.
- Open Wi-Fi setup, enter credentials, save and verify timeout/exit closes the AP. Check that
  the display stays responsive during a GitHub check and that gameplay starts with Wi-Fi off.
- Install a newer test release to the inactive slot. Record active slot before/after and wait
  for the boot-health acceptance serial message. Exercise explicit previous-firmware boot.
- Interrupt download power: the existing slot must still boot. Reject bad hash, wrong-board,
  truncated and oversized test releases. These tests require a separate test repository/release.
- Reset an updated image before its eight-second acceptance window: the prior healthy image
  must boot. Test an update with deliberately failed startup checks and confirm rollback.
- Record free/minimum heap through repeated connect/check/update failures and long game sessions.
- Verify behavior with Wi-Fi unavailable, NTP blocked, GitHub rate limiting and missing assets.

Host tests verify rules, serialization, UI transitions and clipping. They do not establish
SPI performance, radio behavior, physical PSRAM banking or bootloader power-loss recovery.
