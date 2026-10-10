#include "games/SkyFighters.h"
namespace arcade {
void SkyFighters::updateEnemies() {
  if (!bossStage && spawned < 24 && --spawnTimer <= 0) {
    spawnTimer = 95;
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
      if (e.age % uint32_t(std::max(55, 130 - int(level) * 4)) == 0 && e.y > Top)
        enemyFire(e.x + 7, e.y + 10, clamp((px - e.x) / 120, -0.8f, 0.8f),
                  2.0f + options.difficulty * 0.4f);
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
