#pragma once
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
  unsigned tileCutoff = 108;

public:
  Canvas canvas;
  uint32_t fullFrameUs = 0, tileUs = 0, lastTransferUs = 0;
  uint8_t maxFps = 30;
  bool begin();
  void invalidate() {
    invalid = true;
  }
  void invert(bool enabled);
  void present(bool force = false);
  void error(const char *message);
};
class Audio {
  struct Tone {
    uint16_t hz, ms;
  };
  std::array<Tone, 5> sequence{};
  uint32_t deadline = 0;
  uint8_t length = 0, index = 0, priority = 0;
  bool enabled = true, ready = false, active = false;
  bool music = false;
  uint32_t musicTick = 0;
  uint8_t musicStep = 0;
  void next(uint32_t now);

public:
  void begin();
  void update(uint32_t now);
  void play(Sound effect, uint32_t now);
  void stop();
  void setMusic(bool value) {
    music = value;
  }
  void setEnabled(bool value) {
    enabled = value;
    if (!value)
      stop();
  }
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
  uint32_t updateMax = 0, renderMax = 0, frameMax = 0, lastReport = 0, maxInputGap = 0,
           pressToUpdateMax = 0;
  std::array<uint32_t, 6> frameBuckets{};
  void begin();
  void frame(uint32_t duration);
  void report(uint32_t now, const Display &, const StepClock &);
  void draw(Canvas &, const Display &, bool storage) const;
};
uint16_t readButtons();
void beginButtons();
} // namespace arcade
