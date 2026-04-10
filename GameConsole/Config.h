#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

// #include <WiFi.h>
// #include <SPI.h>
// #include <SD.h>

// #if defined(ESP8266)
// #elif defined(ESP32)
// #endif

// ================= PINS =================
#define BTN_LEFT 32
#define BTN_RIGHT 33
#define BTN_FIRE 25
#define BTN_BACK 26
#define BTN_WEAPON 14
#define BUZZER_PIN 27
#define SIGNAL_LED 16   // LED PIN
#define AMBIENT_LED 12  // LED PIN
#define SD_CS 13        // SD CS PIN

// ================= SCREEN =================
#define SCREEN_W 176
#define SCREEN_H 220

// ================= GAME =================
#define FRAME_MS 16

#define MAX_PBULLETS 18
#define MAX_EBULLETS 40
#define MAX_PARTICLES 80
#define MAX_ENEMIES 24
#define MAX_POWERUPS 4
#define MAX_DIVERS 6
#define MAX_STARS 36
#define MAX_LEADERS 5

#define ENEMY_ROWS 4
#define ENEMY_COLS 6

#define SHIELD_COUNT 3
#define SHIELD_W 20
#define SHIELD_H 12

#define PLAYFIELD_Y 14

// ================= COLORS =================
#define BG_COLOR TFT_BLACK
#define HUD_BG TFT_NAVY
#define STAR1_COLOR 0x4208
#define STAR2_COLOR 0x7BEF
#define STAR3_COLOR TFT_WHITE
#define CRT_LINE 0x0841

#define PLAYER_COLOR TFT_CYAN
#define ENEMY_A_COLOR TFT_GREEN
#define ENEMY_B_COLOR TFT_MAGENTA
#define ENEMY_C_COLOR TFT_RED
#define UFO_COLOR TFT_ORANGE
#define BOSS_COLOR TFT_PURPLE
#define BULLET_COLOR TFT_YELLOW
#define SHIELD_COLOR TFT_BLUE

// ================= ATTRACT =================
#define ATTRACT_TIMEOUT 12000