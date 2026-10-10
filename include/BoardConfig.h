#pragma once
// This header is injected into every firmware translation unit, including TFT_eSPI.
#define USER_SETUP_LOADED
#define ILI9225_DRIVER
#define TFT_WIDTH 176
#define TFT_HEIGHT 220
#define TFT_MOSI 23
#define TFT_MISO -1
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4
#define SPI_FREQUENCY 20000000
#define LOAD_GLCD
#ifndef ARCADE_AUDIO
#define ARCADE_AUDIO 1
#endif
#ifndef ARCADE_ONE_BUTTON
#define ARCADE_ONE_BUTTON 0
#endif
#ifndef ARCADE_EXPANDED
#define ARCADE_EXPANDED 0
#endif
#ifndef ARCADE_FORCE_INTERNAL
#define ARCADE_FORCE_INTERNAL 0
#endif
#ifndef ARCADE_PROFILE
#define ARCADE_PROFILE 0
#endif
#ifndef ARCADE_XBOX
#define ARCADE_XBOX 0
#endif
#ifdef __cplusplus
namespace board {
constexpr int Left = 32, Right = 33, Fire = 25, Weapon = 14;
// GPIO26 is DATA-AH21 in pinout.txt. Never enable a pull-up on it by default.
constexpr int Back = -1;
constexpr int Up = 21, Down = 22, Pause = 34; // expanded only; GPIO34 needs external 10k pull-up
constexpr int Buzzer = 27, SdCs = 13;
// GPIO16/17 are PSRAM signals. GPIO12 is a flash-voltage strap. Neither LED is driven.
constexpr int Width = 176, Height = 220, Rotation = 0;
constexpr unsigned ExpectedFlash = 16u * 1024u * 1024u;
} // namespace board
#endif
