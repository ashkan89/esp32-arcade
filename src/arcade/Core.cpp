#include "arcade/Core.h"
#include <cstring>
namespace arcade {
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
  return difficulty <= 2 && sound <= 1 && effects <= 1 && lastGame < 64 && invert <= 1 &&
         layout < 3 && attract <= 1 && (fps == 30 || fps == 60);
}
bool Settings::record(unsigned g, uint32_t s) {
  if (g >= best.size())
    return false;
  bool changed = false;
  if (s > best[g]) {
    best[g] = s;
    changed = true;
  }
  if (g == 0 && s > slyLeaders[4]) {
    for (unsigned i = 0; i < 5; i++)
      if (s > slyLeaders[i]) {
        for (unsigned j = 4; j > i; j--)
          slyLeaders[j] = slyLeaders[j - 1];
        slyLeaders[i] = s;
        changed = true;
        break;
      }
  }
  return changed;
}
uint32_t checksum(const uint8_t *p, size_t n) {
  uint32_t c = 0xffffffff;
  for (size_t i = 0; i < n; i++) {
    c ^= p[i];
    for (int b = 0; b < 8; b++)
      c = (c >> 1) ^ (0xedb88320u & uint32_t(-int32_t(c & 1)));
  }
  return ~c;
}
static void put(SaveBytes &b, size_t o, uint32_t v) {
  for (unsigned k = 0; k < 4; k++)
    b[o + k] = uint8_t(v >> (8 * k));
}
static uint32_t get(const SaveBytes &b, size_t o) {
  uint32_t v = 0;
  for (unsigned k = 0; k < 4; k++)
    v |= uint32_t(b[o + k]) << (8 * k);
  return v;
}
SaveBytes encode(const Settings &s) {
  SaveBytes b{};
  put(b, 0, 0x41435244);
  put(b, 4, 2);
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
    put(b, 272 + 4 * i, s.slyLeaders[i]);
  put(b, 292, uint32_t(s.favorites));
  put(b, 296, uint32_t(s.favorites >> 32));
  put(b, 300, checksum(b.data(), 300));
  return b;
}
bool decode(const SaveBytes &b, Settings &out) {
  if (get(b, 0) != 0x41435244 || get(b, 4) != 2 || get(b, 300) != checksum(b.data(), 300))
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
  for (unsigned i = 0; i < 64; i++)
    s.best[i] = get(b, 16 + 4 * i);
  for (unsigned i = 0; i < 5; i++)
    s.slyLeaders[i] = get(b, 272 + 4 * i);
  s.favorites = uint64_t(get(b, 292)) | (uint64_t(get(b, 296)) << 32);
  if (!s.valid())
    return false;
  for (unsigned i = 1; i < 5; i++)
    if (s.slyLeaders[i] > s.slyLeaders[i - 1])
      return false;
  out = s;
  return true;
}
bool decodeStored(const uint8_t *data, size_t length, Settings &out) {
  SaveBytes bytes{};
  if (length == SaveSize) {
    std::memcpy(bytes.data(), data, length);
    return decode(bytes, out);
  }
  if (length != 72)
    return false;
  std::memcpy(bytes.data(), data, length);
  if (get(bytes, 0) != 0x41435244 || get(bytes, 4) != 1 || get(bytes, 68) != checksum(data, 68))
    return false;
  Settings migrated;
  migrated.difficulty = bytes[8];
  migrated.sound = bytes[9];
  migrated.effects = bytes[10];
  migrated.lastGame = bytes[11];
  migrated.invert = bytes[12];
  migrated.layout = bytes[13];
  migrated.attract = bytes[14];
  migrated.fps = bytes[15];
  for (unsigned i = 0; i < 8; i++)
    migrated.best[i] = get(bytes, 16 + i * 4);
  for (unsigned i = 0; i < 5; i++)
    migrated.slyLeaders[i] = get(bytes, 48 + i * 4);
  auto encoded = encode(migrated);
  return decode(encoded, out);
}
} // namespace arcade
