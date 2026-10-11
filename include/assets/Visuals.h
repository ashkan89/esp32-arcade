#pragma once
#include "arcade/Canvas.h"
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
  c.rect(x, y, w, h, selected ? Panel : Ink);
  c.line(x + 1, y, x + w - 2, y, selected ? accent : Edge);
  c.line(x, y + 1, x, y + h - 2, selected ? accent : Edge);
  c.line(x + 1, y + h - 1, x + w - 2, y + h - 1, Edge);
  c.line(x + w - 1, y + 1, x + w - 1, y + h - 2, Edge);
  if (selected)
    c.rect(x + 1, y + 3, 2, h - 6, accent);
}
// Original procedural pixel assets. No heap, texture decoding, or random-state mutation.
inline void alien(Canvas &c, int x, int y, uint16_t color, unsigned frame, int scale = 1) {
  static constexpr uint16_t rows[] = {0x090, 0x060, 0x1f8, 0x3fc, 0x6f6,
                                      0x7fe, 0x5fa, 0x090, 0x108, 0x204};
  for (int j = 0; j < 10; ++j)
    for (int i = 0; i < 12; ++i) {
      unsigned bits = rows[j];
      if (j > 6 && (frame & 1))
        bits = j == 7 ? 0x108 : j == 8 ? 0x204 : 0x108;
      if (bits & (1u << i))
        c.rect(x + i * scale, y + j * scale, scale, scale, j < 4 ? color : shade(color, 65));
    }
  c.rect(x + 4 * scale, y + 4 * scale, scale, scale, White);
  c.rect(x + 7 * scale, y + 4 * scale, scale, scale, White);
}
inline void ship(Canvas &c, int x, int y, uint16_t color, unsigned frame = 0) {
  for (int j = 0; j < 12; ++j) {
    int half = j < 7 ? j / 2 : 7;
    c.rect(x + 7 - half, y + j, half * 2 + 1, 1, shade(color, 55));
    c.rect(x + 7 - half, y + j, half + 1, 1, color);
  }
  c.rect(x + 6, y + 3, 3, 5, White);
  c.rect(x + 7, y + 4, 2, 3, Cyan);
  c.rect(x + 1, y + 8, 2, 4, Muted);
  c.rect(x + 12, y + 8, 2, 4, Muted);
  c.rect(x + 4, y + 11, 2, 2 + int(frame % 3), Orange);
  c.rect(x + 9, y + 11, 2, 2 + int(frame % 3), Yellow);
}
inline void car(Canvas &c, int x, int y, uint16_t color, int h = 22) {
  c.rect(x - 1, y + 3, 16, h - 6, Ink);
  c.rect(x, y, 14, h, shade(color, 55));
  c.rect(x + 2, y, 10, h - 1, color);
  c.rect(x + 3, y + 4, 8, 5, Ink);
  c.line(x + 3, y + 4, x + 10, y + 4, Cyan);
  c.rect(x + 4, y + 10, 6, h - 15, shade(color, 80));
  c.rect(x + 3, y + h - 6, 8, 3, Ink);
  c.pixel(x + 2, y + 1, White);
  c.pixel(x + 11, y + 1, White);
  c.pixel(x + 2, y + h - 1, Red);
  c.pixel(x + 11, y + h - 1, Red);
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
  c.clear(0x0002);
  // Banded deep-space sky keeps framebuffer work bounded on the ESP32.
  c.rect(0, 28, W, 58, 0x0803);
  c.rect(0, 86, W, 74, 0x0804);
  c.circle(149, 63, 22, 0x18aa, true);
  c.circle(155, 58, 18, 0x10a7, true);
  c.line(118, 72, 175, 49, Edge);
  stars(c, motion ? frame : 0, true);
  int drift = motion ? int(frame / 4 % 12) : 0;
  alien(c, 135, 177 + drift / 3, 0x226a, motion ? frame / 18 : 0);
  alien(c, 17, 27 + drift / 4, 0x2248, motion ? frame / 18 : 0);
  if (motion && frame % 360 < 24) {
    int x = int(frame % 360) * 8;
    c.line(x - 18, 28, x, 36, Muted);
  }
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
inline void thumbnail(Canvas &c, int id, int x, int y, uint16_t color, uint32_t frame) {
  c.rect(x, y, 26, 22, 0x0002);
  if (id == 50) {
    c.rect(x, y, 26, 22, atari::Grass);
    c.rect(x + 11, y, 6, 20, atari::Concrete);
    c.rect(x + 11, y + 8, 6, 11, atari::DarkGrass);
    c.rect(x, y + 20, 26, 2, atari::Gold);
    atari::person(c, x + 6, y + 20, true, false, false, frame / 8);
    return;
  }
  if (id == 51) {
    c.rect(x, y, 26, 22, atari::RiverGrass);
    c.rect(x + 6, y, 14, 22, atari::RiverWater);
    atari::jet(c, x + 13, y + 13);
    return;
  }
  c.pixel(x + 3, y + 3, Muted);
  c.pixel(x + 22, y + 17, Edge);
  switch (motif(id)) {
  case 0:
    alien(c, x + 7, y + 5, color, frame / 20);
    break;
  case 1:
    ship(c, x + 5, y + 3, color, frame / 6);
    break;
  case 2:
    car(c, x + 6, y + 1, color, 20);
    break;
  case 3:
    for (int i = 0; i < 3; ++i) {
      c.rect(x + 2 + i * 8, y + 3, 6, 4, color);
      c.line(x + 2 + i * 8, y + 3, x + 7 + i * 8, y + 3, White);
    }
    c.circle(x + 14, y + 13, 2, White, true);
    c.rect(x + 5, y + 19, 17, 2, color);
    break;
  case 4:
    for (int i = 0; i < 4; ++i) {
      c.rect(x + 3 + i * 5, y + 5, 3, 12 - i * 2, shade(color, 60));
      c.pixel(x + 4 + i * 5, y + 19, White);
    }
    c.circle(x + 4, y + 7, 2, color, true);
    break;
  case 5:
    for (int i = 0; i < 6; ++i) {
      int xx = x + 3 + i % 3 * 7, yy = y + 4 + i / 3 * 7;
      c.rect(xx, yy, 6, 6, i == id % 6 ? White : color);
      c.line(xx + 1, yy + 1, xx + 4, yy + 1, Muted);
    }
    break;
  default:
    c.circle(x + 12, y + 5, 3, Yellow, true);
    c.rect(x + 9, y + 9, 7, 6, color);
    c.line(x + 9, y + 10, x + 5, y + 14, White);
    c.line(x + 15, y + 10, x + 20, y + 7, White);
    c.line(x + 10, y + 15, x + 7, y + 20, color);
    c.line(x + 14, y + 15, x + 18, y + 20, color);
    break;
  }
}
inline void favorite(Canvas &c, int x, int y, uint16_t color) {
  c.line(x, y - 3, x, y + 3, color);
  c.line(x - 3, y, x + 3, y, color);
  c.line(x - 2, y - 2, x + 2, y + 2, color);
  c.line(x + 2, y - 2, x - 2, y + 2, color);
}
inline void tile(Canvas &c, int x, int y, int w, int h, uint16_t color) {
  c.rect(x, y, w, h, shade(color, 45));
  c.rect(x + 1, y + 1, w - 2, h - 2, color);
  c.line(x + 1, y, x + w - 2, y, shade(color, 85));
  c.pixel(x + 1, y + 1, White);
}
} // namespace visual
} // namespace arcade
