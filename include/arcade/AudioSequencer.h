#pragma once
#include "Game.h"
namespace arcade {
struct Note {
  uint16_t hz;
  uint8_t units, gate;
};
struct Track {
  const Note *notes;
  uint8_t count, variant;
  uint16_t beatMs;
  const char *name;
};
constexpr uint8_t MenuTrack = uint8_t(GameCount), SilentTrack = 255;
const Track &audioTrack(unsigned id);
const char *audioTrackName(unsigned id);
class AudioSequencer {
  struct Event {
    Sound sound = Sound::None;
    uint32_t at = 0;
  };
  std::array<Event, 4> queue{};
  std::array<uint32_t, SoundCount> last{};
  std::array<bool, SoundCount> seen{};
  uint8_t track = SilentTrack, count = 0;
  Sound playing = Sound::None;
  uint32_t previous = 0, effectAt = 0, recoveryAt = 0;
  uint64_t position = 0;
  bool initialized = false, paused = false, master = true, music = true, effects = true,
       recovering = false;
  uint16_t output = 0;
  uint16_t musicPitch() const;
  uint16_t effectPitch(uint32_t age, bool &done) const;
  void advance(uint32_t now);

public:
  uint32_t coalesced = 0, dropped = 0, maxServiceGap = 0;
  void configure(bool enabled, bool musicEnabled, bool sfxEnabled);
  void select(uint8_t id, uint32_t now);
  void setPaused(bool value, uint32_t now);
  void play(Sound sound, uint32_t now);
  uint16_t update(uint32_t now);
  void stop(uint32_t now);
  uint64_t musicalPosition() const {
    return position;
  }
  Sound activeEffect() const {
    return playing;
  }
  unsigned queued() const {
    return count;
  }
};
} // namespace arcade
