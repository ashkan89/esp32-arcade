#include "games/Campaign.h"
namespace arcade {
class WildRails final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyWildRails();
#endif
  struct Bandit {
    bool active = false;
    float x = 0, y = 0;
    int hp = 1;
  };
  Pool<Bandit, 16> bandits;
  Pool<Projectile, 32> shots, hostile;
  float x = 30, y = 180;
  int dx = 0, dy = -1, cool = 0, spawned = 0, hp = 0, maximum = 0;
  void setupStage() override {
    bandits.clear();
    shots.clear();
    hostile.clear();
    x = 30;
    y = 180;
    dx = 0;
    dy = -1;
    cool = spawned = 0;
    hp = maximum = bossRound() ? 30 + level : 0;
  }
  void play() override {
    int mx = int(input.down(Right)) - int(input.down(Left)),
        my = int(input.down(Down)) - int(input.down(Up));
    if (mx || my) {
      dx = mx;
      dy = my;
    }
    x = clamp(x + mx * 1.8f, 7.f, 169.f);
    y = clamp(y + my * 1.8f, 42.f, 209.f);
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x + dx * 8, y + dy * 8, dx * 3.8f, dy * 3.8f);
      cool = 12;
      sound(Sound::Shot);
    }
    if (input.hit(Special) && !grace) {
      x = x < 88 ? 140 : 36;
      grace = 60;
      sound(Sound::Power);
    }
    if (!bossRound() && spawned < 18 && stageTick % 70 == 0)
      if (auto *e = bandits.acquire()) {
        e->x = spawned % 2 ? 160 : 16;
        e->y = 40;
        e->hp = 1 + level / 5;
        ++spawned;
      }
    for (auto &e : bandits.items)
      if (e.active) {
        e.x += clamp(x - e.x, -.3f, .3f);
        e.y += .55f + level * .025f;
        if (stageTick % 90 == 0) {
          float a = std::atan2(y - e.y, x - e.x);
          projectile(hostile, e.x, e.y, std::cos(a) * 1.6f, std::sin(a) * 1.6f);
        }
        if (e.y > 215)
          e.active = false;
        if (near(x, y, e.x, e.y, 12))
          hit();
      }
    if (hp && stageTick % 50 == 0) {
      for (int i = -1; i <= 1; ++i)
        projectile(hostile, 88, 72, i * 1.1f, 2.1f);
    }
    moveProjectiles(shots);
    moveProjectiles(hostile);
    for (auto &p : shots.items)
      if (p.active) {
        if (hp && std::abs(p.x - 88) < 17 && p.y < 73) {
          p.active = false;
          --hp;
        } else
          for (auto &e : bandits.items)
            if (e.active && near(e.x, e.y, p.x, p.y, 8)) {
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
      if (p.active && near(x, y, p.x, p.y, 8)) {
        p.active = false;
        hit();
      }
    if (bossRound() && !hp) {
      hostile.clear();
      clearStage();
    } else if (!bossRound() && spawned >= 18 && !bandits.count())
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
    return bandits.count() + shots.count() + hostile.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    c.rect(0, 35, 176, 176, 0x6243);
    c.line(73, 35, 73, 211, Grey);
    c.line(103, 35, 103, 211, Grey);
    for (int yy = 35 + int(tick % 16); yy < 210; yy += 16)
      c.line(69, yy, 107, yy, Grey);
    for (int i = 0; i < 3; ++i) {
      c.rect(77, 83 + i * 38, 23, 31, Navy);
      c.rect(80, 87 + i * 38, 17, 21, Orange);
    }
    for (const auto &e : bandits.items)
      if (e.active) {
        c.rect(int(e.x) - 6, int(e.y), 12, 9, Orange);
        c.circle(int(e.x), int(e.y) - 5, 4, Pink, true);
      }
    if (!grace || tick % 10 < 5) {
      c.rect(int(x) - 7, int(y), 14, 10, Yellow);
      c.circle(int(x), int(y) - 5, 4, White, true);
      c.line(int(x), int(y) - 5, int(x) + dx * 12, int(y) - 5 + dy * 12, Cyan);
    }
    if (hp) {
      c.rect(72, 36, 32, 36, Red);
      c.circle(88, 42, 7, Grey, true);
      bossBar(c, hp, maximum);
    }
    drawProjectiles(c, shots, Yellow);
    drawProjectiles(c, hostile, Pink);
    scenery(c, Orange);
    c.centered(212, "B: DODGE ACROSS RAILS", Grey);
  }
};
GameEntry describeWildRails() {
  return gameEntry<WildRails>(
      {"WILD RAILS",
       "DEFEND THE DESERT TRAIN",
       {"D-PAD RIDES AND AIMS", "HOLD A FOR THE SIX SHOOTER", "B DODGES ACROSS THE RAILS",
        "DODGE REQUIRES NO GRACE", "BOSS: STOP THE LOCOMOTIVE", "WIN ALL 12 TRAIN BATTLES"},
       Orange,
       45,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyWildRails() {
  WildRails g;
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
  projectile(g.shots, 88, 55, 0, -3);
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
