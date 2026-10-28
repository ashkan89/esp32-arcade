#pragma once
#include "arcade/Canvas.h"
#include "assets/Art.h"
namespace arcade {
namespace atari {
// Authored pixel masks; transparent bits leave the playfield untouched.
constexpr uint16_t Blue = 0x52ba, Water = 0x2255, Grass = 0x3c66, DarkGrass = 0x2364, Skin = 0xe5b3,
                   Gold = 0xbca6, Concrete = 0x8c71, Ink = 0x0000;
constexpr uint16_t RiverGrass = 0x74e8, RiverDarkGrass = 0x32e2, RiverWater = 0x3195;
inline void credit(Canvas &c, int y, uint16_t color = White) {
  static constexpr uint16_t colors[] = {Red, Orange, Gold, Grass};
  for (int i = 0; i < 4; ++i)
    c.rect(21, y + i, 32 + i, 1, colors[i]);
  c.text(58, y, "ACTIVISION", color);
}
template <size_t N>
inline void mask(Canvas &c, int x, int y, const std::array<uint8_t, N> &rows, uint16_t color,
                 bool flip = false, int stretch = 1, int ceiling = 0) {
  for (size_t row = 0; row < N; ++row)
    for (int col = 0; col < 8; ++col)
      if (y + int(row) >= ceiling && (rows[row] & (0x80 >> col)))
        c.rect(x + (flip ? 7 - col : col) * stretch, y + int(row), stretch, 1, color);
}
inline void person(Canvas &c, int x, int feet, bool cop, bool duck, bool flip, unsigned frame,
                   bool airborne = false, int ceiling = 0, bool stride = false) {
  int height = duck ? 10 : airborne ? clamp(feet - ceiling, 8, 20) : 20;
  height = std::min(height, std::max(0, feet - ceiling));
  bool secondStep = stride && frame && (frame - 1) % 8 >= 4;
  auto id = cop ? (secondStep ? art::Sprite::CopStep : art::Sprite::Cop)
                : (secondStep ? art::Sprite::ThiefStep : art::Sprite::Thief);
  if (duck) id = cop ? art::Sprite::CopDuck : art::Sprite::ThiefDuck;
  art::sprite(c, id, x - (duck ? 6 : 5), feet - height, duck ? 12 : 10, height, flip);
}
inline void jet(Canvas &c, int x, int y, int bank = 0) {
  mask(c, x - 4, y - 7,
       bank ? std::array<uint8_t, 15>{{0x18, 0x18, 0x18, 0x18, 0x38, 0x78, 0xfe, 0x7e, 0x18, 0x18,
                                       0x38, 0x3c, 0x7c, 0x64, 0x40}}
            : std::array<uint8_t, 15>{{0x18, 0x18, 0x18, 0x18, 0x3c, 0x7e, 0xff, 0xff, 0x18, 0x18,
                                       0x3c, 0x3c, 0x7e, 0x66, 0x42}},
       Yellow, bank < 0);
  if (bank)
    c.rect(x + (bank < 0 ? -4 : 2), y - 2, 2, 3, Gold);
}
// Atari-style wide seven-segment score digits rather than the cabinet font.
inline void digits(Canvas &c, int x, int y, uint32_t value, int count, uint16_t color) {
  static constexpr uint8_t segments[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66,
                                         0x6d, 0x7d, 0x07, 0x7f, 0x6f};
  uint32_t divisor = 1;
  for (int i = 1; i < count; ++i)
    divisor *= 10;
  for (int i = 0; i < count; ++i) {
    uint8_t s = segments[(value / divisor) % 10];
    int xx = x + i * 12;
    if (s & 1)
      c.rect(xx + 2, y, 6, 2, color);
    if (s & 2)
      c.rect(xx + 8, y + 2, 2, 4, color);
    if (s & 4)
      c.rect(xx + 8, y + 8, 2, 4, color);
    if (s & 8)
      c.rect(xx + 2, y + 12, 6, 2, color);
    if (s & 16)
      c.rect(xx, y + 8, 2, 4, color);
    if (s & 32)
      c.rect(xx, y + 2, 2, 4, color);
    if (s & 64)
      c.rect(xx + 2, y + 6, 6, 2, color);
    divisor /= 10;
  }
}
} // namespace atari
} // namespace arcade
