# Dependencies and assets

| Component | Pinned version | License / source |
| --- | --- | --- |
| PlatformIO Core | 6.2.0 | Apache-2.0, https://github.com/platformio/platformio-core |
| Espressif32 PlatformIO platform | 6.10.0 | Apache-2.0, https://github.com/platformio/platform-espressif32 |
| Arduino-ESP32 | 2.0.17 | LGPL-2.1, https://github.com/espressif/arduino-esp32 |
| ESP-IDF prebuilt components | 4.4.7 | Apache-2.0 / component licenses, https://github.com/espressif/esp-idf |
| TFT_eSPI | 2.5.43 | MIT, https://github.com/Bodmer/TFT_eSPI |
| ArduinoJson | 6.21.5 | MIT, https://github.com/bblanchon/ArduinoJson |
| esptool | 4.5.1 | GPL-2.0, https://github.com/espressif/esptool |

Downloaded packages retain their dependency licenses. Distribution must respect their notices
and source/relinking obligations. The firmware links the SDK's default Mozilla CA certificate
bundle; that bundle is part of the pinned framework distribution, not generated from local secrets.

The player, alien and UFO sprites in `include/assets/Sprites.h` came from the user's original
project. No original license was supplied; their ownership/rights are preserved. New code,
procedural art, menus, font, scripts, tests and documentation use `LICENSE-new-code.txt`.
No downloaded commercial game graphics or audio recordings are included.

Keystone Kapers and River Raid are native recreations with newly authored pixel masks.
Activision's manuals and the annotated River Raid disassembly were consulted for rules
and feedback-generator behavior. No cartridge ROM or disassembly is shipped. Their names
refer to the original games; these additions are not represented as licensed commercial
ports. See `docs/ATARI_RESEARCH.md` for sources and remaining fidelity differences.

The 25 v3 additions use original names, native rules and procedural graphics. Classic title
references and researched sources are in `docs/GAME_REFERENCES.md`; they do not confer rights
to those original games or imply that Game Arcade contains licensed commercial ports.
