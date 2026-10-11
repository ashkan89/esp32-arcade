#include "games/Campaign.h"
namespace arcade {
class SolarLance final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifySolarLance();
#endif
  struct Drone {
    bool active = false;
    float x = 0, y = 0;
    int hp = 1;
  };
  Pool<Drone, 16> drones;
  Pool<Projectile, 24> shots, hostile;
  float x = 30, y = 120;
  int cool = 0, charge = 0, beam = 0, spawned = 0, hp = 0, maximum = 0;
  void setupStage() override {
    drones.clear();
    shots.clear();
    hostile.clear();
    x = 30;
    y = 120;
    cool = charge = beam = spawned = 0;
    hp = maximum = bossRound() ? 40 + level * 2 : 0;
  }
  void play() override {
    x = clamp(x + (int(input.down(Right)) - int(input.down(Left))) * 1.8f, 9.f, 130.f);
    y = clamp(y + (int(input.down(Down)) - int(input.down(Up))) * 1.8f, 48.f, 192.f);
    if (input.down(Special))
      charge = std::min(90, charge + 1);
    else if (charge) {
      if (charge >= 30) {
        beam = 12;
        sound(Sound::Power);
      }
      charge = 0;
    }
    if (beam)
      --beam;
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x + 8, y, 4, 0);
      cool = 12;
      sound(Sound::Shot);
    }
    if (!bossRound() && spawned < 22 && stageTick % unsigned(hazardInterval(50)) == 0)
      if (auto *e = drones.acquire()) {
        e->x = 170;
        e->y = 60 + spawned % 4 * 36;
        e->hp = 1 + level / 5;
        ++spawned;
      }
    for (auto &e : drones.items)
      if (e.active) {
        e.x -= .8f + level * .025f;
        e.y += std::sin((stageTick + int(e.x) * 3) * .04f) * .5f;
        if (stageTick % unsigned(hazardInterval(100)) == 0)
          projectile(hostile, e.x, e.y, -2, clamp((y - e.y) / 100, -1.f, 1.f));
        if (e.x < 3)
          e.active = false;
        if (near(x, y, e.x, e.y, 12))
          hit();
        if (beam && e.x > x && std::abs(e.y - y) < 10) {
          e.active = false;
          add(100);
          burst(e.x, e.y);
        }
      }
    float bossY = 120 + std::sin(stageTick * .018f) * 45;
    bool open = stageTick % 180 >= 75;
    if (hp && stageTick % unsigned(hazardInterval(60)) == 0)
      for (int i = -1; i <= 1; ++i)
        projectile(hostile, 143, bossY, -2, i * .6f);
    moveProjectiles(shots);
    moveProjectiles(hostile);
    for (auto &p : shots.items)
      if (p.active) {
        if (hp && p.x > 140 && std::abs(p.y - bossY) < 28) {
          p.active = false;
          if (open)
            --hp;
        } else
          for (auto &e : drones.items)
            if (e.active && near(p.x, p.y, e.x, e.y, 8)) {
              p.active = false;
              if (--e.hp == 0) {
                e.active = false;
                add(75);
                burst(e.x, e.y);
              }
              break;
            }
      }
    if (hp && open && beam && stageTick % 3 == 0 && std::abs(y - bossY) < 30)
      hp = std::max(0, hp - 2);
    for (auto &p : hostile.items)
      if (p.active && near(x, y, p.x, p.y, 8)) {
        p.active = false;
        hit();
      }
    if (bossRound() && hp == 0) {
      hostile.clear();
      clearStage();
    } else if (!bossRound() && spawned >= 22 && !drones.count())
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
    return drones.count() + shots.count() + hostile.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    scenery(c, Cyan);
    for (int i = 0; i < 12; ++i) {
      int xx = (i * 22 - int(tick) % 22);
      c.line(xx, 37, xx + 12, 37, Navy);
      c.line(xx, 202, xx + 12, 202, Navy);
    }
    for (const auto &e : drones.items)
      if (e.active) {
        c.circle(int(e.x), int(e.y), 7, Pink);
        c.rect(int(e.x) - 3, int(e.y) - 3, 6, 6, Red);
      }
    if (!grace || tick % 10 < 5) {
      c.line(int(x) - 8, int(y) - 6, int(x) + 10, int(y), Cyan);
      c.line(int(x) - 8, int(y) + 6, int(x) + 10, int(y), Cyan);
    }
    if (beam)
      c.rect(int(x) + 10, int(y) - 6, 176 - int(x) - 10, 12, Yellow);
    if (charge)
      c.circle(int(x) + 12, int(y), 3 + charge / 20, Yellow);
    if (hp) {
      int yy = 120 + int(std::sin(stageTick * .018f) * 45);
      c.rect(143, yy - 25, 27, 50, stageTick % 180 >= 75 ? Red : Grey);
      c.circle(147, yy, 8, Pink, true);
      bossBar(c, hp, maximum);
    }
    drawProjectiles(c, shots, White);
    drawProjectiles(c, hostile, Orange);
    c.centered(212, "HOLD / RELEASE B: BEAM", Grey);
  }
};
GameEntry describeSolarLance() {
  return gameEntry<SolarLance>(
      {"SOLAR LANCE",
       "CHARGE THROUGH THE FLEET",
       {"D-PAD MOVES  A SHOOTS", "HOLD B AT LEAST HALF A SEC", "RELEASE B FOR A WIDE BEAM",
        "BEAM CUTS THROUGH DRONES", "BOSS CORE OPENS RED", "CLEAR 12 SOLAR SECTORS"},
       Cyan,
       38,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifySolarLance() {
  SolarLance g;
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
  g.stageTick = 90;
  projectile(g.shots, 145, 165, 4, 0);
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  g.start({});
  g.level = 4;
  g.setupStage();
  int before = g.hp;
  g.stageTick = 1;
  projectile(g.shots, 145, 120, 4, 0);
  g.play();
  HOST_CHECK(g.hp == before);
  return true;
}
#endif
} // namespace arcade
