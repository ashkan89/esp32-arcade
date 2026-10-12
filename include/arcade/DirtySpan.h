#pragma once
#include "Core.h"
#include <cstring>
namespace arcade {
struct DirtySpan {
  int left = W, right = 0;
  bool empty() const {
    return left >= right;
  }
};
// Trim unchanged columns exactly, including erasures.
template <class Pixel> DirtySpan dirtySpan(const Pixel *current, const Pixel *previous, int rows) {
  DirtySpan span;
  for (int row = 0; row < rows; ++row) {
    const Pixel *a = current + row * W, *b = previous + row * W;
    if (std::memcmp(a, b, W * sizeof(Pixel)) == 0)
      continue;
    int left = 0, right = W;
    while (left < W && a[left] == b[left])
      ++left;
    while (right > left && a[right - 1] == b[right - 1])
      --right;
    span.left = std::min(span.left, left);
    span.right = std::max(span.right, right);
  }
  return span;
}
struct DirtyPlan {
  std::array<DirtySpan, W / 16> spans{};
  unsigned count = 0;
};
template <class Pixel>
DirtyPlan dirtyPlan(const Pixel *current, const Pixel *previous, int rows, uint32_t windowUs,
                    uint32_t spiHz) {
  DirtyPlan plan;
  DirtySpan bounds = dirtySpan(current, previous, rows);
  if (bounds.empty())
    return plan;
  bool inRun = false;
  for (int x = 0; x < W; x += 16) {
    bool changed = false;
    for (int row = 0; row < rows && !changed; ++row)
      changed = std::memcmp(current + row * W + x, previous + row * W + x, 16 * sizeof(Pixel)) != 0;
    if (changed) {
      if (!inRun)
        plan.spans[plan.count++] = {std::max(x, bounds.left), std::min(x + 16, bounds.right)};
      else
        plan.spans[plan.count - 1].right = std::min(x + 16, bounds.right);
    }
    inRun = changed;
  }
  unsigned pixels = 0;
  for (unsigned i = 0; i < plan.count; ++i)
    pixels += unsigned(plan.spans[i].right - plan.spans[i].left) * rows;
  uint64_t splitCost = uint64_t(plan.count) * windowUs * spiHz + uint64_t(pixels) * 16000000;
  uint64_t bandCost =
      uint64_t(windowUs) * spiHz + uint64_t(bounds.right - bounds.left) * rows * 16000000;
  if (bandCost <= splitCost) {
    plan.count = 1;
    plan.spans[0] = bounds;
  }
  return plan;
}
} // namespace arcade
