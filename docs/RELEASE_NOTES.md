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
