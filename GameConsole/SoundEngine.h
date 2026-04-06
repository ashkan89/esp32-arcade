#pragma once
#include <Arduino.h>

struct ToneEvent {
  int freq;
  int dur;
};

class SoundEngine {
public:
  void begin(uint8_t pin);
  void update();

  void queue(const ToneEvent* seq, int count);
  void shot();
  void explosion();
  void powerup();
  void gameOver();
  void bossHit();
  void bossRoar();
  void menuMove();
  void menuSelect();
  void ufo();
  void waveClear();

private:
  static const int MAX_QUEUE = 24;
  ToneEvent q[MAX_QUEUE];
  int qHead = 0;
  int qTail = 0;

  uint8_t buzzerPin = 255;
  bool toneActive = false;
  unsigned long toneEnd = 0;

  void enqueue(int f, int d);
  bool dequeue(ToneEvent& e);
  void play(int freq, int dur);
};