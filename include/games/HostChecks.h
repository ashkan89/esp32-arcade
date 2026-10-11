#pragma once
// Test-only access is compiled out of ESP32 firmware.
#ifdef ARCADE_HOST_TESTS
#include "arcade/Canvas.h"
#include <cstdio>
namespace arcade {
extern unsigned campaignAssertions;
inline bool captureAtari(const Canvas &canvas, const char *name) {
  char path[160];
  std::snprintf(path, sizeof(path), "test-results/atari_%s.ppm", name);
  FILE *file = std::fopen(path, "wb");
  if (!file)
    return false;
  std::fprintf(file, "P6\n%d %d\n255\n", W, H);
  bool ok = true;
  for (int y = 0; y < H; ++y)
    for (int x = 0; x < W; ++x) {
      uint16_t p = canvas.pixel(x, y);
      uint8_t bytes[] = {uint8_t(((p >> 11) & 31) * 255 / 31), uint8_t(((p >> 5) & 63) * 255 / 63),
                         uint8_t((p & 31) * 255 / 31)};
      ok = std::fwrite(bytes, 1, 3, file) == 3 && ok;
    }
  return std::fclose(file) == 0 && ok;
}
} // namespace arcade
#define HOST_CHECK(expr)                                                                           \
  do {                                                                                             \
    ++campaignAssertions;                                                                          \
    if (!(expr)) {                                                                                 \
      std::fprintf(stderr, "Campaign check %s:%d: %s\n", __FILE__, __LINE__, #expr);               \
      return false;                                                                                \
    }                                                                                              \
  } while (0)
#endif
