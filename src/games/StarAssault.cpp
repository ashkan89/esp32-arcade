#include "games/StarAssault.h"
namespace arcade {
void StarAssault::updateEnemies() {
  for (auto &e : enemies.items)
    if (e.active) {
      e.age++;
      if (e.kind == 4) {
        updateBoss(e);
        continue;
      }
      if (e.kind != 3) {
        e.x = e.homeX + std::sin(tick * 0.022f) * 8;
        if (e.age > 90 && rng.range(0, 900) < 2 + std::min(8, int(level))) {
          e.kind = 3;
          e.age = 0;
          e.vx = (px - e.x) / 90;
          e.vy = 1.4f;
        }
      } else {
        e.x += e.vx + std::sin(e.age * 0.06f) * 1.4f;
        e.y += e.vy;
        if (e.age % 70 == 0)
          enemyFire(e.x + 7, e.y + 10, 0, 2.5f);
        if (e.y > H + 10) {
          e.y = e.homeY;
          e.x = e.homeX;
          e.kind = 1;
          e.age = 0;
        }
      }
    }
  formationFire();
}
} // namespace arcade

#include "games/Registration.h"
namespace arcade {
GameEntry describeStarAssault() {
  return gameEntry<StarAssault>(
      {"STAR ASSAULT",
       "CHASE THE DIVING SWARM",
       {"L/R MOVE - A SHOOTS", "B CYCLES GUNS", "CHAIN KILLS FOR COMBO", "DIVING ALIENS RETURN",
        "NO DAMAGE STAGE BONUS", "BOSS EVERY FIFTH STAGE"},
       Pink,
       2,
       Fire});
}
} // namespace arcade
