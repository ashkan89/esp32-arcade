#pragma once
#include <Arduino.h>

class AudioEngine {
public:
  void begin(uint8_t pin);
  void update();

  void setEnabled(bool en) {
    enabled = en;
  }

  void shot();
  void laser();
  void spread();
  void plasma();
  void explosion();
  void powerup();
  void hit();
  void bossRoar();
  void ufo();
  void menuMove();
  void menuSelect();
  void gameOver();
  void waveClear();

  void startMusic();
  void stopMusic();

private:
  struct ToneEvt {
    uint16_t freq;
    uint16_t dur;
    uint8_t vol;
  };

  static const int QMAX = 64;
  ToneEvt q[QMAX];
  int qh = 0, qt = 0;

  uint8_t buzzerPin = 255;
  bool active = false;
  bool enabled = true;
  unsigned long toneEnd = 0;

  bool musicOn = true;
  unsigned long musicTick = 0;
  uint8_t musicStep = 0;

  void enqueue(uint16_t f, uint16_t d, uint8_t v = 128);
  bool dequeue(ToneEvt &e);
  void play(uint16_t f, uint16_t d, uint8_t v);
};