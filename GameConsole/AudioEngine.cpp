#include "AudioEngine.h"

void AudioEngine::begin(uint8_t pin) {
  buzzerPin = pin;
  ledcAttach(buzzerPin, 1000, 8);
  ledcWrite(buzzerPin, 0);
}

void AudioEngine::enqueue(uint16_t f, uint16_t d, uint8_t v) {
  int n = (qt + 1) % QMAX;
  if (n == qh) return;
  q[qt] = { f, d, v };
  qt = n;
}

bool AudioEngine::dequeue(ToneEvt &e) {
  if (qh == qt) return false;
  e = q[qh];
  qh = (qh + 1) % QMAX;
  return true;
}

void AudioEngine::play(uint16_t f, uint16_t d, uint8_t v) {
  if (!enabled) return;
  ledcChangeFrequency(buzzerPin, f, 8);
  ledcWrite(buzzerPin, v);
  active = true;
  toneEnd = millis() + d;
}

void AudioEngine::update() {
  if (active && millis() >= toneEnd) {
    ledcWrite(buzzerPin, 0);
    active = false;
  }

  if (musicOn && millis() - musicTick > 180) {
    musicTick = millis();
    const uint16_t notes[] = { 262, 330, 392, 330, 294, 349, 440, 349 };
    enqueue(notes[musicStep % 8], 40, 35);
    musicStep++;
  }

  if (!active) {
    ToneEvt e;
    if (dequeue(e)) play(e.freq, e.dur, e.vol);
  }
}

void AudioEngine::startMusic() {
  musicOn = true;
}
void AudioEngine::stopMusic() {
  musicOn = false;
}

void AudioEngine::shot() {
  enqueue(1800, 16, 120);
}
void AudioEngine::laser() {
  enqueue(2200, 24, 140);
  enqueue(1400, 14, 100);
}
void AudioEngine::spread() {
  enqueue(1200, 12, 110);
  enqueue(1500, 12, 110);
  enqueue(1800, 12, 110);
}
void AudioEngine::plasma() {
  enqueue(700, 18, 120);
  enqueue(1200, 20, 140);
}
void AudioEngine::explosion() {
  enqueue(1200, 18, 130);
  enqueue(800, 18, 120);
  enqueue(500, 28, 100);
}
void AudioEngine::powerup() {
  enqueue(1000, 18, 120);
  enqueue(1400, 18, 120);
  enqueue(2000, 28, 120);
}
void AudioEngine::hit() {
  enqueue(900, 14, 110);
}
void AudioEngine::bossRoar() {
  enqueue(300, 80, 140);
  enqueue(240, 100, 140);
  enqueue(180, 120, 140);
}
void AudioEngine::ufo() {
  enqueue(900, 40, 110);
  enqueue(1100, 40, 110);
  enqueue(1300, 40, 110);
}
void AudioEngine::menuMove() {
  enqueue(1200, 12, 100);
}
void AudioEngine::menuSelect() {
  enqueue(1000, 14, 120);
  enqueue(1600, 20, 120);
}
void AudioEngine::gameOver() {
  enqueue(700, 60, 130);
  enqueue(500, 70, 120);
  enqueue(300, 90, 110);
}
void AudioEngine::waveClear() {
  enqueue(900, 20, 120);
  enqueue(1200, 20, 120);
  enqueue(1500, 24, 120);
  enqueue(1900, 28, 120);
}