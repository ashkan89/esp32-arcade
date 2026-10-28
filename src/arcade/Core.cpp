#include "arcade/Core.h"
#include <cstring>
namespace arcade {
uint8_t soundPriority(Sound s) {
  switch (s) {
  case Sound::None:
    return 0;
  case Sound::Footstep:
  case Sound::Move:
    return 1;
  case Sound::Shot:
    return 2;
  case Sound::Select:
  case Sound::BackCue:
    return 3;
  case Sound::Power:
  case Sound::Start:
    return 4;
  case Sound::Hit:
  case Sound::Explosion:
    return 5;
  case Sound::Warning:
    return 6;
  case Sound::Wave:
    return 7;
  case Sound::Over:
  case Sound::Victory:
  case Sound::HighScore:
    return 8;
  }
  return 0;
}
bool segmentCircle(float x0, float y0, float x1, float y1, float cx, float cy, float radius) {
  float dx = x1 - x0, dy = y1 - y0, length = dx * dx + dy * dy;
  float t = length > 0 ? clamp(((cx - x0) * dx + (cy - y0) * dy) / length, 0.0f, 1.0f) : 0;
  float x = x0 + t * dx - cx, y = y0 + t * dy - cy;
  return x * x + y * y <= radius * radius;
}
bool circleRect(float cx, float cy, float radius, Rect b) {
  float x = clamp(cx, b.x, b.x + b.w) - cx, y = clamp(cy, b.y, b.y + b.h) - cy;
  return x * x + y * y <= radius * radius;
}
bool swept(float x0, float y0, float x1, float y1, Rect b, float r) {
  b.x -= r;
  b.y -= r;
  b.w += 2 * r;
  b.h += 2 * r;
  float lo = 0, hi = 1;
  const float p[2] = {x0, y0}, d[2] = {x1 - x0, y1 - y0}, mn[2] = {b.x, b.y},
              mx[2] = {b.x + b.w, b.y + b.h};
  for (int k = 0; k < 2; k++) {
    if (std::abs(d[k]) < 0.00001f) {
      if (p[k] < mn[k] || p[k] > mx[k])
        return false;
    } else {
      float a = (mn[k] - p[k]) / d[k], z = (mx[k] - p[k]) / d[k];
      if (a > z)
        std::swap(a, z);
      lo = std::max(lo, a);
      hi = std::min(hi, z);
      if (lo > hi)
        return false;
    }
  }
  return true;
}
InputFrame Input::poll(uint16_t raw, uint32_t now) {
  InputFrame f;
  for (unsigned i = 0; i < keys.size(); i++) {
    auto &k = keys[i];
    uint16_t b = 1u << i;
    bool v = raw & b;
    if (v != k.raw) {
      k.raw = v;
      k.changed = now;
    }
    if (elapsed(now, k.changed) >= 20 && k.stable != k.raw) {
      k.stable = k.raw;
      if (k.stable) {
        k.start = k.repeat = now;
        k.longSent = false;
        if (!k.blocked)
          f.pressed |= b;
      } else {
        if (!k.blocked)
          f.released |= b;
        k.blocked = false;
      }
    }
    if (k.blocked && !k.raw && !k.stable)
      k.blocked = false;
    if (k.stable && !k.blocked) {
      f.held |= b;
      if (!k.longSent && elapsed(now, k.start) >= 700) {
        f.longPress |= b;
        k.longSent = true;
      }
      if (elapsed(now, k.start) >= 350 && elapsed(now, k.repeat) >= 120) {
        f.repeated |= b;
        k.repeat = now;
      }
    }
  }
  return f;
}
void Input::suppress() {
  for (auto &k : keys)
    k.blocked = k.stable || k.raw;
}
bool Settings::valid() const {
  bool end = false;
  uint64_t seen = 0;
  for (auto id : recent) {
    if (id == 255) {
      end = true;
      continue;
    }
    if (id >= 64 || end || (seen & (uint64_t(1) << id)))
      return false;
    seen |= uint64_t(1) << id;
  }
  return difficulty < DifficultyCount && sound <= 1 && effects <= 1 && lastGame < 64 &&
         invert <= 1 && layout < 3 && attract <= 1 && music <= 1 && sfx <= 1 && enhanced <= 1 &&
         (fps == 30 || fps == 60);
}
bool Settings::played(unsigned game) {
  if (game >= 64 || recent[0] == game)
    return false;
  unsigned at = 0;
  while (at + 1 < recent.size() && recent[at] != game)
    ++at;
  for (; at; --at)
    recent[at] = recent[at - 1];
  recent[0] = uint8_t(game);
  return true;
}
void Settings::resetPreferences() {
  difficulty = sound = effects = attract = music = sfx = 1;
  invert = layout = enhanced = 0;
  fps = 30;
}
void Settings::clearScores() {
  best.fill(0);
  skyLeaders.fill(0);
  for (auto &mode : modeBest)
    mode.fill(0);
}
bool Settings::record(unsigned g, uint32_t s, unsigned difficulty, bool enhancedMode) {
  if (g >= best.size() || difficulty >= DifficultyCount)
    return false;
  bool changed = false;
  auto &mode = modeBest[difficulty + (enhancedMode ? 4 : 0)][g];
  if (s > mode) {
    mode = s;
    changed = true;
  }
  if (s > best[g]) {
    best[g] = s;
    changed = true;
  }
  if (g == 0 && s > skyLeaders[4]) {
    for (unsigned i = 0; i < 5; i++)
      if (s > skyLeaders[i]) {
        for (unsigned j = 4; j > i; j--)
          skyLeaders[j] = skyLeaders[j - 1];
        skyLeaders[i] = s;
        changed = true;
        break;
      }
  }
  return changed;
}
uint32_t checksum(const uint8_t *p, size_t n) {
  static constexpr uint32_t table[] = {0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
                                       0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
                                       0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                                       0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
  uint32_t c = 0xffffffff;
  for (size_t i = 0; i < n; i++) {
    c ^= p[i];
    c = (c >> 4) ^ table[c & 15];
    c = (c >> 4) ^ table[c & 15];
  }
  return ~c;
}
static void put(SaveBytes &b, size_t o, uint32_t v) {
  for (unsigned k = 0; k < 4; k++)
    b[o + k] = uint8_t(v >> (8 * k));
}
static uint32_t get(const uint8_t *b, size_t o) {
  uint32_t v = 0;
  for (unsigned k = 0; k < 4; k++)
    v |= uint32_t(b[o + k]) << (8 * k);
  return v;
}
SaveBytes encode(const Settings &s) {
  SaveBytes b{};
  put(b, 0, 0x41435244);
  put(b, 4, 4);
  b[8] = s.difficulty;
  b[9] = s.sound;
  b[10] = s.effects;
  b[11] = s.lastGame;
  b[12] = s.invert;
  b[13] = s.layout;
  b[14] = s.attract;
  b[15] = s.fps;
  for (unsigned i = 0; i < 64; i++)
    put(b, 16 + 4 * i, s.best[i]);
  for (unsigned i = 0; i < 5; i++)
    put(b, 272 + 4 * i, s.skyLeaders[i]);
  put(b, 292, uint32_t(s.favorites));
  put(b, 296, uint32_t(s.favorites >> 32));
  b[300] = s.music;
  b[301] = s.sfx;
  b[302] = s.enhanced;
  for (unsigned mode = 0; mode < 8; ++mode)
    for (unsigned g = 0; g < 64; ++g)
      put(b, 304 + mode * 256 + g * 4, s.modeBest[mode][g]);
  std::copy(s.recent.begin(), s.recent.end(), b.begin() + 2352);
  put(b, SaveSize - 4, checksum(b.data(), SaveSize - 4));
  return b;
}
bool decodeStored(const uint8_t *b, size_t length, Settings &out) {
  if (!b || (length != SaveSize && length != PreviousSaveSize && length != 304 && length != 72))
    return false;
  unsigned version = length == SaveSize ? 4 : length == PreviousSaveSize ? 3 : length == 304 ? 2 : 1;
  if (get(b, 0) != 0x41435244 || get(b, 4) != version ||
      get(b, length - 4) != checksum(b, length - 4))
    return false;
  Settings s;
  s.difficulty = b[8];
  s.sound = b[9];
  s.effects = b[10];
  s.lastGame = b[11];
  s.invert = b[12];
  s.layout = b[13];
  s.attract = b[14];
  s.fps = b[15];
  for (unsigned i = 0; i < (version == 1 ? 8u : 64u); ++i)
    s.best[i] = get(b, 16 + i * 4);
  unsigned leaders = version == 1 ? 48 : 272;
  for (unsigned i = 0; i < 5; ++i)
    s.skyLeaders[i] = get(b, leaders + i * 4);
  if (version >= 2)
    s.favorites = uint64_t(get(b, 292)) | (uint64_t(get(b, 296)) << 32);
  if (version >= 3) {
    s.music = b[300];
    s.sfx = b[301];
    s.enhanced = b[302];
    for (unsigned mode = 0; mode < 8; ++mode)
      for (unsigned g = 0; g < 64; ++g)
        s.modeBest[mode][g] = get(b, 304 + mode * 256 + g * 4);
  }
  if (version >= 4)
    std::copy(b + 2352, b + 2360, s.recent.begin());
  if (!s.valid())
    return false;
  for (unsigned i = 1; i < 5; ++i)
    if (s.skyLeaders[i] > s.skyLeaders[i - 1])
      return false;
  out = s;
  return true;
}
bool decode(const SaveBytes &b, Settings &out) {
  return decodeStored(b.data(), b.size(), out);
}
} // namespace arcade
