#include "games/SlyInvaders.h"
namespace arcade {
void SlyInvaders::updateEnemies() {
  bool step = ++moveTimer >= std::max(5, 19 - int(level));
  if (step)
    moveTimer = 0;
  if (step && !bossStage) {
    float mn = W, mx = 0;
    for (const auto &e : enemies.items)
      if (e.active && e.kind != 3) {
        mn = std::min(mn, e.x);
        mx = std::max(mx, e.x + 14);
      }
    bool edge = (dir > 0 && mx > W - 7) || (dir < 0 && mn < 7);
    if (edge)
      dir = -dir;
    for (auto &e : enemies.items)
      if (e.active && e.kind != 3) {
        if (edge)
          e.y += 7;
        else
          e.x += dir * 3;
        if (e.y + 10 >= py) {
          health = 0;
          ended = true;
        }
      }
  }
  for (auto &e : enemies.items)
    if (e.active) {
      e.age++;
      if (e.kind == 4) {
        updateBoss(e);
        continue;
      }
      if (level >= 3 && e.kind != 3 && rng.range(0, 10000) < 2) {
        e.kind = 3;
        e.vx = (px - e.x) / 85;
        e.vy = 1.7f;
      }
      if (e.kind == 3) {
        e.x += e.vx + std::sin(e.age * 0.07f) * 0.6f;
        e.y += e.vy;
        if (e.y > H)
          e.active = false;
      }
    }
  formationFire();
}
} // namespace arcade

#include "games/Registration.h"
namespace arcade {
GameEntry describeSlyInvaders() {
  return gameEntry<SlyInvaders>(
      {"SLY INVADERS",
       "DEFEND THE LAST CITY",
       {"L/R MOVE - A SHOOTS", "B CYCLES 3 GUNS", "R RAPID / L EXTRA LIFE", "S SHIELD / D DOUBLE",
        "BREAKABLE COVER BELOW", "BOSS EVERY FIFTH WAVE"},
       Green,
       0,
       Fire});
}
} // namespace arcade
