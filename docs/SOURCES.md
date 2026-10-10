# Official references used

These sources were checked alongside the installed dependency source. Installed package
versions and compilation are the compatibility evidence for this repository; current web
documentation may describe newer APIs.

* [PlatformIO ESP-WROVER-KIT board](https://docs.platformio.org/en/latest/boards/espressif32/esp-wrover-kit.html)
  and [6.10.0 board manifest](https://github.com/platformio/platform-espressif32/blob/v6.10.0/boards/esp-wrover-kit.json):
  classic ESP32, 240 MHz, DIO/40 MHz, default 4 MB flash; explicit override is required here.
* [PlatformIO 6.10.0 platform manifest](https://github.com/platformio/platform-espressif32/blob/v6.10.0/platform.json):
  Arduino core 2.0.17 package family and Xtensa 8.4.0 toolchain.
* [PlatformIO partition configuration](https://docs.platformio.org/en/latest/platforms/espressif32.html#partition-tables):
  custom CSV selection and flash configuration.
* [Espressif classic ESP32 external RAM](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-guides/external-ram.html):
  mapping/heap limits, cache restrictions, and unsuitable peripheral DMA buffers in PSRAM.
* [Espressif ESP32 SPI master](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/spi_master.html):
  internal DMA-capable aligned buffers and transfer lifetime requirements for a future DMA
  implementation. This console uses synchronous transfer, not that DMA implementation.
* [WROVER hardware reference](https://documentation.espressif.com/esp-dev-kits/en/latest/esp32/esp-wrover-kit/index.html):
  PSRAM GPIO16/17 and board signal/strap constraints. Carrier-specific assignments still need
  the user's carrier schematic; this repository's pinout determines baseline wiring.
* [Arduino-ESP32 2→3 migration guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html)
  and [core 2.0.17 LEDC source](https://github.com/espressif/arduino-esp32/blob/2.0.17/cores/esp32/esp32-hal-ledc.c):
  core-3 pin-based LEDC APIs differ from the pinned channel-based implementation.
* [TFT_eSPI ILI9225 driver](https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_Drivers/ILI9225_Defines.h):
  portrait dimensions, controller commands and incomplete inversion command. The downloaded
  2.5.43 package's `TFT_Drivers/ILI9225_{Defines,Init,Rotation}.h`, `TFT_eSPI.cpp`, and
  `Processors/TFT_eSPI_ESP32.c` were inspected directly for the actual compiled path.
* [Xbox controller library](https://github.com/asukiaaa/arduino-XboxSeriesXControllerESP32)
  and [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino):
  subscription/report interface and compatible pinned NimBLE 2.x APIs. Pairing and controller
  report behavior require physical testing.

No claim of 16 MB detected flash, 8 MB detected PSRAM, measured FPS, end-to-end latency,
transducer loudness, or carrier-pin availability beyond the source mapping is made.
