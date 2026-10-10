Sly Arcade 2.0.0 for classic ESP32-WROVER N16R8 and SPI ILI9225.

- 25 separate native game modules with an expandable catalog and one shared game arena.
- Eight-button wiring, favorites, score pages, settings, audio and instant restart.
- Removed obsolete controller, sensor, LED, migration and alternate hardware configurations.
- Only release/debug builds; required PSRAM framebuffer and banked last-run frame history.
- Browser Wi-Fi/repository setup and verified GitHub A/B OTA with boot-health rollback.
- Host gameplay/rule tests and both firmware builds are verified. Physical hardware acceptance
  remains required; this release has not been exercised on an attached ESP32.

Rewire according to docs/HARDWARE.md. First installation requires USB/serial bootloader and
partition-table upload. Use arcade-esp32-n16r8.bin for OTA; the factory image is for initial flash.
