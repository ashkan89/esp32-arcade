#include "SoundEngine.h"

void SoundEngine::begin(uint8_t pin) {
  buzzerPin = pin;
  ledcAttach(buzzerPin, 1000, 8);  // ESP32 new API
  ledcWrite(buzzerPin, 0);
}

void SoundEngine::enqueue(int f, int d) {
  int next = (qTail + 1) % MAX_QUEUE;
  if (next == qHead) return;
  q[qTail] = { f, d };
  qTail = next;
}

bool SoundEngine::dequeue(ToneEvent& e) {
  if (qHead == qTail) return false;
  e = q[qHead];
  qHead = (qHead + 1) % MAX_QUEUE;
  return true;
}

void SoundEngine::play(int freq, int dur) {
  ledcChangeFrequency(buzzerPin, freq, 8);
  ledcWrite(buzzerPin, 128);
  toneActive = true;
  toneEnd = millis() + dur;
}

void SoundEngine::queue(const ToneEvent* seq, int count) {
  for (int i = 0; i < count; i++) enqueue(seq[i].freq, seq[i].dur);
}

void SoundEngine::update() {
  if (toneActive && millis() >= toneEnd) {
    ledcWrite(buzzerPin, 0);
    toneActive = false;
  }

  if (!toneActive) {
    ToneEvent e;
    if (dequeue(e)) play(e.freq, e.dur);
  }
}

void SoundEngine::shot() {
  ToneEvent s[] = { { 1800, 16 } };
  queue(s, 1);
}

void SoundEngine::explosion() {
  ToneEvent s[] = { { 1200, 18 }, { 800, 18 }, { 500, 24 } };
  queue(s, 3);
}

void SoundEngine::powerup() {
  ToneEvent s[] = { { 1000, 18 }, { 1400, 18 }, { 2000, 28 } };
  queue(s, 3);
}

void SoundEngine::gameOver() {
  ToneEvent s[] = { { 700, 60 }, { 500, 70 }, { 300, 90 } };
  queue(s, 3);
}

void SoundEngine::bossHit() {
  ToneEvent s[] = { { 900, 12 }, { 1400, 12 } };
  queue(s, 2);
}

void SoundEngine::bossRoar() {
  ToneEvent s[] = { { 300, 80 }, { 250, 90 }, { 200, 120 } };
  queue(s, 3);
}

void SoundEngine::menuMove() {
  ToneEvent s[] = { { 1200, 12 } };
  queue(s, 1);
}

void SoundEngine::menuSelect() {
  ToneEvent s[] = { { 1000, 14 }, { 1600, 20 } };
  queue(s, 2);
}

void SoundEngine::ufo() {
  ToneEvent s[] = { { 900, 40 }, { 1100, 40 }, { 1300, 40 } };
  queue(s, 3);
}

void SoundEngine::waveClear() {
  ToneEvent s[] = { { 900, 20 }, { 1200, 20 }, { 1500, 24 }, { 1900, 28 } };
  queue(s, 4);
}