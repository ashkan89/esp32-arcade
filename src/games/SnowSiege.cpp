#include "assets/Art.h"
#include "games/Campaign.h"
namespace arcade {
class SnowSiege final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifySnowSiege();
#endif
  struct Foe {
    bool active = false;
    float x = 0, y = 0;
    int dir = 1, snow = 0, hp = 1;
  };
  Pool<Foe, 12> foes;
  Pool<Projectile, 24> shots, balls;
  float x = 30, y = 195, vy = 0;
  int facing = 1, cool = 0;
  void setupStage() override {
    foes.clear();
    shots.clear();
    balls.clear();
    x = 30;
    y = 195;
    vy = 0;
    cool = 0;
    for (int i = 0; i < std::min(12, 4 + int(level) / 2); ++i)
      if (auto *e = foes.acquire()) {
        e->x = 45 + i % 3 * 42;
        e->y = 195 - i / 3 * 45;
        e->dir = i % 2 ? 1 : -1;
        e->hp = bossRound() && i == 0 ? 4 : 1;
      }
  }
  void play() override {
    int dx = int(input.down(Right)) - int(input.down(Left));
    if (dx)
      facing = dx;
    x = clamp(x + dx * 1.8f, 6.f, 170.f);
    if (input.hit(Special) && vy == 0)
      vy = -4.2f;
    float old = y;
    y += vy;
    vy = std::min(4.f, vy + .13f);
    if (vy >= 0)
      for (int r = 0; r < 4; ++r) {
        int yy = 205 - r * 45;
        if (old + 10 <= yy && y + 10 >= yy) {
          y = float(yy - 10);
          vy = 0;
        }
      }
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x, y, facing * 3.f, 0);
      cool = 9;
      sound(Sound::Shot);
    }
    moveProjectiles(shots);
    for (auto &p : balls.items)
      if (p.active) {
        p.oldX = p.x;
        p.oldY = p.y;
        p.x += p.vx;
        if (--p.life <= 0)
          p.active = false;
      }
    for (auto &e : foes.items)
      if (e.active) {
        if (e.snow < 3) {
          e.x += e.dir * hazardSpeed(.5f + level * .04f);
          if (e.x < 8 || e.x > 168)
            e.dir = -e.dir;
          for (auto &p : shots.items)
            if (p.active && projectileNear(p, e.x, e.y, 9)) {
              p.active = false;
              ++e.snow;
              sound(Sound::Hit);
              break;
            }
          if (stageTick % unsigned(hazardInterval(240)) == 0 && e.snow)
            e.snow--;
          if (near(x, y, e.x, e.y, 12))
            hit();
        } else if (near(x, y, e.x, e.y, 18)) {
          projectile(balls, e.x, e.y, facing * 3.3f, 0, 1, int(&e - foes.items.data()));
          e.snow = 0;
          if (--e.hp == 0)
            e.active = false;
          else {
            e.x = clamp(e.x + facing * 22, 8.f, 168.f);
            grace = 60;
          }
          add(100);
        }
        for (auto &p : balls.items)
          if (p.active && p.owner != int(&e - foes.items.data()) &&
              projectileNear(p, e.x, e.y, 12)) {
            e.snow = 3;
            p.active = false;
            burst(e.x, e.y);
            break;
          }
      }
    // Rolling snowballs bounce down to sweep the next platform.
    for (auto &p : balls.items)
      if (p.active) {
        if (p.x < 6 || p.x > 170) {
          p.vx = -p.vx;
          p.x = clamp(p.x, 6.f, 170.f);
          p.y += 45;
        }
        if (p.y > 205)
          p.active = false;
      }
    if (!foes.count())
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return bossRound() && foes.count() > 0;
  }
  int bossHealth() const override {
    int n = 0;
    for (const auto &e : foes.items)
      if (e.active)
        n = std::max(n, e.hp);
    return bossRound() ? n : 0;
  }
  size_t activeEntities() const override {
    return foes.count() + shots.count() + balls.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    art::scene(c, 36);
    for (int r = 0; r < 4; ++r) {
      art::platform(c, 0, 205 - r * 45, 176, 6, art::Sprite::SteelPlatform);
      c.rect(0, 205 - r * 45, 176, 1, White);
    }
    for (const auto &e : foes.items)
      if (e.active)
        art::object(c, e.snow == 3 ? art::Sprite::Pinball : art::Sprite::SnowMonster, int(e.x), int(e.y), 14 + e.snow, 16 + e.snow);
    if (!grace || tick % 10 < 5) {
      art::object(c, art::Sprite::SnowHero, int(x), int(y), 18, 18);
    }
    drawProjectiles(c, shots, White);
    for (const auto &p : balls.items)
      if (p.active)
        c.circle(int(p.x), int(p.y), 8, White);
    if (bossRound())
      bossBar(c, bossHealth(), 4);
    scenery(c, Cyan);
    c.centered(212, "A SNOW  B JUMP", Grey);
  }
};
GameEntry describeSnowSiege() {
  return gameEntry<SnowSiege>(
      {"SNOW SIEGE",
       "FREEZE AND ROLL THE FOES",
       {"L/R MOVES  B JUMPS", "HOLD A TO THROW SNOW", "THREE HITS FREEZE A FOE",
        "TOUCH FROZEN FOES TO KICK", "BOSS: FOUR FREEZE KICKS", "CLEAR ALL 12 SNOW FORTS"},
       Cyan,
       36,
       Left | Right | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifySnowSiege() {
  SnowSiege g;
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
  g.foes.clear();
  auto *e = g.foes.acquire();
  e->x = g.x;
  e->y = g.y;
  e->snow = 3;
  e->hp = 1;
  g.play();
  HOST_CHECK(!g.foes.count() && g.transition > 0);
  g.start({});
  g.balls.clear();
  projectile(g.balls, 169, 100, 3, 0);
  g.play();
  HOST_CHECK(g.balls.count() == 1 && g.balls.items[0].y == 145 && g.balls.items[0].vx == -3);
  g.start({});
  g.level = 4;
  g.setupStage();
  g.foes.clear();
  auto *boss = g.foes.acquire();
  boss->x = g.x;
  boss->y = g.y;
  boss->hp = 2;
  boss->snow = 3;
  g.play();
  HOST_CHECK(boss->hp == 1 && boss->snow == 0);
  return true;
}
#endif
} // namespace arcade
