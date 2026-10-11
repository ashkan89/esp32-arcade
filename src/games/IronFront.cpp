#include "games/Campaign.h"
namespace arcade {
class IronFront final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyIronFront();
#endif
  struct Soldier {
    bool active = false;
    float x = 0, y = 0;
    int hp = 1;
  };
  Pool<Soldier, 12> foes;
  Pool<Projectile, 32> shots, hostile, grenades;
  float x = 25, y = 188, vy = 0;
  int facing = 1, cool = 0, ammo = 3, spawned = 0, hp = 0, maximum = 0;
  void setupStage() override {
    foes.clear();
    shots.clear();
    hostile.clear();
    grenades.clear();
    x = 25;
    y = 188;
    vy = 0;
    facing = 1;
    cool = spawned = 0;
    ammo = 3;
    hp = maximum = bossRound() ? 40 + level * 2 : 0;
  }
  void play() override {
    int dx = int(input.down(Right)) - int(input.down(Left));
    if (dx)
      facing = dx;
    x = clamp(x + dx * 1.7f, 6.f, hp ? 120.f : 169.f);
    if (input.hit(Up) && vy == 0)
      vy = -3.9f;
    y += vy;
    vy = std::min(4.f, vy + .14f);
    if (y >= 188) {
      y = 188;
      vy = 0;
    }
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x + facing * 8, y - 5, facing * 4.f, 0);
      cool = 9;
      sound(Sound::Shot);
    }
    if (input.hit(Special) && ammo) {
      --ammo;
      projectile(grenades, x, y - 8, facing * 1.8f, -2.7f);
      sound(Sound::Power);
    }
    if (!bossRound() && spawned < 16 && stageTick % unsigned(hazardInterval(75)) == 0)
      if (auto *e = foes.acquire()) {
        e->x = 170;
        e->y = 188;
        e->hp = 1 + level / 5;
        ++spawned;
      }
    for (auto &e : foes.items)
      if (e.active) {
        e.x -= .35f + level * .015f;
        if (stageTick % unsigned(hazardInterval(100)) == 0)
          projectile(hostile, e.x, e.y - 5, -2, 0);
        if (e.x < 2) {
          e.active = false;
          hit();
        }
        if (near(x, y, e.x, e.y, 12))
          hit();
      }
    if (hp && stageTick % unsigned(hazardInterval(hp < maximum / 2 ? 35 : 65)) == 0) {
      projectile(hostile, 140, 173, -2.3f, 0);
      if (stageTick % 130 == 0)
        projectile(hostile, 140, 173, -1.3f, -1.2f);
    }
    moveProjectiles(shots);
    moveProjectiles(hostile);
    moveProjectiles(grenades);
    for (auto &p : grenades.items)
      if (p.active) {
        p.vy += .08f;
        if (p.y >= 190) {
          p.active = false;
          burst(p.x, p.y);
          for (auto &e : foes.items)
            if (e.active && near(p.x, p.y, e.x, e.y, 32)) {
              e.active = false;
              add(150);
            }
          if (hp && p.x > 113)
            hp = std::max(0, hp - 8);
          hostile.clear();
        }
      }
    for (auto &p : shots.items)
      if (p.active) {
        if (hp && p.x > 138 && p.y >= 145) {
          p.active = false;
          --hp;
        } else
          for (auto &e : foes.items)
            if (e.active && near(p.x, p.y, e.x, e.y - 5, 10)) {
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
      if (p.active && near(p.x, p.y, x, y - 5, 9)) {
        p.active = false;
        hit();
      }
    if (bossRound() && !hp) {
      hostile.clear();
      clearStage();
    } else if (!bossRound() && spawned >= 16 && !foes.count())
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
    return foes.count() + shots.count() + hostile.count() + grenades.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    for (int i = 0; i < 6; ++i) {
      c.rect(i * 35 - int(tick / 3) % 35, 110 + i % 2 * 20, 28, 85, 0x2125);
      c.rect(i * 35 + 6 - int(tick / 3) % 35, 130, 4, 6, Orange);
    }
    c.rect(0, 198, 176, 22, Grey);
    for (const auto &e : foes.items)
      if (e.active) {
        c.circle(int(e.x), int(e.y) - 12, 4, Orange, true);
        c.rect(int(e.x) - 4, int(e.y) - 8, 8, 16, Pink);
      }
    if (!grace || tick % 10 < 5) {
      c.circle(int(x), int(y) - 12, 4, White, true);
      c.rect(int(x) - 4, int(y) - 8, 8, 15, Green);
      c.rect(int(x) + facing * 5 - 3, int(y) - 6, 9, 3, Yellow);
    }
    if (hp) {
      c.rect(140, 151, 34, 41, Red);
      c.rect(137, 190, 38, 8, Navy);
      c.rect(125, 162, 32, 6, Orange);
      bossBar(c, hp, maximum);
    }
    drawProjectiles(c, shots, Yellow);
    drawProjectiles(c, hostile, Pink);
    drawProjectiles(c, grenades, Green);
    scenery(c, Green);
    c.text(4, 211, "B GRENADES", White);
    c.number(76, 211, ammo, Yellow);
  }
};
GameEntry describeIronFront() {
  return gameEntry<IronFront>(
      {"IRON FRONT",
       "RUN GUN AND GRENADES",
       {"L/R WALK  UP JUMPS", "HOLD A FOR MACHINE GUN", "B THROWS THREE GRENADES",
        "JUMP OVER THE LOW GUNFIRE", "BOSS TANK FIRES FASTER", "CLEAR ALL 12 FRONT LINES"},
       Green,
       41,
       Left | Right | Up | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyIronFront() {
  IronFront g;
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
  projectile(g.shots, 145, 173, 4, 0);
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
