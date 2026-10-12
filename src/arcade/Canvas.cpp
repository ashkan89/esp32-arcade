#include "arcade/Canvas.h"
#include <cstdio>
#include <cstring>
namespace arcade {
// Original 5x7 glyphs, rows packed in five bits. Lowercase is folded to uppercase.
static constexpr uint8_t glyphs[][7] = {
    {14, 17, 17, 31, 17, 17, 17}, {30, 17, 17, 30, 17, 17, 30}, {14, 17, 16, 16, 16, 17, 14},
    {30, 17, 17, 17, 17, 17, 30}, {31, 16, 16, 30, 16, 16, 31}, {31, 16, 16, 30, 16, 16, 16},
    {14, 17, 16, 23, 17, 17, 15}, {17, 17, 17, 31, 17, 17, 17}, {14, 4, 4, 4, 4, 4, 14},
    {7, 2, 2, 2, 18, 18, 12},     {17, 18, 20, 24, 20, 18, 17}, {16, 16, 16, 16, 16, 16, 31},
    {17, 27, 21, 21, 17, 17, 17}, {17, 25, 25, 21, 19, 19, 17}, {14, 17, 17, 17, 17, 17, 14},
    {30, 17, 17, 30, 16, 16, 16}, {14, 17, 17, 17, 21, 18, 13}, {30, 17, 17, 30, 20, 18, 17},
    {15, 16, 16, 14, 1, 1, 30},   {31, 4, 4, 4, 4, 4, 4},       {17, 17, 17, 17, 17, 17, 14},
    {17, 17, 17, 17, 17, 10, 4},  {17, 17, 17, 21, 21, 27, 17}, {17, 17, 10, 4, 10, 17, 17},
    {17, 17, 10, 4, 4, 4, 4},     {31, 1, 2, 4, 8, 16, 31},     {14, 17, 19, 21, 25, 17, 14},
    {4, 12, 4, 4, 4, 4, 14},      {14, 17, 1, 2, 4, 8, 31},     {30, 1, 1, 14, 1, 1, 30},
    {2, 6, 10, 18, 31, 2, 2},     {31, 16, 16, 30, 1, 1, 30},   {14, 16, 16, 30, 17, 17, 14},
    {31, 1, 2, 4, 8, 8, 8},       {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 1, 14},
    {0, 4, 4, 0, 4, 4, 0},        {0, 0, 0, 31, 0, 0, 0},       {0, 0, 4, 31, 4, 0, 0},
    {1, 2, 2, 4, 8, 8, 16},       {0, 0, 0, 0, 0, 6, 6},        {8, 4, 2, 1, 2, 4, 8},
    {2, 4, 8, 16, 8, 4, 2},       {14, 17, 1, 2, 4, 0, 4},      {0, 0, 0, 0, 0, 4, 8},
    {4, 4, 4, 4, 4, 0, 4},        {0, 0, 31, 0, 31, 0, 0},      {17, 2, 4, 8, 17, 0, 0},
    {0, 0, 0, 0, 0, 0, 0}};
static int glyph(char c) {
  if (c >= 'a' && c <= 'z')
    c -= 32;
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= '0' && c <= '9')
    return 26 + c - '0';
  const char *p = ":-+/.><?,!=%";
  const char *v = std::strchr(p, c);
  return v ? 36 + int(v - p) : 48;
}
static uint8_t pack(uint16_t c) {
  return uint8_t(((c >> 8) & 0xe0) | ((c >> 6) & 0x1c) | ((c >> 3) & 3));
}
static uint16_t unpack(uint8_t c) {
  static const std::array<uint16_t, 256> colors = [] {
    std::array<uint16_t, 256> result{};
    for (unsigned i = 0; i < result.size(); ++i)
      result[i] =
          uint16_t(((i >> 5) * 31 / 7) << 11 | (((i >> 2) & 7) * 63 / 7) << 5 | (i & 3) * 31 / 3);
    return result;
  }();
  return colors[c];
}
void Canvas::copyRect(int x, int y, int w, int h, uint16_t *out, bool invert) const {
  for (int row = 0; row < h; ++row) {
    size_t offset = size_t((y + row) * W + x);
    if (rgb565 && !invert)
      std::memcpy(out, static_cast<const uint16_t *>(buffer) + offset, size_t(w) * 2);
    else
      for (int col = 0; col < w; ++col) {
        uint16_t color = rgb565 ? static_cast<const uint16_t *>(buffer)[offset + col]
                                : unpack(static_cast<const uint8_t *>(buffer)[offset + col]);
        out[col] = uint16_t(color ^ (invert ? 0xffff : 0));
      }
    out += w;
  }
}
void Canvas::clear(uint16_t c) {
  if (!buffer)
    return;
  if (rgb565)
    std::fill_n(static_cast<uint16_t *>(buffer), W * H, c);
  else
    std::memset(buffer, pack(c), W * H);
}
void Canvas::pixel(int x, int y, uint16_t c) {
  if (!buffer || x < 0 || y < 0 || x >= W || y >= H)
    return;
  if (rgb565)
    static_cast<uint16_t *>(buffer)[y * W + x] = c;
  else
    static_cast<uint8_t *>(buffer)[y * W + x] = pack(c);
}
uint16_t Canvas::pixel(int x, int y) const {
  if (!buffer || x < 0 || y < 0 || x >= W || y >= H)
    return 0;
  return rgb565 ? static_cast<uint16_t *>(buffer)[y * W + x]
                : unpack(static_cast<uint8_t *>(buffer)[y * W + x]);
}
void Canvas::rect(int x, int y, int w, int h, uint16_t c, bool fill) {
  if (w <= 0 || h <= 0)
    return;
  if (!fill) {
    line(x, y, x + w - 1, y, c);
    line(x, y + h - 1, x + w - 1, y + h - 1, c);
    line(x, y, x, y + h - 1, c);
    line(x + w - 1, y, x + w - 1, y + h - 1, c);
    return;
  }
  int x0 = clamp(x, 0, W), x1 = clamp(x + w, 0, W), y0 = clamp(y, 0, H), y1 = clamp(y + h, 0, H);
  if (!buffer || x1 <= x0)
    return;
  uint8_t packed = pack(c);
  for (int j = y0; j < y1; j++) {
    if (rgb565)
      std::fill_n(static_cast<uint16_t *>(buffer) + j * W + x0, x1 - x0, c);
    else
      std::memset(static_cast<uint8_t *>(buffer) + j * W + x0, packed, size_t(x1 - x0));
  }
}
void Canvas::line(int x, int y, int x1, int y1, uint16_t c) {
  int dx = std::abs(x1 - x), sx = x < x1 ? 1 : -1, dy = -std::abs(y1 - y), sy = y < y1 ? 1 : -1,
      e = dx + dy;
  for (;;) {
    pixel(x, y, c);
    if (x == x1 && y == y1)
      break;
    int e2 = 2 * e;
    if (e2 >= dy) {
      e += dy;
      x += sx;
    }
    if (e2 <= dx) {
      e += dx;
      y += sy;
    }
  }
}
void Canvas::circle(int x, int y, int r, uint16_t c, bool fill) {
  for (int j = -r; j <= r; j++)
    for (int i = -r; i <= r; i++) {
      int d = i * i + j * j;
      if (d <= r * r && (fill || d >= (r - 1) * (r - 1)))
        pixel(x + i, y + j, c);
    }
}
void Canvas::text(int x, int y, const char *s, uint16_t c, int scale) {
  if (!s)
    return;
  for (; *s; s++, x += 6 * scale) {
    const auto &g = glyphs[glyph(*s)];
    for (int j = 0; j < 7; j++)
      for (int i = 0; i < 5; i++)
        if (g[j] & (16 >> i))
          rect(x + i * scale, y + j * scale, scale, scale, c);
  }
}
void Canvas::centered(int y, const char *s, uint16_t c, int scale) {
  text((W - int(std::strlen(s)) * 6 * scale) / 2, y, s, c, scale);
}
void Canvas::number(int x, int y, uint32_t n, uint16_t c) {
  char s[12];
  std::snprintf(s, sizeof(s), "%lu", static_cast<unsigned long>(n));
  text(x, y, s, c);
}
void Canvas::bitmap(int x, int y, const uint16_t *data, int w, int h) {
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++)
      if (data[j * w + i])
        pixel(x + i, y + j, data[j * w + i]);
}
} // namespace arcade
