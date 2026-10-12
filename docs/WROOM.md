# ESP32-WROOM without PSRAM

Use `wroom-4mb` for physical 4 MB flash or `wroom-16mb` for physical 16 MB flash.
These target classic ESP32, retain all 52 games, and use the existing ILI9225/button/buzzer
wiring in [HARDWARE.md](HARDWARE.md). Flash capacity must match the environment exactly.
Neither target defines `BOARD_HAS_PSRAM` or links the banked PSRAM frame-history code.
The WROVER `release` and `debug` targets retain RGB565. Frame history is removed from all targets.

```sh
pio run -e wroom-4mb
pio run -e wroom-4mb -t upload --upload-port YOUR_PORT
python scripts/package_release.py --env wroom-4mb
# Substitute wroom-16mb for physical 16 MB flash.
```

First installation must use serial to install the rollback-enabled bootloader and the
matching partition table. OTA cannot convert a WROVER installation or change flash layout.
4 MB targets have two `0x1f0000` (1984 KiB) application slots; 16 MB targets have two
`0x300000` (3 MiB) slots. Both retain redundant OTA data and NVS. Release packaging checks
the compiled partitions, image header, board/version marker and slot size before producing
separate board-specific application, manifest and factory assets. CI builds and packages
all targets. No hardware is flashed by the verification scripts.

## Memory investigation

16 MB flash does not increase internal RAM. Both WROOM variants use the same memory policy.

| Allocation | WROOM policy |
| --- | --- |
| Framebuffer | 38,720 bytes internal RGB332 instead of 77,440 bytes RGB565 |
| Display staging | 2,816 bytes internal (eight RGB565 scanlines), synchronous SPI |
| Previous-frame snapshot | Disabled; CRC strip comparison and periodic full refresh |
| Frame history | Disabled; no replacement history allocation in internal RAM |
| Game state | Existing single 8,192-byte static arena shared by all games |
| OTA worker | 10,240-byte task stack, allocated only for check/install |
| Release metadata | 16,384-byte JSON document, only selected fields retained |
| Manifest | 2,048-byte JSON document, after release document is destroyed |
| HTTP body reader | 512 bytes inside the worker stack; no full-body allocation |
| HTTP buffers | 1,024-byte receive and 4,096-byte transmit buffers |
| Firmware transfer | 4,096-byte buffer; image streamed to inactive flash slot |

The former updater reserved a 65,536-byte PSRAM JSON document and a 131,073-byte response
buffer for GitHub discovery. It could not run without PSRAM. Filtered stream parsing now
discards descriptions and other unused metadata while reading. It owns parsed strings,
also fixing the old lifetime bug where zero-copy JSON referenced a freed response buffer.
The response deadline, byte limits, HTTP completeness, trailing-content validation and
JSON capacity checks reject malformed/truncated/oversized responses without switching boot slots.
TX remains 4 KB because GitHub signed redirect URLs exceed the SDK's default 512-byte buffer.

Wi-Fi, TLS, lwIP, certificates, URL strings, NVS and SDK tasks require additional dynamic
RAM. These are not included in PlatformIO's static RAM report. HTTPS connections remain
sequential, and release parsing/document storage is freed before downloading the manifest.
Full CA/hostname verification, SHA-256, chip/flash header checks, board/version markers,
inactive-slot writes and startup rollback remain enabled.

## Physical acceptance checks

Local firmware builds and release packaging passed for both WROOM targets:

| Target | Static RAM | Application binary | Slot capacity |
| --- | --- | --- | --- |
| `wroom-4mb` | 61,448 bytes | 1,261,072 bytes | 2,031,616 bytes |
| `wroom-16mb` | 61,448 bytes | 1,261,088 bytes | 3,145,728 bytes |

Symbol inspection found no `esp_himem` initialization in either WROOM image. The host
suite passed 2,223,249 assertions, including buffered JSON regression tests and the
2,080,000-tick game lifecycle soak. The 41,536-byte display heap allocation and all
network/SDK heap allocations are additional to the static RAM values above.

Build/host tests cannot establish runtime heap margin or prove a physical WROOM is reliable.
Before treating either target as hardware-verified:

1. Boot the matching target with no PSRAM; verify detected flash, RGB8 renderer, saved
   settings and boot-health confirmation. Exercise every game, menus, sound and RGB332 legibility.
2. Provision Wi-Fi, save settings, reconnect and check a real GitHub release. Repeat setup,
   check and failed-download sessions at least twenty times to expose leaks/fragmentation.
3. Record `[https]` free/largest-block logs and `[network] Done` free/minimum/largest and
   worker stack-unused logs. Check for allocation failures, shrinking recovered heap, stack
   exhaustion, watchdog resets or display stalls. SDK/TLS peak memory must be measured on-device.
4. Install a newer board-specific release with all three boards' assets present. Verify
   hash/image validation, restart, boot confirmation and settings preservation.
5. Exercise wrong-board/wrong-size/bad-hash releases and Wi-Fi interruption. Confirm the
   running firmware remains bootable. Cut power during download, then reset a pending new
   image before acceptance, and verify automatic rollback. Test explicit previous-firmware boot.

References: [ArduinoJson filtering and streaming](https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/),
[ESP-IDF OTA and rollback](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/ota.html).
