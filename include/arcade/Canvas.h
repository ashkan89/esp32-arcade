#pragma once
#include "Core.h"
namespace arcade {
class Canvas {
  void *buffer = nullptr;
  bool rgb565 = true;

public:
  void attach(void *memory, bool sixteen = true) {
    buffer = memory;
    rgb565 = sixteen;
  }
  bool ready() const {
    return buffer != nullptr;
  }
  bool sixteen() const {
    return rgb565;
  }
  void *data() {
    return buffer;
  }
  const void *data() const {
    return buffer;
  }
  void clear(uint16_t color = Black);
  // Pack an in-bounds rectangle into contiguous RGB565 transfer memory.
  void copyRect(int x, int y, int w, int h, uint16_t *out, bool invert = false) const;
  void pixel(int x, int y, uint16_t color);
  uint16_t pixel(int x, int y) const;
  void rect(int x, int y, int w, int h, uint16_t color, bool filled = true);
  void line(int x0, int y0, int x1, int y1, uint16_t color);
  void circle(int x, int y, int r, uint16_t color, bool filled = false);
  void text(int x, int y, const char *s, uint16_t color = White, int scale = 1);
  void centered(int y, const char *s, uint16_t color = White, int scale = 1);
  void number(int x, int y, uint32_t value, uint16_t color = White);
  void bitmap(int x, int y, const uint16_t *data, int w, int h);
  // Packed 4-bit flash artwork; clipped nearest-neighbor scaling, no decoding buffer.
  void indexed(int x, int y, int w, int h, const uint8_t *data,
               const uint16_t *palette, int sourceW, int sourceH,
               bool transparent = true, bool flip = false,
               int cropX = 0, int cropY = 0, int cropW = 0, int cropH = 0);
};
} // namespace arcade
