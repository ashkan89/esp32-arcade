#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace arcade {
constexpr uint8_t DifficultyCount = 4;
inline const char *difficultyName(unsigned difficulty) {
  static const char *const names[] = {"EASY", "MEDIUM", "HARD", "NIGHTMARE"};
  return names[difficulty < DifficultyCount ? difficulty : 1];
}
constexpr int W = 176, H = 220, Top = 20;
constexpr uint16_t Black = 0x0000, Navy = 0x0843, Cyan = 0x07ff, Pink = 0xf81f, Green = 0x07e0,
                   Yellow = 0xffe0, Red = 0xf800, White = 0xffff, Grey = 0x8410, Orange = 0xfd20;
inline uint32_t elapsed(uint32_t now, uint32_t then) {
  return now - then;
}
inline bool reached(uint32_t now, uint32_t deadline) {
  return int32_t(now - deadline) >= 0;
}
template <class T> T clamp(T v, T lo, T hi) {
  return std::min(hi, std::max(lo, v));
}
struct Rect {
  float x, y, w, h;
};
inline bool overlaps(Rect a, Rect b) {
  return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}
// Swept segment against an expanded box prevents fast projectiles tunneling.
bool swept(float x0, float y0, float x1, float y1, Rect target, float radius = 0);
bool segmentCircle(float x0, float y0, float x1, float y1, float cx, float cy, float radius);
bool circleRect(float cx, float cy, float radius, Rect box);
struct Random {
  uint32_t state = 1;
  uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  int range(int lo, int hi) {
    return lo + int(next() % uint32_t(hi - lo));
  }
};
inline uint32_t derivedSeed(uint32_t seed, uint32_t stream) {
  uint32_t v = seed ^ (stream * 0x9e3779b9u);
  v ^= v >> 16;
  v *= 0x7feb352du;
  v ^= v >> 15;
  v *= 0x846ca68bu;
  v ^= v >> 16;
  return v ? v : 1;
}
template <class T, size_t N> struct Pool {
  std::array<T, N> items{};
  T *acquire() {
    for (auto &v : items)
      if (!v.active) {
        v = T{};
        v.active = true;
        return &v;
      }
    return nullptr;
  }
  void clear() {
    for (auto &v : items)
      v = T{};
  }
  size_t count() const {
    size_t n = 0;
    for (auto &v : items)
      n += v.active;
    return n;
  }
};
enum Button : uint16_t {
  Left = 1,
  Right = 2,
  Fire = 4,
  Special = 8,
  Back = 16,
  Up = 32,
  Down = 64,
  Pause = 128
};
struct InputFrame {
  uint16_t held = 0, pressed = 0, released = 0, repeated = 0, longPress = 0;
  bool down(Button b) const {
    return held & b;
  }
  bool hit(Button b) const {
    return pressed & b;
  }
  bool nav(Button b) const {
    return (pressed | repeated) & b;
  }
};
class Input {
  struct Key {
    bool raw = false, stable = false, blocked = false, longSent = false;
    uint32_t changed = 0, start = 0, repeat = 0;
  };
  std::array<Key, 8> keys{};

public:
  InputFrame poll(uint16_t raw, uint32_t now);
  void suppress();
};
class StepClock {
  uint32_t previous = 0, accumulator = 0;
  bool initialized = false;

public:
  static constexpr uint32_t StepUs = 16667;
  uint32_t dropped = 0;
  void reset(uint32_t now) {
    previous = now;
    accumulator = 0;
    initialized = true;
  }
  unsigned advance(uint32_t now) {
    if (!initialized) {
      reset(now);
      return 0;
    }
    uint32_t dt = elapsed(now, previous);
    previous = now;
    if (dt > 100000) {
      dropped += dt - 100000;
      dt = 100000;
    }
    // Fractional microseconds: exactly 60 Hz, sharing phase with FramePacer.
    accumulator += dt * 60;
    unsigned n = std::min(4u, accumulator / 1000000);
    accumulator -= n * 1000000;
    if (accumulator >= 1000000) {
      dropped += (accumulator / 1000000) * 1000000 / 60;
      accumulator %= 1000000;
    }
    return n;
  }
};
class FramePacer {
  uint32_t previous = 0, phase = 0;
  uint8_t rate = 30;

public:
  uint32_t missed = 0;
  void reset(uint32_t now, uint8_t fps) {
    previous = now;
    phase = 0;
    rate = fps == 60 ? 60 : 30;
  }
  bool advance(uint32_t now) {
    uint64_t total = uint64_t(elapsed(now, previous)) * rate + phase;
    previous = now;
    uint32_t due = uint32_t(total / 1000000);
    phase = uint32_t(total % 1000000);
    if (due > 1)
      missed += due - 1;
    return due != 0;
  }
  uint32_t remaining(uint32_t now) const {
    uint64_t used = uint64_t(elapsed(now, previous)) * rate + phase;
    return used >= 1000000 ? 0 : uint32_t((1000000 - used + rate - 1) / rate);
  }
};
class FrameBudget {
  unsigned samples = 0, slow = 0;

public:
  void reset() {
    samples = slow = 0;
  }
  bool observe(uint32_t workUs) {
    // Reserve time for polling and RTOS service. Ignore isolated transition spikes.
    slow += workUs > 14667;
    if (++samples < 30)
      return false;
    bool overloaded = slow >= 8;
    reset();
    return overloaded;
  }
};
enum class Sound : uint8_t {
  None,
  Move,
  Shot,
  Hit,
  Explosion,
  Power,
  Wave,
  Over,
  Select,
  BackCue,
  Start,
  Warning,
  Victory,
  HighScore,
  Footstep
};
constexpr size_t SoundCount = size_t(Sound::Footstep) + 1;
uint8_t soundPriority(Sound sound);
struct Settings {
  uint8_t difficulty = 1, sound = 1, effects = 1, lastGame = 0, invert = 0, layout = 0, attract = 1,
          fps = 30;
  std::array<uint32_t, 64> best{}; // Stable catalog IDs; spare slots allow expansion.
  uint64_t favorites = 0;
  std::array<uint32_t, 5> skyLeaders{};
  uint8_t music = 1, sfx = 1, enhanced = 0;
  // Aggregate best/leaderboard remains historical; comparable records start fresh.
  std::array<std::array<uint32_t, 64>, 8> modeBest{};
  std::array<uint8_t, 8> recent{{255, 255, 255, 255, 255, 255, 255, 255}};
  bool played(unsigned game);
  void resetPreferences();
  void clearScores();
  bool valid() const;
  bool record(unsigned game, uint32_t score, unsigned difficulty = 1, bool enhanced = false);
  uint32_t comparableBest(unsigned game, unsigned difficulty, bool enhanced) const {
    return game < 64 && difficulty < DifficultyCount
               ? modeBest[difficulty + (enhanced ? 4 : 0)][game]
               : 0;
  }
};
constexpr size_t PreviousSaveSize = 2356;
constexpr size_t SaveSize = 2364;
using SaveBytes = std::array<uint8_t, SaveSize>;
SaveBytes encode(const Settings &s);
bool decode(const SaveBytes &bytes, Settings &out);
bool decodeStored(const uint8_t *bytes, size_t length, Settings &out);
uint32_t checksum(const uint8_t *bytes, size_t size);
} // namespace arcade
