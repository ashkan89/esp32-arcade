# Verification — 2026-10-10

Software verification was performed in the shared Windows workspace. No serial device was
reported by `pio device list`, so no upload, physical capacity confirmation, panel test,
audio test, radio pairing, or hardware FPS/latency measurement is claimed.

## Reproduce

```sh
python scripts/verify.py
```

This builds `migration`, `release`, `debug`, `no_psram`, `one_button`, `expanded`, and `xbox`,
then compiles/runs the real pure-C++ rules and generates `artifacts/console-preview.png`.
Logs: `test-results/firmware-builds.log`, `host-build.log`, and `host-tests.log`.

## Verified dependency set

* PlatformIO Core 6.2.0, Espressif32 platform 6.10.0.
* Framework package 3.20017.241212+sha.dcc1105b: Arduino-ESP32 2.0.17, ESP-IDF 4.4.7.
* Xtensa ESP32 GCC 8.4.0+2021r2-patch5; classic ESP32 target, 16 MB image header,
  DIO/40 MHz flash, 3 MB application partition limit.
* TFT_eSPI 2.5.43, inspected ILI9225 definitions/init/rotation/window/SPI implementation.
* Xbox build: XboxSeriesXControllerESP32_asukiaaa 1.1.1, NimBLE-Arduino 2.3.6,
  XboxControllerNotificationParser 1.0.5, XboxSeriesXHIDReportBuilder_asukiaaa 1.0.1.
* Host rules: MinGW GCC 5.1.0, C++14, `-O2 -Wall -Wextra -Wpedantic`. Final host compilation
  produced no diagnostics. No Arduino mocks, hardware substitutions, or copied test rules.

## Firmware build results

All seven environments compile, link, pass PlatformIO size checks, and produce ESP32 images.
The only expected library warning is missing `TOUCH_CS`; no touch hardware is configured.

| Environment | Result | Static RAM bytes | Flash bytes |
| --- | --- | ---: | ---: |
| `migration` | PASS | 28,368 | 355,569 |
| `release` | PASS | 48,668 | 364,189 |
| `debug` | PASS | 48,668 | 374,909 |
| `no_psram` | PASS | 48,548 | 357,429 |
| `one_button` | PASS | 48,668 | 364,129 |
| `expanded` | PASS | 48,668 | 364,253 |
| `xbox` | PASS | 63,588 | 710,205 |

Static RAM figures exclude startup framebuffer allocations, stacks, NVS/driver allocation,
and BLE runtime heap. They are link-time footprint figures, not measured free heap. NVS and
two 3 MB app partitions end at `0x610000`; the remaining advertised flash is deliberately
unallocated. There is no OTA service despite reserving a second application partition.

## Host test results

**PASS: 172,177 assertions**, including 160,000 simulation ticks and **708 game-over/restart
cycles**. The rule tests cover:

* unsigned timer rollover, deadline comparison, 30/60 render-independent fixed-step count,
  bounded catch-up, dropped-time reporting;
* debounce/bounce, press/release, repeat, one-shot long hold, and release-gated suppression
  including suppression during a pending debounce;
* AABB, swept-segment and circular collision boundaries, degenerate segments, asteroid corner
  rejection, single-bullet consumption, shields with empty cells, grace period after damage;
* laser double-shot, expiring buffs, fifth-wave boss, wave-clear/next-wave progression,
  large-asteroid splitting and full entity-pool bounds, Sky bomb and cleanup;
* paddle bounce/minimum horizontal speed, brick clear progression, loss/serve reset;
* snake growth/scoring, food placement in all three layouts, queued-turn limit, vacated tail,
  wall collision, completely filled board termination;
* safe lane in 200 generated rows, collectible score, simultaneous hazard damage grace;
* flap gravity/press-edge behavior, score once per gate, ceiling collision;
* settings round-trip, every individual byte corrupted, CRC/version/range rejection,
  unchanged output on invalid decode, scores, leaderboard order, invalid game ID;
* launcher/instructions/play/pause/controls/exit/settings transitions and one-button menus;
* eight games × both control modes × 10,000 ticks, repeated start/cleanup, score reset,
  bounded entities, pause freezing, rendering/clipping guards in RGB565 and RGB332.

The count includes repeated guard/limit assertions from the soak; it is not a count of
172,177 independent scenarios. The preview comes from the same render implementations and
was visually inspected for menu readability, clipping and original sprite preservation.
Sanitizer runtimes were unavailable in this host compiler; sanitizer execution is not claimed.

## Known limits / remaining evidence

* Advertised 16 MB flash / 8 MB PSRAM are **configured, not physically confirmed**. Startup
  reads the physical flash and PSRAM chip size separately from mapped heap. The TFT dimension
  check reads library dimensions, not a controller ID; physical display verification remains.
* SPI uses a conservative 20 MHz request. Framebuffer/dirty-tile allocation fallbacks and
  transfer calibration compile, but their actual memory/timing values require boot logs.
  The 30 FPS default and optional 60 FPS cap are targets, not measured achieved performance.
* Software latency counters exclude electrical debounce history and panel scanout. Sustained
  render overload drops bounded simulation catch-up rather than making unsafe timing promises.
* Xbox discovery/reconnect and GATT vibration can block menus. GPIO play stays independent;
  report format, periodic hold reports, pairing and reconnection remain hardware tests.
* GPIO26 wiring remains ambiguous and is intentionally unused. GPIO16/12 legacy LED features
  are disabled for WROVER/strap compatibility; physically conflicting wiring still needs fixing.
* Original sprites have no repository license declaration. Original sources and assets are
  preserved with their existing ownership; see `THIRD_PARTY.md`.
* No battery reading, SD assets, OTA update service, DAC/PCM playback or arbitrary buzzer volume
  exists. The old OTA label was nonfunctional. All core eight-game entries are implemented.
* One-button Meteor is a horizontal survival shooter instead of rotation/thrust; Breaker uses
  sweep/reverse controls; Sky's bomb requires the existing Special button. These variants keep
  control demands practical for the available pins.
* Saving is deferred to menus. Power loss before the serial save confirmation can lose recent
  in-memory records/settings. `HARDWARE_CHECKLIST.md` defines the device acceptance work.
