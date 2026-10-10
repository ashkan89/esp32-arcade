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
#ifndef ARCADE_PROFILE
#define ARCADE_PROFILE 0
#endif
#ifdef __cplusplus
namespace board {
constexpr int Left = 25, Right = 21, Up = 32, Down = 33;
constexpr int Fire = 22, Weapon = 19, Pause = 34, Back = 35;
constexpr int Buzzer = 27;
constexpr int Width = 176, Height = 220, Rotation = 0;
constexpr unsigned ExpectedFlash = 16u * 1024u * 1024u;
} // namespace board
#endif
