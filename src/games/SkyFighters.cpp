#include "games/SkyFighters.h"
namespace arcade {
void SkyFighters::updateEnemies() {
  if (!bossStage && spawned < 24 && --spawnTimer <= 0) {
    spawnTimer = hazardInterval(95);
    spawnSky();
  }
  for (auto &e : enemies.items)
    if (e.active) {
      e.age++;
      if (e.kind == 4) {
        updateBoss(e);
        continue;
      }
      e.y += e.vy;
      if (e.kind == 1)
        e.x = e.homeX + std::sin(e.age * 0.055f) * 18;
      if (e.kind == 2) {
        e.x += e.vx;
        if (e.x < 5 || e.x > W - 19)
          e.vx = -e.vx;
      }
      if (e.cue) {
        if (--e.cue == 0)
          aimedFire(e);
      } else if (--e.actionTimer <= 0 && e.y > Top && e.y < py - 50) {
        e.cue = 18;
        e.actionTimer = hazardInterval(130) + rng.range(0, 45);
      }
      if (e.y > H)
        e.active = false;
    }
}
} // namespace arcade

#include "games/Registration.h"
namespace arcade {
GameEntry describeSkyFighters() {
  return gameEntry<SkyFighters>(
      {"SKY FIGHTERS",
       "TAKE BACK THE SKIES",
       {"L/R MOVE - A SHOOTS", "B: ONE BOMB/WAVE", "R RAPID / S SHIELD", "D SPREAD / L LIFE",
        "DODGE THREE SQUADRONS", "BOSS EVERY FOUR WAVES"},
       Cyan,
       1,
       Fire});
}
} // namespace arcade
