Game Arcade 3.0.0 for classic ESP32-WROVER N16R8 and SPI ILI9225.

- Rename console, Wi-Fi setup, portal and release branding to Game Arcade.
- 50 separate native game modules, including 25 original interpretations of researched classics.
- Twelve-stage campaigns, victory screen and completion bonuses across the catalog.
- Game-specific combat bosses: exposed cores, attack telegraphs, armor and repeated captures.
- Final shooter bosses, sentinel, reactor, bombing-grid, tank and reaction bosses.
- Escalating Grid Merge boards and nine reproducible, constructively solvable Cargo Shift warehouses.
- Fix dropped movement, repeated falling damage, skippable kitchen boss and post-victory generation.
- Preserve stable save IDs, settings schema and v2-compatible OTA identity.
- Focused campaign/objective/boss tests, warehouse solution replays and 1.5 million simulation ticks.
- Linux address/undefined sanitizer CI.
- Eight-button wiring, favorites, score pages, settings, audio and instant restart.
- Removed obsolete controller, sensor, LED, migration and alternate hardware configurations.
- Only release/debug builds; required PSRAM framebuffer and banked last-run frame history.
- Browser Wi-Fi/repository setup and verified GitHub A/B OTA with boot-health rollback.
- Host gameplay/rule tests and both firmware builds are verified. Physical hardware acceptance
  remains required; this release has not been exercised on an attached ESP32.

Rewire according to docs/HARDWARE.md. First installation requires USB/serial bootloader and
partition-table upload. Use arcade-esp32-n16r8.bin for OTA; the factory image is for initial flash.

Classic inspirations are linked in docs/GAME_REFERENCES.md. New games use original code and
procedural graphics; commercial ROMs, downloaded sprites and soundtracks are not included.
Stages vary arcade objectives and difficulty; these are not complete commercial game ports.
This release is software-verified and has not been hardware-certified for production.
