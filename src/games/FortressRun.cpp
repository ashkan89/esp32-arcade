#include "games/Campaign.h"
namespace arcade {
class FortressRun final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyFortressRun();
#endif
  struct Guard {
    bool active = false;
    float x = 0, y = 0;
    int hp = 1;
  };
  Pool<Guard, 12> guards;
  Pool<Projectile, 32> shots, hostile;
  float x = 88, y = 192;
  int dx = 0, dy = -1, cool = 0, grenades = 2, hp = 0, maximum = 0;
  bool cover(float xx, float yy) const {
    return (yy > 82 && yy < 99 && (xx < 52 || xx > 124)) ||
           (yy > 147 && yy < 159 && xx > 55 && xx < 121);
  }
  void setupStage() override {
    guards.clear();
    shots.clear();
    hostile.clear();
    x = 88;
    y = 192;
    dx = 0;
    dy = -1;
    cool = 0;
    grenades = 2;
    hp = maximum = bossRound() ? 25 + level : 0;
    for (int i = 0; i < std::min(12, 4 + int(level) / 2); ++i)
      if (auto *e = guards.acquire()) {
        e->x = 18 + i % 4 * 46;
        e->y = 45 + i / 4 * 26;
        e->hp = 1 + level / 6;
      }
  }
  void play() override {
    int mx = int(input.down(Right)) - int(input.down(Left)),
        my = int(input.down(Down)) - int(input.down(Up));
    if (mx || my) {
      dx = mx;
      dy = my;
    }
    float nx = clamp(x + mx * 1.5f, 6.f, 170.f), ny = clamp(y + my * 1.5f, 40.f, 208.f);
    if (!cover(nx, y))
      x = nx;
    if (!cover(x, ny))
      y = ny;
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x + dx * 8, y + dy * 8, dx * 3.5f, dy * 3.5f);
      cool = 13;
      sound(Sound::Shot);
    }
    if (input.hit(Special) && grenades) {
      --grenades;
      float gx = x + dx * 42, gy = y + dy * 42;
      burst(gx, gy);
      for (auto &e : guards.items)
        if (e.active && near(e.x, e.y, gx, gy, 35)) {
          e.active = false;
          add(100);
        }
      if (hp && near(88, 48, gx, gy, 40))
        hp = std::max(0, hp - 6);
      hostile.clear();
    }
    for (auto &e : guards.items)
      if (e.active) {
        float ex = e.x + clamp(x - e.x, -.25f, .25f), ey = e.y + clamp(y - e.y, -.25f, .25f);
        if (!cover(ex, e.y))
          e.x = ex;
        if (!cover(e.x, ey))
          e.y = ey;
        if (stageTick % unsigned(std::max(45, 120 - int(level) * 4)) == 0) {
          float a = std::atan2(y - e.y, x - e.x);
          projectile(hostile, e.x, e.y, std::cos(a) * 1.8f, std::sin(a) * 1.8f);
        }
        if (near(e.x, e.y, x, y, 10))
          hit();
      }
    if (hp && stageTick % 70 == 0)
      for (int i = -2; i <= 2; ++i)
        projectile(hostile, 88, 63, i * .5f, 2);
    moveProjectiles(shots);
    moveProjectiles(hostile);
    for (auto &p : shots.items)
      if (p.active) {
        if (cover(p.x, p.y)) {
          p.active = false;
          continue;
        }
        if (hp && std::abs(p.x - 88) < 25 && p.y < 62) {
          p.active = false;
          --hp;
        } else
          for (auto &e : guards.items)
            if (e.active && near(p.x, p.y, e.x, e.y, 8)) {
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
      if (p.active) {
        if (cover(p.x, p.y))
          p.active = false;
        else if (near(p.x, p.y, x, y, 8)) {
          p.active = false;
          hit();
        }
      }
    if (!guards.count() && !hp && y < 55) {
      clearStage();
    }
  }

public:
  bool bossEncounter() const override {
    return hp > 0;
  }
  int bossHealth() const override {
    return hp;
  }
  size_t activeEntities() const override {
    return guards.count() + shots.count() + hostile.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    c.rect(0, 35, 176, 175, 0x1143);
    c.rect(0, 82, 52, 17, Grey);
    c.rect(124, 82, 52, 17, Grey);
    c.rect(55, 147, 66, 12, Grey);
    c.rect(70, 35, 36, 5, guards.count() || hp ? Red : Green);
    for (const auto &e : guards.items)
      if (e.active) {
        c.circle(int(e.x), int(e.y) - 3, 4, Pink, true);
        c.rect(int(e.x) - 4, int(e.y), 8, 7, Orange);
      }
    if (!grace || tick % 10 < 5) {
      c.circle(int(x), int(y) - 4, 4, White, true);
      c.rect(int(x) - 4, int(y), 8, 7, Green);
      c.line(int(x), int(y), int(x) + dx * 12, int(y) + dy * 12, Yellow);
    }
    if (hp) {
      c.rect(64, 38, 48, 23, Red);
      c.rect(81, 48, 14, 18, Orange);
      bossBar(c, hp, maximum);
    }
    drawProjectiles(c, shots, Yellow);
    drawProjectiles(c, hostile, Pink);
    scenery(c, Green);
    c.centered(212, "CLEAR FOES THEN TOP EXIT", Grey);
  }
};
GameEntry describeFortressRun() {
  return gameEntry<FortressRun>(
      {"FORTRESS RUN",
       "BREAK THROUGH THE BUNKER",
       {"D-PAD MOVES AND AIMS", "HOLD A TO FIRE YOUR RIFLE", "B THROWS TWO GRENADES",
        "COVER STOPS BOTH SIDES", "BOSS: DESTROY THE BUNKER", "CLEAR THEN USE TOP EXIT"},
       Green,
       39,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyFortressRun() {
  FortressRun g;
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
  g.guards.clear();
  g.hp = 1;
  g.y = 45;
  g.x = 88;
  g.stageTick = 1;
  projectile(g.shots, 88, 50, 0, -3);
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
