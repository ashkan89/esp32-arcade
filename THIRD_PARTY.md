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

The 3.7.0 bitmap overhaul uses four newly generated PNG atlases, retained in `assets/art/`,
with source hashes, inspected crop boundaries and conversion provenance. See
[`assets/art/PROVENANCE.md`](assets/art/PROVENANCE.md). `scripts/pack_art.py` uses Pillow
12.3.0 (HPND license) for build-time conversion; firmware contains packed indexed data and
does not depend on Pillow or an image decoder at runtime.

River Raid now imports terrain, palette and sprite data from athaless/RiverRaid,
commit d6dcebe7e62e41d0a0566cb57694edd34406b7d8 (no upstream license file).
Keystone Kapers executes the user's local ROM, which is ignored along with its generated
header and analysis. Local firmware embeds that cartridge; it is not automatically a
redistributable release. The 6502 core in `include/third_party/m6502.h` is by Andre
Weissflog, under its retained zlib/libpng license. Its pinned revision and hash, source
provenance and compatibility limits are in `docs/ATARI_SOURCE_PORTS.md`.

The 25 v3 additions use original names, native rules and procedural graphics. Classic title
references and researched sources are in `docs/GAME_REFERENCES.md`; they do not confer rights
to those original games or imply that Game Arcade contains licensed commercial ports.
