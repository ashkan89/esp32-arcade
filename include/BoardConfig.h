#pragma once
#define USER_SETUP_LOADED
#define ILI9225_DRIVER
#define TFT_WIDTH 176
#define TFT_HEIGHT 220
#define TFT_MOSI 23
#define TFT_MISO -1
#define TFT_SCLK 18
#define TFT_CS 13
#define TFT_DC 26
#define TFT_RST 14
#define SPI_FREQUENCY 20000000
#define LOAD_GLCD
#ifndef ARCADE_WROOM
#define ARCADE_WROOM 0
#endif
#ifndef ARCADE_FLASH_MB
#define ARCADE_FLASH_MB 16
#endif
#if ARCADE_WROOM && ARCADE_FLASH_MB == 4
#define ARCADE_BOARD_ID "esp32-wroom-n4-ili9225"
#define ARCADE_BINARY "arcade-esp32-wroom-n4.bin"
#define ARCADE_MANIFEST "arcade-release-wroom-n4.json"
#define ARCADE_IDENTITY "SKY_ARCADE_WROOM_N4|"
#elif ARCADE_WROOM && ARCADE_FLASH_MB == 16
#define ARCADE_BOARD_ID "esp32-wroom-n16-ili9225"
#define ARCADE_BINARY "arcade-esp32-wroom-n16.bin"
#define ARCADE_MANIFEST "arcade-release-wroom-n16.json"
#define ARCADE_IDENTITY "SKY_ARCADE_WROOM_N16|"
#elif !ARCADE_WROOM && ARCADE_FLASH_MB == 16
#define ARCADE_BOARD_ID "esp32-wrover-n16r8-ili9225"
#define ARCADE_BINARY "arcade-esp32-n16r8.bin"
#define ARCADE_MANIFEST "arcade-release.json"
#define ARCADE_IDENTITY "SKY_ARCADE_N16R8|"
#else
#error Unsupported arcade memory configuration
#endif
#ifndef ARCADE_PROFILE
#define ARCADE_PROFILE 0
#endif
#ifdef __cplusplus
namespace board {
constexpr int Left = 25, Right = 21, Up = 32, Down = 33;
// Menu/Back use strapping pins: release both buttons during power-on/reset.
constexpr int Fire = 22, Weapon = 4, Pause = 5, Back = 15;
constexpr int Buzzer = 27;
constexpr int Width = 176, Height = 220, Rotation = 0;
constexpr unsigned ExpectedFlash = ARCADE_FLASH_MB * 1024u * 1024u;
constexpr unsigned ExpectedPsram = ARCADE_WROOM ? 0u : 8u * 1024u * 1024u;
} // namespace board
#endif
