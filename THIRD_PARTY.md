# Dependencies and assets

| Component | Pinned version | License / source |
| --- | --- | --- |
| PlatformIO Core (verification tool) | 6.2.0 | Apache-2.0, https://github.com/platformio/platformio-core |
| Espressif32 PlatformIO platform | 6.10.0 | Apache-2.0, https://github.com/platformio/platform-espressif32 |
| Arduino-ESP32 | Core 2.0.17 / package 3.20017.241212+sha.dcc1105b | LGPL-2.1, https://github.com/espressif/arduino-esp32 |
| ESP-IDF prebuilt framework components | 4.4.7 in selected core | Espressif Apache-2.0 and component-specific notices, https://github.com/espressif/esp-idf |
| TFT_eSPI | 2.5.43 | MIT, https://github.com/Bodmer/TFT_eSPI/blob/master/license.txt |
| XboxSeriesXControllerESP32_asukiaaa (Xbox only) | 1.1.1 | MIT, https://github.com/asukiaaa/arduino-XboxSeriesXControllerESP32 |
| NimBLE-Arduino (Xbox only) | 2.3.6 | Apache-2.0 and bundled Apache Mynewt/component notices, https://github.com/h2zero/NimBLE-Arduino |
| XboxControllerNotificationParser (Xbox only) | 1.0.5 | MIT, https://github.com/asukiaaa/arduino-XboxControllerNotificationParser |
| XboxSeriesXHIDReportBuilder_asukiaaa (Xbox only) | 1.0.1 | MIT, https://github.com/asukiaaa/arduino-XboxSeriesXHIDReportBuilder |
| esptool (optional diagnostic utility) | 4.5.1 | GPL-2.0, https://github.com/espressif/esptool |

Dependency licenses are retained in downloaded packages; distributing a firmware product
requires following applicable notices and source/relinking obligations. No dependency source
is copied into the repository. TFT_eSPI compiles its built-in GLCD font for the migration and
error screen; optional GFX FreeFonts are not enabled.

The original player, alien and UFO sprites in `include/assets/Sprites.h` are copied from the
user-supplied `GameConsole/Sprites.h`. No license was present in the original repository;
ownership/distribution rights for those assets and legacy sources remain with their owner.
They have not been silently relicensed. New menus, procedural icons/scenery, game shapes,
and the compact 5×7 font in `Canvas.cpp` were authored for this implementation and use no
downloaded game art or audio recordings. Game titles are project names, not borrowed brands.

New code in `include/arcade`, `include/games`, `src/arcade`, `src/games`, `src/main.cpp`,
scripts, tests and documentation is provided under `LICENSE-new-code.txt`. This does not
change the licensing of original code, copied sprites, generated migration copies or dependencies.
