#include "arcade/AudioSequencer.h"
namespace arcade {
namespace {
// Original monophonic phrases. Units are quarter-beat subdivisions; gate is percent.
constexpr Note space[] = {{330, 2, 70}, {392, 1, 65}, {494, 1, 65}, {440, 2, 75},
                          {0, 1, 0},    {392, 1, 55}, {330, 2, 75}, {294, 2, 60},
                          {262, 1, 65}, {330, 1, 65}, {392, 2, 70}, {0, 2, 0}};
constexpr Note rocks[] = {{220, 2, 65}, {0, 1, 0},    {330, 1, 60}, {294, 2, 65}, {220, 2, 55},
                          {262, 1, 60}, {294, 1, 60}, {330, 2, 70}, {196, 2, 60}, {0, 2, 0}};
constexpr Note action[] = {{294, 1, 60}, {349, 1, 60}, {440, 2, 70}, {392, 1, 60},
                           {349, 1, 60}, {294, 2, 75}, {262, 2, 70}, {330, 1, 60},
                           {392, 1, 60}, {349, 2, 70}, {0, 2, 0}};
constexpr Note road[] = {{196, 1, 55}, {294, 1, 65}, {392, 2, 70}, {330, 1, 65},
                         {294, 1, 65}, {220, 2, 60}, {196, 1, 55}, {294, 1, 65},
                         {440, 2, 70}, {392, 2, 65}, {0, 2, 0}};
constexpr Note puzzle[] = {{262, 2, 65}, {330, 2, 65}, {392, 2, 70}, {0, 2, 0}, {349, 1, 65},
                           {330, 1, 65}, {294, 2, 70}, {220, 2, 65}, {0, 2, 0}};
constexpr Note bounce[] = {{392, 1, 60}, {330, 1, 60}, {262, 2, 65}, {330, 1, 60}, {440, 1, 65},
                           {392, 2, 65}, {294, 2, 65}, {349, 2, 65}, {330, 2, 70}, {0, 2, 0}};
constexpr Note chase[] = {{220, 1, 55}, {262, 1, 55}, {330, 2, 70}, {294, 1, 55},
                          {262, 1, 55}, {247, 2, 65}, {220, 1, 55}, {330, 1, 55},
                          {392, 2, 70}, {330, 2, 65}, {0, 2, 0}};
constexpr Note flight[] = {{196, 2, 65}, {294, 1, 55}, {330, 1, 55}, {392, 2, 70}, {330, 2, 65},
                           {262, 2, 65}, {294, 1, 55}, {349, 1, 55}, {330, 2, 75}, {0, 2, 0}};
constexpr Note menu[] = {{262, 2, 70}, {330, 1, 65}, {392, 1, 65}, {523, 2, 75},
                         {0, 2, 0},    {440, 1, 65}, {392, 1, 65}, {330, 2, 70},
                         {294, 1, 65}, {330, 1, 65}, {262, 2, 75}, {0, 2, 0}};
constexpr Note mall[] = {{220, 1, 55}, {294, 1, 55}, {349, 2, 70}, {0, 1, 0},    {330, 1, 55},
                         {262, 2, 65}, {247, 1, 55}, {294, 1, 55}, {392, 2, 70}, {349, 1, 55},
                         {294, 1, 55}, {220, 2, 70}, {0, 2, 0}};
constexpr Note river[] = {{196, 1, 55}, {0, 1, 0},    {294, 2, 65}, {330, 1, 55}, {392, 1, 55},
                          {349, 2, 70}, {262, 1, 55}, {294, 1, 55}, {330, 2, 65}, {196, 2, 55},
                          {247, 1, 55}, {294, 1, 55}, {0, 2, 0}};
template <size_t N>
constexpr Track t(const Note (&notes)[N], uint8_t variant, uint16_t beat, const char *name) {
  return {notes, uint8_t(N), variant, beat, name};
}
constexpr Track tracks[] = {t(space, 0, 420, "CITY WATCH"),     t(space, 1, 400, "SKY SIGNAL"),
                            t(space, 2, 380, "SWARM DANCE"),    t(rocks, 0, 440, "ROCK PULSE"),
                            t(bounce, 0, 440, "NEON RALLY"),    t(chase, 0, 460, "COIL WALK"),
                            t(road, 0, 360, "OPEN LANE"),       t(flight, 1, 480, "WINGBEAT"),
                            t(bounce, 1, 460, "CIRCUIT SERVE"), t(puzzle, 0, 520, "FALLING GLASS"),
                            t(puzzle, 1, 560, "QUIET GRID"),    t(chase, 1, 480, "MAZE FOOTSTEPS"),
                            t(bounce, 2, 480, "RIVER STEPS"),   t(space, 3, 420, "ORBIT CLOCK"),
                            t(flight, 2, 520, "SOFT LANDING"),  t(action, 0, 440, "FUSE DANCE"),
                            t(action, 1, 440, "STEEL PATROL"),  t(bounce, 3, 460, "BUBBLE HOP"),
                            t(puzzle, 2, 540, "MERGE BLOOM"),   t(puzzle, 3, 560, "PAIR WALTZ"),
                            t(bounce, 4, 420, "ROBOT TAPS"),    t(puzzle, 4, 480, "TOWER TICKS"),
                            t(flight, 3, 440, "CLOUD CLIMB"),   t(puzzle, 5, 580, "CARGO STEPS"),
                            t(puzzle, 6, 520, "COLOR FALL"),    t(rocks, 1, 400, "VECTOR THRUM"),
                            t(space, 4, 400, "BUG MARCH"),      t(space, 5, 440, "CITY BEACON"),
                            t(road, 1, 380, "PRISM DRIVE"),     t(puzzle, 7, 500, "CLAIM LINES"),
                            t(chase, 2, 420, "DOT PATROL"),     t(chase, 3, 460, "TUNNEL SHUFFLE"),
                            t(action, 2, 460, "BARREL STEPS"),  t(flight, 4, 420, "SKY SPURS"),
                            t(chase, 4, 460, "BURGER BEAT"),    t(bounce, 5, 440, "BUBBLE TRAIL"),
                            t(action, 3, 440, "SNOW MARCH"),    t(flight, 5, 400, "CANYON WIND"),
                            t(space, 6, 380, "SOLAR SPARK"),    t(action, 4, 440, "FORTRESS PULSE"),
                            t(chase, 5, 480, "CASTLE SHADOW"),  t(action, 5, 420, "IRON CADENCE"),
                            t(action, 6, 480, "BLADE ANSWER"),  t(chase, 6, 440, "LIFT CIPHER"),
                            t(road, 2, 380, "ALPINE ARC"),      t(road, 3, 400, "RAIL RHYTHM"),
                            t(chase, 7, 440, "CRYSTAL WALK"),   t(road, 4, 360, "NIGHT STRIDE"),
                            t(bounce, 6, 420, "NOVA TABLE"),    t(flight, 6, 440, "RESCUE CALL"),
                            t(mall, 0, 400, "MALL PURSUIT"),    t(river, 0, 380, "RIVER ENGINE"),
                            t(menu, 0, 480, "ARCADE LIGHTS")};
static_assert(sizeof(tracks) / sizeof(tracks[0]) == GameCount + 1,
              "Every game needs a soundtrack descriptor");
unsigned noteDuration(const Track &tr, unsigned i) {
  unsigned units = tr.notes[i].units;
  if (tr.variant && (i + tr.variant) % 5 == 0)
    ++units;
  return units * tr.beatMs / 4;
}
} // namespace
const Track &audioTrack(unsigned id) {
  return tracks[id <= GameCount ? id : GameCount];
}
const char *audioTrackName(unsigned id) {
  return audioTrack(id).name;
}
void AudioSequencer::advance(uint32_t now) {
  if (initialized) {
    uint32_t gap = elapsed(now, previous);
    maxServiceGap = std::max(maxServiceGap, gap);
    if (!paused)
      position += gap;
  }
  initialized = true;
  previous = now;
}
void AudioSequencer::configure(bool enabled, bool musicEnabled, bool sfxEnabled) {
  master = enabled;
  music = musicEnabled;
  effects = sfxEnabled;
  if (!master || !effects) {
    playing = Sound::None;
    count = 0;
  }
  if (!master)
    output = 0;
}
void AudioSequencer::select(uint8_t id, uint32_t now) {
  advance(now);
  if (track == id)
    return;
  track = id;
  position = 0;
  playing = Sound::None;
  count = 0;
  recovering = false;
  seen.fill(false);
}
void AudioSequencer::setPaused(bool value, uint32_t now) {
  advance(now);
  if (value && !paused) {
    playing = Sound::None;
    count = 0;
    output = 0;
  }
  paused = value;
}
void AudioSequencer::stop(uint32_t now) {
  advance(now);
  playing = Sound::None;
  count = 0;
  output = 0;
  track = SilentTrack;
  position = 0;
  recovering = false;
}
void AudioSequencer::play(Sound s, uint32_t now) {
  if (!master || !effects || paused || s == Sound::None)
    return;
  unsigned id = unsigned(s);
  if (id >= last.size())
    return;
  unsigned interval = s == Sound::Shot ? 120 : s == Sound::Move || s == Sound::Footstep ? 70 : 180;
  if (seen[id] && elapsed(now, last[id]) < interval) {
    ++coalesced;
    return;
  }
  seen[id] = true;
  last[id] = now;
  if ((playing != Sound::None && soundPriority(s) > soundPriority(playing)) ||
      (playing == Sound::None && (!recovering || soundPriority(s) >= 6))) {
    playing = s;
    effectAt = now;
    return;
  }
  // A foot contact is only useful now: never queue a tap behind a hit or warning.
  if (s == Sound::Footstep) {
    ++dropped;
    return;
  }
  if (s == playing) {
    ++coalesced;
    return;
  }
  for (unsigned i = 0; i < count; ++i)
    if (queue[i].sound == s) {
      ++coalesced;
      return;
    }
  if (count == queue.size()) {
    unsigned lowest = 0;
    for (unsigned i = 1; i < count; ++i)
      if (soundPriority(queue[i].sound) < soundPriority(queue[lowest].sound))
        lowest = i;
    if (soundPriority(s) <= soundPriority(queue[lowest].sound)) {
      ++dropped;
      return;
    }
    queue[lowest] = {s, now};
    ++dropped;
  } else
    queue[count++] = {s, now};
}
uint16_t AudioSequencer::musicPitch() const {
  if (track == SilentTrack || !music)
    return 0;
  const auto &tr = audioTrack(track);
  uint32_t duration = 0;
  for (unsigned i = 0; i < tr.count; ++i)
    duration += noteDuration(tr, i);
  uint32_t at = uint32_t(position % duration);
  for (unsigned i = 0; i < tr.count; ++i) {
    auto n = tr.notes[i];
    unsigned ms = noteDuration(tr, i);
    if (at < ms) {
      if (at >= ms * n.gate / 100 || !n.hz)
        return 0;
      // Each descriptor changes both register and selected phrase accents, never gameplay RNG.
      unsigned scale = 92 + (tr.variant % 5) * 4;
      if ((i + tr.variant) % 7 == 0 && tr.variant)
        scale = scale * 3 / 2;
      return uint16_t(n.hz * scale / 100);
    }
    at -= ms;
  }
  return 0;
}
uint16_t AudioSequencer::effectPitch(uint32_t age, bool &done) const {
  done = false;
  switch (playing) {
  case Sound::Footstep:
    done = age >= 24;
    return age < 8 ? 330 : 180;
  case Sound::Move:
    done = age >= 18;
    return 880;
  case Sound::Shot:
    done = age >= 28;
    return uint16_t(1600 - std::min(28u, age) * 30);
  case Sound::Hit:
    done = age >= 60;
    return age < 30 ? 440 : 220;
  case Sound::Explosion:
    done = age >= 120;
    return uint16_t(700 - std::min(120u, age) * 4);
  case Sound::Warning:
    done = age >= 180;
    return age < 65 ? 740 : age < 110 ? 0 : 740;
  case Sound::Over:
    done = age >= 420;
    return age < 140 ? 392 : age < 280 ? 294 : 196;
  case Sound::BackCue:
    done = age >= 80;
    return age < 40 ? 440 : 330;
  case Sound::Wave:
  case Sound::Victory:
  case Sound::HighScore:
    done = age >= 360;
    return age < 90 ? 330 : age < 180 ? 392 : age < 270 ? 494 : 659;
  case Sound::Power:
  case Sound::Start:
  case Sound::Select:
    done = age >= 110;
    return age < 35 ? 330 : age < 70 ? 440 : 523;
  default:
    done = true;
    return 0;
  }
}
uint16_t AudioSequencer::update(uint32_t now) {
  advance(now);
  if (!master || paused)
    return output = 0;
  if (playing != Sound::None) {
    bool done;
    auto hz = effectPitch(elapsed(now, effectAt), done);
    if (!done)
      return output = hz;
    playing = Sound::None;
    recovering = true;
    recoveryAt = now;
  }
  if (recovering && elapsed(now, recoveryAt) >= 50)
    recovering = false;
  for (unsigned i = 0; i < count;) {
    if (elapsed(now, queue[i].at) > 500) {
      queue[i] = queue[--count];
      ++dropped;
    } else
      ++i;
  }
  if (count) {
    unsigned best = 0;
    for (unsigned i = 1; i < count; ++i)
      if (soundPriority(queue[i].sound) > soundPriority(queue[best].sound))
        best = i;
    if (!recovering || soundPriority(queue[best].sound) >= 6) {
      playing = queue[best].sound;
      effectAt = now;
      queue[best] = queue[--count];
      bool done;
      return output = effectPitch(0, done);
    }
  }
  return output = musicPitch();
}
} // namespace arcade
