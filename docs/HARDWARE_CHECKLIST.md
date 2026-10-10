# On-device acceptance

No physical ESP32 was available for these checks. Record results on the actual N16R8 console.

- Check the new pin map, the two external pull-ups, 3.3 V logic and display power rating.
- Upload the release build over USB, including its bootloader and partition table.
- Confirm serial detects 16 MB flash / 8 MB physical PSRAM; check mapped heap and banked-history
  capacity separately. Wrong-capacity hardware must halt startup.
- Confirm portrait orientation, RGB colors, clipping and inversion. Record full-frame/tile time,
  actual frame-time buckets, update time, input gap, and minimum internal heap.
- Exercise all eight buttons independently, menu repeats, held-button transitions and pause/resume.
- Play every game and complete a stage where applicable; check collisions, controls and audio.
- Save scores/settings/favorites, reboot, and confirm persistence. Check the last-run history scrubber.
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
