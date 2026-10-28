#pragma once
#include "arcade/Canvas.h"
#include "assets/Art.h"
#include "assets/AtariPixels.h"
namespace arcade {
namespace visual {
constexpr uint16_t Ink = 0x0844, Panel = 0x10a7, Edge = 0x21cc, Muted = 0x9d36;
inline uint16_t shade(uint16_t c, int strength) {
  return uint16_t((((c >> 11) & 31) * strength / 100) << 11 |
                  (((c >> 5) & 63) * strength / 100) << 5 | (c & 31) * strength / 100);
}
inline void card(Canvas &c, int x, int y, int w, int h, uint16_t accent, bool selected) {
  c.rect(x + 2, y + 2, w, h, Ink);
  art::scene(c, selected ? 54 : 53, x, y, w, h, selected ? 42 : 28);
  c.line(x + 1, y, x + w - 2, y, selected ? accent : Edge);
  c.line(x, y + 1, x, y + h - 2, selected ? accent : Edge);
  c.line(x + 1, y + h - 1, x + w - 2, y + h - 1, Edge);
  c.line(x + w - 1, y + 1, x + w - 1, y + h - 2, Edge);
  if (selected)
    c.rect(x + 1, y + 3, 2, h - 6, accent);
}
// Original procedural pixel assets. No heap, texture decoding, or random-state mutation.
inline void alien(Canvas &c, int x, int y, uint16_t color, unsigned frame, int scale = 1) {
  art::sprite(c, color == Pink ? art::Sprite::AlienPink : art::Sprite::Alien,
              x, y, 12 * scale, 10 * scale, bool(frame & 1));
}
inline void ship(Canvas &c, int x, int y, uint16_t, unsigned = 0) {
  art::sprite(c, art::Sprite::Jet, x, y, 16, 14);
}
inline void car(Canvas &c, int x, int y, uint16_t color, int h = 22) {
  art::sprite(c, color == Red ? art::Sprite::EnemyCar : art::Sprite::Car, x, y, 14, h);
}
inline void stars(Canvas &c, uint32_t frame, bool menu = false) {
  for (int i = 0; i < (menu ? 44 : 28); ++i) {
    int x = (i * 47 + 13) % W;
    int y = Top + int((i * 31 + frame / (1 + i % 3)) % (H - Top));
    uint16_t color = i % 4 == 0 ? Muted : Edge;
    c.pixel(x, y, color);
    if (i % 11 == 0) {
      c.pixel(x - 1, y, Edge);
      c.pixel(x + 1, y, Edge);
      c.pixel(x, y - 1, Edge);
      c.pixel(x, y + 1, Edge);
    }
  }
}
inline void backdrop(Canvas &c, uint32_t frame, bool motion) {
  art::scene(c, 52, 0, 0, W, H, 75);
  if (motion) stars(c, frame, true);
}
// Categories correspond to stable catalog IDs and produce recognizable thumbnail art.
inline int motif(int id) {
  switch (id) {
  case 0:
  case 2:
  case 26:
    return 0;
  case 1:
  case 3:
  case 13:
  case 14:
  case 25:
  case 27:
  case 37:
  case 38:
  case 49:
    return 1;
  case 6:
  case 28:
  case 44:
  case 45:
  case 47:
    return 2;
  case 4:
  case 8:
  case 17:
  case 35:
  case 48:
    return 3;
  case 5:
  case 11:
  case 12:
  case 15:
  case 29:
  case 30:
  case 31:
    return 4;
  case 9:
  case 10:
  case 18:
  case 19:
  case 21:
  case 23:
  case 24:
    return 5;
  default:
    return 6;
  }
}
inline void thumbnail(Canvas &c, int id, int x, int y, uint16_t, uint32_t) {
  art::cover(c, unsigned(id), x, y, 26, 22);
}
inline void favorite(Canvas &c, int x, int y, uint16_t color) {
  c.line(x, y - 3, x, y + 3, color);
  c.line(x - 3, y, x + 3, y, color);
  c.line(x - 2, y - 2, x + 2, y + 2, color);
  c.line(x + 2, y - 2, x - 2, y + 2, color);
}
inline void tile(Canvas &c, int x, int y, int w, int h, uint16_t color) {
  art::tile(c, x, y, w, h, color);
}
} // namespace visual
} // namespace arcade
