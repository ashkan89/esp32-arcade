# Full-button N16R8 wiring

This replaces the old wiring. Use a classic ESP32-WROVER module with 16 MB flash and 8 MB PSRAM,
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
| B / secondary | 19 | Button to ground, internal pull-up |
| Menu / pause | 34 | Button to ground; external 10 kΩ to 3.3 V |
| Back | 35 | Button to ground; external 10 kΩ to 3.3 V |
| Passive piezo | 27 | Small low-current piezo through 100–220 Ω; other terminal ground |

GPIO34/35 are input-only and have no internal pull-ups. Do install both external resistors.
TFT MISO is unused. The framebuffer is portrait 176×220, rotation 0, RGB565, at 20 MHz SPI.
Power/backlight connections depend on the actual display carrier: follow its voltage rating;
do not feed a raw high-current backlight from a GPIO. Use a regulated supply with enough current
for the ESP32's Wi-Fi peaks and the display. Provide 100 nF local decoupling and a 100–470 μF
bulk capacitor near the module; keep SPI wires short.

GPIO6–11 connect to flash; GPIO16/17 connect to WROVER PSRAM. No peripheral uses them.
The new map avoids boot-strapping GPIO0/2/5/12/15. The sensor, signal LED, ambient LED, SD card,
and Xbox controller are removed. Do not retain their old connections on reassigned pins.

An electrodynamic speaker or larger transducer needs a driver/amplifier, not direct GPIO drive.
The firmware uses one LEDC channel for passive-piezo effects and menu music, with a mute setting.

## Memory use

Mapped PSRAM holds two 77,440-byte RGB565 buffers and temporary bounded release JSON buffers.
The classic ESP32 cannot directly map the whole 8 MB at once. The SDK reserves a bank-mapping
window; the himem API allocates approximately 3.75 MB for up to 40 visual-history slots.
Only one 32 KB bank is mapped during copies; pointers never survive an unmap.
Game state, the 8 KB game arena, the SPI staging strip and task stacks remain in internal RAM.
Startup logs distinguish physical PSRAM, mapped heap, and banked-history allocation.
The history cache is optional on allocation failure; the PSRAM framebuffer is required.
