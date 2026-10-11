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
      if (e.kind == 6) {
        if (--e.cue == 0) {
          e.kind = 3;
          e.age = 0;
        }
      } else if (e.kind == 7) {
        e.x += clamp(e.homeX - e.x, -1.2f, 1.2f);
        e.y += 1.4f;
        if (e.y >= e.homeY) {
          e.y = e.homeY;
          e.kind = 1;
          e.age = 0;
        }
      } else if (e.kind != 3) {
        e.x = e.homeX + std::sin(tick * 0.022f) * 8;
        unsigned diving = 0;
        for (auto &other : enemies.items)
          diving += other.active && (other.kind == 3 || other.kind == 6);
        if (diving < unsigned(1 + challenge() / 2) && e.age % 30 == 0 &&
            e.age > unsigned(hazardInterval(90)) &&
            rng.range(0, 900) < 2 + std::min(8, int(level)) + challenge()) {
          e.kind = 6;
          e.cue = 24;
          e.age = 0;
          e.vx = clamp((observedX - e.x) / 90, -1.1f, 1.1f);
          e.vy = hazardSpeed(1.4f);
        }
      } else {
        e.x += e.vx + std::sin(e.age * 0.06f) * 1.4f;
        e.y += e.vy;
        if (e.age % unsigned(hazardInterval(70)) == 0)
          aimedFire(e);
        if (e.y > H + 10) {
          e.y = -14;
          e.x = clamp(e.x, 4.f, float(W - 18));
          e.kind = 7;
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
