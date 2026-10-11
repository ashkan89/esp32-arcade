#pragma once
#include "Core.h"
namespace arcade {
// Simulation-tick units. Prediction is bounded and based only on sampled past movement.
inline float leadAim(float fromX, float fromY, float playerX, float playerY, float observedVx,
                     float projectileSpeed, float error, float maxLead = 24) {
  float travel = clamp((playerY - fromY) / std::max(.1f, projectileSpeed), 1.f, 120.f);
  float target = clamp(playerX + observedVx * std::min(travel, maxLead) + error, 8.f, float(W - 8));
  return clamp((target - fromX) / travel, -1.25f, 1.25f);
}
inline int variedChoice(Random &rng, unsigned count, int previous) {
  if (count < 2)
    return 0;
  unsigned choice = unsigned(rng.range(0, int(count - 1)));
  return int(choice >= unsigned(std::max(0, previous)) ? choice + 1 : choice);
}
inline float reflectedHeight(float y, float low, float high) {
  float span = high - low, v = std::fmod(y - low, 2 * span);
  if (v < 0)
    v += 2 * span;
  return low + (v <= span ? v : 2 * span - v);
}
struct FirePlan {
  int timer = 90;
  float vx = 0, vy = 0;
  bool cue() const {
    return timer > 0 && timer <= 18;
  }
  bool step(float fromX, float fromY, float targetX, float targetY, float speed, Random &rng,
            int interval) {
    if (--timer == 18) {
      float dx = targetX + float(rng.range(-10, 11)) - fromX, dy = targetY - fromY;
      float length = std::max(1.f, std::sqrt(dx * dx + dy * dy));
      vx = dx / length * speed;
      vy = dy / length * speed;
    }
    if (timer > 0)
      return false;
    timer = interval + rng.range(0, 35);
    return true;
  }
};
template <size_t N, class Blocked>
int routeStep(int from, int target, int width, Blocked blocked, bool flee = false) {
  std::array<int16_t, N> distance{}, queue{};
  distance.fill(-1);
  if (target < 0 || target >= int(N) || blocked(target))
    return from;
  unsigned head = 0, tail = 1;
  queue[0] = int16_t(target);
  distance[target] = 0;
  while (head < tail) {
    int p = queue[head++];
    for (int d : {-1, 1, -width, width}) {
      int q = p + d;
      if (q < 0 || q >= int(N) ||
          std::abs(q % width - p % width) + std::abs(q / width - p / width) != 1 || blocked(q) ||
          distance[q] >= 0)
        continue;
      distance[q] = distance[p] + 1;
      queue[tail++] = int16_t(q);
    }
  }
  int best = from;
  for (int d : {-1, 1, -width, width}) {
    int q = from + d;
    if (q < 0 || q >= int(N) ||
        std::abs(q % width - from % width) + std::abs(q / width - from / width) != 1 ||
        blocked(q) || distance[q] < 0)
      continue;
    if (distance[best] < 0 || (flee ? distance[q] > distance[best] : distance[q] < distance[best]))
      best = q;
  }
  return best;
}
} // namespace arcade
