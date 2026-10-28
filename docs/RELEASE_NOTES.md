## 3.7.0 modern graphics — October 28, 2026

- Renew graphics for all 52 games with 52 bitmap environments, four UI materials and 104 sprites.
- Replace primitive characters/vehicles/objects with authored palette artwork; add proper
  snake, lander, birds/riders, rear-view race cars, duelists, food, cards, enemies and props.
- Rebuild the launcher around an illustrated feature card and three-row browsing. Theme
  menus, help, settings, pause and results with textured panels and a constellation backdrop.
- Add clipped four-bit rendering, tight sprite bounds and correct fractional-scale mirroring.
- Preserve gameplay, IDs, scores, controls, active wiring and memory profiles.
- Add opt-in COM9/WROOM4 graphics diagnostics and a source-backed 52-game art coverage report.
- Classic Atari mode retains reconstructed rules with modern art; cartridge graphic accuracy
  is not claimed. See MODERN_GRAPHICS.md for measured results and remaining manual checks.

## Historical 3.6.0 interface work — October 28, 2026

- Preserve all 52 games, stable catalog IDs, active wiring, partitions and boot protections.
- Add Library filters for categories, favorites and recent launches; sort by title or mode best.
- Save the eight most recent actual launches with v4 CRC validation and v1–v3 migration.
- Add separate settings/score reset actions with Cancel selected on a confirmation screen.
- Correct stale wiring instructions and the claim that Atari score attacks end after twelve stages.
- Add a reproducible numbered inventory, save/menu regressions and real-renderer menu captures.
- Firmware downgrade to a v1–v3-only decoder does not preserve access to new v4 saves. See
  [migration and acceptance limits](REDESIGN_CHECKPOINT.md). No firmware was uploaded or published.

## Historical 3.5.1: frame history removal and flash write protection

- Remove the frame-history ring, capture/playback work and system-menu entry on all targets.
- Skip unchanged or reverted settings saves, batch edits and limit NVS attempts/retries to
  once per minute per save path. Wi-Fi setup also skips identical saved configuration.
- Keep defaults in RAM until a real change; preserve save formats, migrations and the WROVER
  constructor safeguard. See [FLASH_WRITES.md](FLASH_WRITES.md) for delayed-save tradeoffs.

## Historical 3.1.1 startup fix

Game Arcade 3.1.1 for classic ESP32-WROVER N16R8 and SPI ILI9225.

- Fix the startup boot loop by initializing banked frame history after Arduino initializes PSRAM.
- Link a project-local SDK object with premature himem initialization disabled; leave the installed SDK unchanged.
- Verify the linked constructor table in both release and debug builds to prevent this regression.
- Avoid error logs when optional settings or historical NVS namespaces are absent.
- Add a 64 KB coredump partition after the existing A/B firmware slots.
- Preserve all 50 games, game IDs, saved records, Sky naming and compatibility with older OTA clients.
- Align firmware VERSION and release tag. The previous v3.1.0 attempt failed packaging because they differed.

The startup fix was previously exercised on an attached ESP32: boot health passed, physical
16 MB flash / 8 MB PSRAM were detected, 40 banked-history slots were allocated, and a
35-second serial capture showed no errors, aborts or repeated resets. Full hardware
acceptance, gameplay tuning, Wi-Fi/HTTPS and OTA power-loss/rollback checks remain pending.
See docs/VERIFICATION.md and docs/HARDWARE_CHECKLIST.md.

Use arcade-esp32-n16r8.bin for OTA. The factory image is for initial USB/serial installation;
it replaces saved settings. Wiring is documented in docs/HARDWARE.md.
