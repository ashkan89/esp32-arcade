# Existing full-button ESP32 wiring

The checked-in configuration targets a classic ESP32-WROVER module with 16 MB flash and 8 MB PSRAM,
or an ESP32-WROOM with 4 MB / 16 MB flash and no PSRAM,
an SPI ILI9225 TFT, eight momentary buttons, and a small passive piezo transducer.
All signals use 3.3 V logic; grounds are common. Configuration lives in `include/BoardConfig.h`.

| Device / signal | ESP32 GPIO | Connection |
| --- | --- | --- |
| TFT clock | 18 | ILI9225 SCK / CLK |
| TFT data | 23 | ILI9225 MOSI / SDI |
| TFT chip select | 13 | CS |
| TFT command/data | 26 | DC / RS / A0 |
| TFT reset | 14 | RST / RESET |
| D-pad Up | 32 | Button to ground, internal pull-up |
| D-pad Down | 33 | Button to ground, internal pull-up |
| D-pad Left | 25 | Button to ground, internal pull-up |
| D-pad Right | 21 | Button to ground, internal pull-up |
| A / action | 22 | Button to ground, internal pull-up |
| B / secondary | 4 | Button to ground, internal pull-up |
| Menu / pause | 5 | Button to ground, internal pull-up; release during power-on/reset |
| Back | 15 | Button to ground, internal pull-up; release during power-on/reset |
| Passive piezo | 27 | Small low-current piezo through 100–220 Ω; other terminal ground |

All eight buttons use internal pull-ups; no external button pull-up resistors are required.
An earlier repository revision moved B, Menu and Back from GPIO19/34/35 to GPIO4/5/15.
The October 28 redesign preserves the current GPIO4/5/15 map; it does not request another move.
GPIO34/35 are unused. Historical wiring is not evidence of the currently fitted hardware;
confirm the revision before uploading to an unidentified console.
GPIO5/15 are strapping pins: keep Menu and Back released during power-on/reset to preserve
the default startup settings. Internal button pull-ups are enabled by the firmware after boot.
TFT MISO is unused. The framebuffer is portrait 176×220, rotation 0, RGB565 on WROVER
or internal RGB565 on WROOM when memory permits, with RGB332 compact/fallback.
Both transfer RGB565 pixels at 20 MHz SPI.
Power/backlight connections depend on the actual display carrier: follow its voltage rating;
do not feed a raw high-current backlight from a GPIO. Use a regulated supply with enough current
for the ESP32's Wi-Fi peaks and the display. Provide 100 nF local decoupling and a 100–470 μF
bulk capacitor near the module; keep SPI wires short.

GPIO6–11 connect to flash; GPIO16/17 connect to WROVER PSRAM. No peripheral uses them.
The map avoids boot-strapping GPIO0/2/12 and uses GPIO5/15 as described above.
The sensor, signal LED, ambient LED, SD card,
and Xbox controller are removed. Do not retain their old connections on reassigned pins.

An electrodynamic speaker or larger transducer needs a driver/amplifier, not direct GPIO drive.
The firmware uses one LEDC channel for passive-piezo effects and menu music, with a mute setting.

## Memory use

Mapped PSRAM holds two 77,440-byte RGB565 buffers and temporary bounded release JSON buffers.
The classic ESP32 cannot directly map the whole 8 MB at once. The SDK reserves a bank-mapping
window, but the application no longer allocates banked frame history.
Game state, the 8 KB game arena, the SPI staging strip and task stacks remain in internal RAM.
Startup logs distinguish physical PSRAM and mapped heap.
WROVER requires the PSRAM framebuffer. Persistence keeps a 2,364-byte saved-state snapshot in RAM
to skip unchanged flash writes. See FLASH_WRITES.md.
WROOM prefers one 77,440-byte internal RGB565 framebuffer while preserving a 128 KiB reserve,
and uses 38,720-byte RGB332 for network screens or allocation fallback. There is no
previous-frame snapshot or history. Format changes occur at screen edges, not per frame.
See [WROOM memory analysis](WROOM.md) for OTA RAM budgets and physical checks.

## COM9 artwork validation

The 3.7.0 artwork run passed all 52 automated game rendering exercises and ten framebuffer format transitions on the 4 MB board. The existing Arduino loop task uses 12 KiB of stack; the measured minimum headroom was 4,460 bytes. See [full evidence and limitations](MODERN_GRAPHICS.md).
