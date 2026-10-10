# Sly Arcade — ESP32-WROVER / ILI9225

A PlatformIO Arduino console with eight playable games, shared menus, passive-buzzer sound,
saved scores, and runtime diagnostics. The original `GameConsole/` files and `pinout.txt` are
preserved. The `migration` environment compiles a converted copy of the original game;
`release` builds the new console.

![Actual C++ renderer preview](artifacts/console-preview.png)

## Build and upload

Install Python and PlatformIO Core:

```sh
python -m pip install platformio==6.2.0
pio run -e release
pio device list
```

**Check the physical flash before the first upload.** The configuration targets the advertised
classic ESP32-WROVER with 16 MB flash; no device was connected during development. Install
the matching diagnostic utility and replace `COM7` with the port from `pio device list`:

```sh
python -m pip install esptool==4.5.1
python -m esptool --chip esp32 --port COM7 flash_id
pio run -e release -t upload --upload-port COM7
pio device monitor -e release --port COM7
```

On Linux/macOS, use the detected serial device instead of `COM7`. If flash detection differs
from 16 MB, correct `board_upload.flash_size` and the partition table before uploading.
The firmware reports the JEDEC flash capacity, configured image capacity, detected PSRAM,
mapped PSRAM heap, internal heap, chip revision, and reset reason at 115200 baud. Physical
8 MB PSRAM does **not** imply an ordinary 8 MB allocation heap on classic ESP32.

All pins and display setup are centralized in [BoardConfig.h](include/BoardConfig.h), injected
into firmware and library compilation. The selected board is `esp-wrover-kit`, with explicit
16 MB flash overrides, DIO at 40 MHz, and PSRAM cache workaround. This is a module-oriented
configuration, not a claim that your carrier is Espressif's full WROVER-KIT development board.
ILI9225 uses portrait rotation 0, 176×220 pixels, SPI at 20 MHz. TFT_eSPI **2.5.43 includes its
ILI9225 driver**; it was inspected and compiled. No external Arduino `User_Setup.h` is required.

## Configurations and verification

| Environment | Purpose |
| --- | --- |
| `migration` | Original Sly/Space Invaders conversion, physical buttons, core-2 LEDC fixes |
| `release` | Eight-game baseline, four existing buttons, GPIO27 sound, radios off |
| `debug` | Debug symbols and the same runtime profiling |
| `no_psram` | Force internal framebuffer allocation; compile audio out |
| `one_button` | GPIO25 only; complete simplified controls for all games |
| `expanded` | Optional up/down on GPIO21/22 and pause on GPIO34 |
| `xbox` | Opt-in Xbox Series BLE controller plus existing physical controls |

```sh
python scripts/verify.py
# Or run separately:
pio run -e migration -e release -e debug -e no_psram -e one_button -e expanded -e xbox
python scripts/test_host.py
python scripts/preview.py
```

Host tests require GCC/Clang with C++14 or newer, or MSVC. Set `CXX` if compiler discovery
needs help; the helper also finds PlatformIO's Windows MinGW installation. Firmware uses
C++17, while the rules deliberately stay within the common C++14 subset. Optional
`python scripts/test_host.py --sanitize` requires a compiler with working sanitizer runtimes;
sanitizer execution was not available in the development toolchain. Formatting uses the
checked-in `.clang-format`; clang-format is optional for builds.

Exact dependency pins and build results are in [verification](docs/VERIFICATION.md).
Tests and build logs live in `test-results/`. Preview images come from the game renderer,
not an illustration of hypothetical behavior.

## Existing wiring

Buttons connect their GPIO to **GND** when pressed; the baseline uses internal pull-ups.
Keep the existing display voltage and backlight wiring appropriate to its module.

| Signal | GPIO | Baseline behavior |
| --- | ---: | --- |
| TFT MOSI / SDA | 23 | Existing SPI output |
| TFT SCK / CLK | 18 | Existing SPI clock |
| TFT CS | 5 | Existing chip select; boot strap, no added external pull-down |
| TFT RS / DC | 2 | Existing command/data; boot strap, preserve wiring |
| TFT RESET | 4 | Existing reset |
| Left | 32 | Existing button |
| Right | 33 | Existing button |
| Fire / Confirm | 25 | Existing button |
| Weapon / Special | 14 | Existing button; acts as Back in console menus |
| Buzzer | 27 | Existing sound output; verify transducer type/current |
| SD CS | 13 | Held HIGH to deselect optional card; SD storage not required |
| SD MISO | 19 | Reserved by original wiring; unused by console |
| DATA-AH21 | 26 | **Untouched**: wiring note conflicts with code's BTN_BACK definition |
| Signal LED | 16 | **Not driven**: WROVER PSRAM connection |
| Ambient LED | 12 | **Not driven**: flash-voltage boot strap |

GPIO16/17 are module PSRAM signals. An existing LED physically connected to GPIO16 can
still interfere even when firmware does not drive it: disconnect that LED from the PSRAM
signal if it is actually connected there. The original ambient/signal LED features and
placeholder OTA screen are documented in the [audit](docs/AUDIT.md); the new console does
not claim unsupported OTA or battery measurements.

## Controls

Baseline: **Left/Right** select menu items or move; **Fire** confirms or shoots;
**Weapon** returns from menus, changes shooter weapons, or uses Sky's bomb.
Press **Left+Right together** to pause in a game or go back in a menu. GPIO26 is not needed.
In pause, select Resume, Restart, Controls, or Exit using Left/Right and Fire. Fire immediately
replays from game over; Weapon returns to the launcher. New screens suppress held controls
until release. There is a skippable splash and a non-scoring Sly demo after 20 seconds idle.

One-button: tap/release Fire to select the next menu item; hold 700 ms to confirm. A tap after
the eighth game opens the system menu. In gameplay, tap Fire for the action below; hold it
to pause. Game over: hold to replay or tap to exit. Expanded mode adds directional buttons;
Xbox maps D-pad, A confirm/fire, X weapon/special, B back/pause, Start/Xbox pause. Pair or
reconnect the controller in a menu; discovery deliberately does not run during gameplay.

| Game | Baseline / expanded controls | One-button variant | Progression |
| --- | --- | --- | --- |
| Sly Invaders | L/R move, Fire shoot, Weapon cycles laser/spread/plasma | Auto-fire and horizontal sweep; tap reverses direction | 24-alien waves, divers, UFO beam warning, boss every 5 waves, destructible cover, five power-ups |
| Sky Fighters | L/R move, Fire shoot, Weapon uses one bomb per wave; expanded U/D moves vertically | Auto-fire; tap reverses sweep; bomb unavailable | Eight squadrons with three patterns, scrolling river, upgrades/shield, boss every 4 waves |
| Star Assault | L/R move, Fire shoot, Weapon cycles guns | Auto-fire; tap reverses sweep | Curved dives return to formation, kill-chain multiplier, clean-stage bonus, bosses |
| Meteor Patrol | L/R move, Fire shoot, Weapon cycles guns | Auto-fire; tap reverses sweep | Large→medium→small splits, increasing 15-second survival pressure, kill multiplier |
| Neon Breaker | L/R paddle, Fire serves | Paddle sweeps automatically; tap reverses direction; automatic serve | Brick patterns/armored bricks, wide paddle/slow ball/extra life drops, escalating stages |
| Snake Rush | L/R relative turns; Fire clockwise; expanded D-pad absolute directions | Tap clockwise | Three selectable layouts, growth, faster movement, full-board completion bonus |
| Turbo Dodge | L/R lane changes | Tap cycles three lanes | Safe lane in every obstacle row, coins, three lives, speed increases every 10 seconds |
| Flap Escape | Tap Fire to flap | Same control | Fairly spaced gates, increasing speed, narrower gaps, one point per pass |

Shooter drops: **R** rapid fire, **L** extra life, **S** shield, **D** double shot, **W** weapon
upgrade. Neon drops: **W** wide paddle, **S** slow ball, **L** extra life. Every game has saved
records, instructions, pause, restart, game over, and cleanup. Per-game best scores and Sly's
top five are available in the system menu. Settings save difficulty, mute, reduced effects,
snake layout, attract mode, software color inversion, requested FPS, and last started game.

Before removing power after changing settings or setting a record, remain in a menu for at
least **six seconds** and look for `Settings and records saved` in serial output. Writes are
deferred and skipped during play. Invalid version/checksum/value ranges recover to defaults;
first use imports records and useful settings from the original `invadersv4` NVS namespace.

## Rendering, reliability, and hardware checks

The renderer uses RGB565 in mapped PSRAM when available, then internal RGB565, then
RGB332 (38,720 bytes) if necessary. An optional previous-frame snapshot enables exact
16×8 dirty tiles. Boot measures full-frame and tile transfer time and chooses a crossover;
dense frames use strips. Without snapshot memory, CRC-based dirty strips have a periodic
full refresh. The synchronous TFT_eSPI SPI path uses a 2,816-byte internal staging buffer.
No PSRAM pointer is submitted to SPI DMA; no DMA path or second display task is enabled.

Simulation runs at 60 fixed steps/second, independently of requested 30/60 render FPS.
Catch-up is limited to four steps, with dropped time reported. Default rendering targets 30
FPS; 60 is capped when boot timing or actual render duration exceeds its budget. **Actual
FPS and latency remain unmeasured without the hardware.** SPI bandwidth, panel tolerance,
the carrier's real flash/PSRAM, image orientation, and buzzer behavior need device checks.

Every 30 seconds serial output reports update/render maxima, frame interval buckets,
transfer duration, input polling gaps, debounced-edge-to-update delay, internal/PSRAM free
heap, minimum internal heap, and dropped simulation time. These are software timings;
button-to-photon latency requires an external measurement. Display/storage allocation
failures are reported; NVS failure leaves scores volatile; display allocation failure shows
an error and safely halts the console. Wi-Fi stays uninitialized, and BLE exists only in `xbox`.

See [hardware additions](docs/HARDWARE.md), [device checklist](docs/HARDWARE_CHECKLIST.md),
[architecture / adding games](docs/ARCHITECTURE.md), [licensing](THIRD_PARTY.md), and
[official references](docs/SOURCES.md). No SD card, DAC, new display, OTA service, or battery
sensor is required.
