#pragma once
#include "AudioSequencer.h"
#include "BoardConfig.h"
#include "Canvas.h"
#include "Core.h"
#include <Arduino.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
namespace arcade {
class Display {
  TFT_eSPI tft;
  void *memory = nullptr;
  void *previous = nullptr;
  uint16_t *strip = nullptr;
  std::array<uint32_t, 28> hashes{};
  bool invalid = true;
  bool inverted = false;
  uint32_t presents = 0;

public:
  Canvas canvas;
  void (*service)() = nullptr;
  uint32_t fullFrameUs = 0, tileUs = 0, lastTransferUs = 0;
  uint8_t maxFps = 60;
  bool begin();
  void invalidate() {
    invalid = true;
  }
  void invert(bool enabled);
  void present(bool force = false);
  void error(const char *message);
};
class Audio {
  bool ready = false;
  uint16_t pitch = 0;

public:
  AudioSequencer sequencer;
  uint32_t maxCostUs = 0;
  void begin();
  void update(uint32_t now);
  void play(Sound effect, uint32_t now);
  void stop();
};
class Storage {
  Preferences prefs;
  bool ready = false, dirty = false;
  uint32_t changed = 0, lastAttempt = 0;

public:
  Settings settings;
  void begin();
  void mark(uint32_t now) {
    dirty = true;
    changed = now;
  }
  void update(uint32_t now, bool safe);
  bool available() const {
    return ready;
  }
};
struct Diagnostics {
  uint32_t chipFlash = 0, psramPhysical = 0, mappedPsram = 0, frames = 0, updates = 0;
  uint32_t updateMax = 0, drawMax = 0, transferMax = 0, renderMax = 0, frameMax = 0, lastReport = 0,
           maxInputGap = 0, pressToUpdateMax = 0;
  std::array<uint32_t, 6> frameBuckets{};
  uint32_t windowUs = 0, windowFrames = 0, achievedFps10 = 0, windowMax = 0, recentMax = 0;
  void begin();
  void frame(uint32_t duration);
  void report(uint32_t now, const Display &, const StepClock &);
  void draw(Canvas &, const Display &, bool storage) const;
};
uint16_t readButtons();
void beginButtons();
} // namespace arcade
