#include "games/Campaign.h"
namespace arcade {
class CanyonStrike final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyCanyonStrike();
#endif
  struct Plane {
    bool active = false;
    float x = 0, y = 0, vx = 0;
    int hp = 1;
    FirePlan fire;
  };
  Pool<Plane, 16> planes;
  Pool<Projectile, 32> shots, hostile;
  float x = 88, y = 185, bx = 88;
  int cool = 0, spawned = 0, bombs = 1, hp = 0, maximum = 0;
  void setupStage() override {
    planes.clear();
    shots.clear();
    hostile.clear();
    x = 88;
    y = 185;
    cool = spawned = 0;
    bombs = 1;
    hp = maximum = bossRound() ? 40 + level * 2 : 0;
  }
  void play() override {
    x = clamp(x + (int(input.down(Right)) - int(input.down(Left))) * 2.2f, 8.f, 168.f);
    y = clamp(y + (int(input.down(Down)) - int(input.down(Up))) * 1.8f, 80.f, 208.f);
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x - 4, y, 0, -5);
      projectile(shots, x + 4, y, 0, -5);
      cool = 12;
      sound(Sound::Shot);
    }
    if (input.hit(Special) && bombs) {
      --bombs;
      hostile.clear();
      grace = 150;
      for (auto &e : planes.items)
        if (e.active) {
          e.active = false;
          add(70);
        }
      hp = std::max(0, hp - 10);
      if (bossRound() && !hp)
        clearStage();
      sound(Sound::Power);
    }
    if (!bossRound() && spawned < 24 && stageTick % unsigned(hazardInterval(45)) == 0)
      if (auto *e = planes.acquire()) {
        e->x = 20 + spawned % 4 * 43;
        e->y = 35;
        e->vx = spawned % 2 ? .4f : -.4f;
        e->hp = 1 + level / 6;
        e->fire.timer = 50 + (spawned % 4) * 17;
        ++spawned;
      }
    for (auto &e : planes.items)
      if (e.active) {
        e.x += e.vx;
        e.y += .75f + level * .06f;
        if (e.x < 8 || e.x > 168)
          e.vx = -e.vx;
        if (e.y < y - 35 && e.fire.step(e.x, e.y, x, y, 1.9f, rng, hazardInterval(110)) &&
            hostile.count() < 12)
          projectile(hostile, e.x, e.y, e.fire.vx, e.fire.vy);
        if (near(x, y, e.x, e.y, 12))
          hit();
        if (e.y > 215)
          e.active = false;
      }
    if (hp) {
      bx = 88 + std::sin(stageTick * .02f) * 50;
      if (stageTick % unsigned(hazardInterval(hp < maximum / 2 ? 35 : 60)) == 0)
        for (int i = -2; i <= 2; ++i)
          projectile(hostile, bx + i * 13, 65, i * .35f, 2 + challenge() * .2f);
    }
    moveProjectiles(shots);
    moveProjectiles(hostile);
    for (auto &p : shots.items)
      if (p.active) {
        if (hp && std::abs(p.x - bx) < 32 && p.y < 66 && p.y > 38) {
          p.active = false;
          if (--hp == 0) {
            burst(bx, 52);
            hostile.clear();
            clearStage();
          }
          continue;
        }
        for (auto &e : planes.items)
          if (e.active && projectileNear(p, e.x, e.y, 9)) {
            p.active = false;
            if (--e.hp == 0) {
              e.active = false;
              add(100);
              burst(e.x, e.y);
            }
            break;
          }
      }
    for (auto &p : hostile.items)
      if (p.active && near(x, y, p.x, p.y, 7)) {
        p.active = false;
        hit();
      }
    if (!bossRound() && spawned >= 24 && !planes.count())
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return hp > 0;
  }
  int bossHealth() const override {
    return hp;
  }
  size_t activeEntities() const override {
    return planes.count() + shots.count() + hostile.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    for (int yy = 32; yy < 210; yy += 3) {
      int left = 18 + int(std::sin((yy + tick) * .024f) * 12);
      c.rect(left, yy, 140 - left, 3, 0x0945);
      c.rect(left - 8, yy, 8, 3, 0x6284);
      c.rect(140, yy, 12, 3, 0x6284);
    }
    for (const auto &e : planes.items)
      if (e.active) {
        c.rect(int(e.x) - 3, int(e.y) - 8, 6, 16, Pink);
        c.rect(int(e.x) - 10, int(e.y) - 2, 20, 4, Orange);
        if (e.fire.cue())
          c.line(int(e.x), int(e.y), int(e.x + e.fire.vx * 12), int(e.y + e.fire.vy * 12), Yellow);
      }
    if (!grace || tick % 10 < 5)
      ship(c, x, y, Green);
    if (hp) {
      c.rect(int(bx) - 30, 40, 60, 17, Red);
      c.rect(int(bx) - 7, 34, 14, 31, Orange);
      bossBar(c, hp, maximum);
    }
    drawProjectiles(c, shots, Yellow);
    drawProjectiles(c, hostile, Pink);
    scenery(c, Green);
    c.centered(212, bombs ? "B: ONE SMART BOMB" : "BOMB USED THIS STAGE", Grey);
  }
};
GameEntry describeCanyonStrike() {
  return gameEntry<CanyonStrike>(
      {"CANYON STRIKE",
       "WING THROUGH THE CANYON",
       {"D-PAD MOVES YOUR FIGHTER", "HOLD A FOR TWIN CANNONS", "B SMART BOMB ONCE A STAGE",
        "DODGE AIMED ENEMY FIRE", "BOSS BOMBER ENRAGES AT 50%", "CLEAR 12 CANYON SORTIES"},
       Green,
       37,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyCanyonStrike() {
  CanyonStrike g;
  g.start({});
  for (int stage = 1; stage <= 12; ++stage) {
    HOST_CHECK(g.stage() == stage && !g.gameOver());
    HOST_CHECK(g.activeEntities() < 500);
    g.clearStage();
    g.pause();
    for (int i = 0; i < 100; ++i)
      g.update();
    HOST_CHECK(g.transition == 90);
    g.resume();
    for (int i = 0; i < 90; ++i)
      g.update();
  }
  HOST_CHECK(g.campaignComplete() && g.stage() == 12);
  auto finalScore = g.score();
  for (int i = 0; i < 100; ++i)
    g.update();
  HOST_CHECK(g.score() == finalScore);
  g.start({});
  HOST_CHECK(!g.gameOver() && !g.campaignComplete() && g.score() == 0);
  g.level = 4;
  g.setupStage();
  g.stageTick = 1;
  HOST_CHECK(g.bossHealth() > 0);
  g.hp = 1;
  g.stageTick = 1;
  projectile(g.shots, 88, 60, 0, -5);
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
