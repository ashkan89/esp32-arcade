#include "games/MeteorPatrol.h"
namespace arcade {
void MeteorPatrol::updateEnemies() {
  if (!bossStage && --spawnTimer <= 0) {
    spawnTimer = std::max(20, 95 - int(level) * 4 - options.difficulty * 10);
    spawnRock(float(rng.range(14, W - 28)), -26, 2, float(rng.range(-4, 5)) * 0.1f);
  }
  for (auto &e : enemies.items)
    if (e.active) {
      e.age++;
      if (e.kind == 4) {
        updateBoss(e);
        continue;
      }
      e.x += e.vx;
      e.y += e.vy;
      int r = 5 + e.size * 5;
      if (e.x < r || e.x > W - r) {
        e.x = clamp(e.x, float(r), float(W - r));
        e.vx = -e.vx;
      }
      if (e.y > H + r)
        e.active = false;
    }
}
} // namespace arcade

#include "games/Registration.h"
namespace arcade {
GameEntry describeMeteorPatrol() {
  return gameEntry<MeteorPatrol>(
      {"METEOR PATROL",
       "SURVIVE THE ROCK STORM",
       {"L/R MOVE - A SHOOTS", "B CYCLES GUNS", "LARGE ROCKS SPLIT", "CHAIN KILLS: X8 MAX",
        "PRESSURE RISES IN 15S", "5/10/12: STATION BOSS"},
       Orange,
       3,
       Fire});
}
} // namespace arcade
