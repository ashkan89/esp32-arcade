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
  void pixel(int x, int y, uint16_t color);
  uint16_t pixel(int x, int y) const;
  void rect(int x, int y, int w, int h, uint16_t color, bool filled = true);
  void line(int x0, int y0, int x1, int y1, uint16_t color);
  void circle(int x, int y, int r, uint16_t color, bool filled = false);
  void text(int x, int y, const char *s, uint16_t color = White, int scale = 1);
  void centered(int y, const char *s, uint16_t color = White, int scale = 1);
  void number(int x, int y, uint32_t value, uint16_t color = White);
  void bitmap(int x, int y, const uint16_t *data, int w, int h);
};
} // namespace arcade
