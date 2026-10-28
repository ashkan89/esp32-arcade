# Modern artwork release 3.7.0

This revision renews the visual presentation of all 52 games with bitmap artwork and
redesigns the launcher around illustrated feature cards. It supersedes the procedural-art
presentation described in the older release notes. Per-game changes are enumerated in
[GRAPHICS_COVERAGE.md](GRAPHICS_COVERAGE.md).

## Production assets and rendering

The project now contains 52 illustrated game environments, four console materials and
104 foreground sprites. Ships, aliens, cars, actors, monsters, tanks, food, crates,
puzzle tiles, gems, cards, platforms and other game objects use packed bitmap assets.
There are dedicated walking/crouching characters, correctly oriented tanks/spacecraft,
rear-view cars for perspective racing and separate symbols for memory pairs.

The original game rules, collision geometry, scores, IDs, controls and saved records are
retained. Small projectiles, collision cues, HUD text and thin interactive geometry still
use efficient drawing operations. A backdrop cannot introduce a new collision object:
the river's art is masked by its real banks/islands; pinball's decorative artwork is kept
around its playable field; store floors and transport remain aligned to their actual rules.

Source PNGs, hashes and provenance are in `assets/art/`. Rebuild embedded data with:

```powershell
python -m pip install Pillow==12.3.0
python scripts/pack_art.py
python scripts/audit_art.py
```

Pillow is needed only to repack art, not for a normal firmware build. The packed artwork
and bounds occupy 281,888 flash bytes, with no extra framebuffer and no runtime image
decoding. RGB565 is used on WROVER. WROOM prefers a single internal RGB565 buffer if
allocation leaves a 128 KiB reserve, and retains RGB332 for network screens and low-memory
fallback. Screen-edge reallocations clear and invalidate the new framebuffer; failure
retains the old buffer. Network actions are rejected safely if compact allocation fails.
The real 176×220 screen limits detail and color resolution; source PNG quality is not a
claim that the panel can reproduce a desktop render.

The launcher has a large illustrated preview and a three-row library, category labels,
favorites, recents and retained filter/sort settings for the session. Left/Right moves
three titles. All menus use authored metal/leather materials and a constellation backdrop.
Decorative UI motion is localized to a glint so it does not dirty every SPI band.
Reduced Effects freezes that motion. Settings, help, pause and results retain their
documented button controls.

Both Atari rule modes now use the renewed artwork, as requested. Classic selects the
historical reconstructed rule set; it no longer denotes cartridge-style sprite art.
Neither title is claimed to be pixel-perfect, cycle-accurate or an authorized commercial port.

## Hardware identified on COM9

Read-only identification and the existing 3.5.1 boot log establish ESP32-D0WD-V3 revision
3.1, 4 MB flash, no initialized/physical PSRAM, and the WROOM N4 firmware identity.
The partition table matches `partitions_4mb.csv`: NVS 0x9000 / 0x5000, OTA data 0xe000 /
0x2000, app0 0x10000 / 0x1f0000, app1 0x200000 / 0x1f0000, coredump 0x3f0000 / 0x10000.
Existing display/button/audio pins are unchanged. Its earlier full-frame RGB8 transfer
measured 42,466 µs; this is a baseline measurement, not this revision's achieved FPS.

A complete 4,194,304-byte backup is stored privately at `.cache/com9-backup-20261028.bin`.
SHA-256: `97c755416e68570ea4ce649460931161b23ee6e247645e49e800e66330914f20`.
It includes user NVS and is excluded from source/release packages. The backup succeeded
at 115200 baud; the earlier 460800 attempt was stopped without writing flash.

## Verification and installation procedure

Normal verification remains `python scripts/verify.py`. Additional coverage includes
four-bit sampling, transparency, clipping, crop/stride behavior, mirroring, rotated sprites,
all asset bounds, per-game rendering and asset-provenance checks. Older tests that assumed
one flat skin/road/depot color now check visible actors, continuous water gaps and depot
visibility in the collision band while retaining HUD/ceiling isolation checks.

`pio run -e art-check` builds an opt-in WROOM 4 MB diagnostic. It exercises each actual
game for 180 simulation ticks and 60 physical presentations, records update/draw/transfer
maxima, free heap, stack high-water mark and framebuffer CRC, and permits Back to abort.
It yields to the RTOS, retains watchdog/brownout protection, runs without network activity,
and does not record gameplay scores or recents. It is not a human playthrough.

The diagnostic image must be replaced with the normal `wroom-4mb` image after measurement.
Installation uses application-only writes after checking chip, flash, partition layout,
image identity, size and a complete backup. NVS, bootloader and partition table are preserved.
Do not distribute the `art-check` image as a normal release. Results and remaining manual
acceptance checks are recorded below.

## Final build and device results (2026-10-28)

All four normal targets built successfully. The host suite passed 2,282,590 assertions, including 2,080,000 lifecycle ticks and 15,323 loss/restart cycles. Asset hashing and coverage checks passed for all 52 games.

| Target | Static RAM bytes | Linked flash bytes | Application image bytes |
| --- | ---: | ---: | ---: |
| release | 66208 | 1592225 | 1598800 |
| debug | 66224 | 1744621 | 1751200 |
| wroom-4mb | 66152 | 1568597 | 1575168 |
| wroom-16mb | 66152 | 1568613 | 1575184 |

The physical COM9 diagnostic completed all 52 games: 180 simulated ticks and 60 actual draw/SPI presentations per game. Ten RGB565/RGB332 allocation cycles also passed. Heap remained exactly 198,536 bytes at every game boundary (198,736 before initial setup; minimum free heap 193,192). Minimum stack headroom was 4,460 bytes after raising the existing Arduino loop stack from 8 to 12 KiB; the first 8 KiB run had only 364 bytes. ESP-IDF reports these high-water marks in bytes. Watchdog and brownout protection remain enabled.

The maximum sampled update, drawing and presentation times were 684, 20,504 and 43,132 microseconds respectively. These are observed stage maxima, not guaranteed frame rates. The scheduler may pace heavy screens at 30 FPS; this is not a claim of 60 FPS across all games.

Raw device evidence: `artifacts/com9-art-check.log`; per-game measurements: `test-results/art-device-results.json`. Normal builds: `test-results/firmware-builds.log` and `firmware-summary.json`; host evidence: `test-results/host-tests.log`. Normal release packages are under `dist/`; diagnostic firmware is excluded.

### Installation status

**Normal 3.7.0 firmware is installed and boot-health confirmed on COM9.** Continuous transfers encountered USB serial timeouts, so installation used 25 independently hash-verified application sections. A subsequent whole-image verification matched all 1,575,168 bytes against the normal WROOM 4 MB binary (SHA-256 `ef5c34b8bc7259f66dafcd7f5921e5beef25e8d4ab7d24dd9b313caa2702dad4`). No diagnostic run occurs on normal startup. Logs: `artifacts/com9-section-install.log`, `com9-section-session.log` and `com9-final-reboot.log`.

The normal firmware selected RGB565 and passed startup health checks twice, including a reset after a read-only NVS capture. The final saved-state blob has valid CRC and schema 4; preferences and favorites match the schema 3 backup, and none of the existing score fields decreased. Two score fields increased, while last-game/recent entries changed during device use; exact byte equality of those activity fields is therefore not claimed. The original private backup is retained. All installation writes stayed inside app0; NVS, the bootloader and partition layout were preserved.

Automated simulation and successful SPI transfers do not verify physical button operation, panel appearance, audio quality, Wi-Fi provisioning, power-loss rollback, long-duration stability or a complete human playthrough of every game. These remain explicit manual acceptance checks.
