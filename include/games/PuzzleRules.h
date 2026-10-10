#pragma once
#include "arcade/Core.h"
namespace arcade {
// Compress toward index 0; each original tile can merge at most once per move.
inline uint32_t mergeLine(std::array<uint8_t, 4> &line) {
  std::array<uint8_t, 4> packed{}, result{};
  unsigned n = 0, out = 0;
  uint32_t score = 0;
  for (auto v : line)
    if (v)
      packed[n++] = v;
  for (unsigned i = 0; i < n; i++) {
    uint8_t value = packed[i];
    if (i + 1 < n && packed[i + 1] == value && value < 15) {
      value++;
      i++;
      score += uint32_t(1) << value;
    }
    result[out++] = value;
  }
  line = result;
  return score;
}
inline int adjacentMines(const std::array<uint8_t, 81> &mines, int p) {
  int count = 0;
  for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++) {
      int x = p % 9 + dx, y = p / 9 + dy;
      if ((dx || dy) && x >= 0 && x < 9 && y >= 0 && y < 9)
        count += bool(mines[y * 9 + x]);
    }
  return count;
}
inline uint64_t matchRuns(const std::array<uint8_t, 64> &cells) {
  uint64_t mask = 0;
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 8; x++) {
      int p = y * 8 + x;
      if (!cells[p])
        continue;
      if (x < 6 && cells[p] == cells[p + 1] && cells[p] == cells[p + 2]) {
        int end = x + 3;
        while (end < 8 && cells[y * 8 + end] == cells[p])
          end++;
        for (int k = x; k < end; k++)
          mask |= uint64_t(1) << (y * 8 + k);
      }
      if (y < 6 && cells[p] == cells[p + 8] && cells[p] == cells[p + 16]) {
        int end = y + 3;
        while (end < 8 && cells[end * 8 + x] == cells[p])
          end++;
        for (int k = y; k < end; k++)
          mask |= uint64_t(1) << (k * 8 + x);
      }
    }
  return mask;
}
} // namespace arcade
