#include "games/Campaign.h"
namespace arcade {
class RescueWing final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyRescueWing();
#endif
  struct Alien {
    bool active = false;
    float x = 0, y = 0;
    int hp = 1;
    FirePlan fire;
  };
  Pool<Alien, 12> aliens;
  Pool<Projectile, 24> shots, hostile;
  std::array<int, 5> humans{};
  float x = 44, y = 150;
  int facing = 1, cool = 0, carrying = -1, rescued = 0, hp = 0, maximum = 0;
  static float wrap(float v) {
    if (v < 0)
      v += 528;
    if (v >= 528)
      v -= 528;
    return v;
  }
  float screenX(float world) const {
    float d = world - x;
    while (d < -264)
      d += 528;
    while (d > 264)
      d -= 528;
    return d + 88;
  }
  void setupStage() override {
    aliens.clear();
    shots.clear();
    hostile.clear();
    humans = {{100, 180, 260, 340, 420}};
    x = 44;
    y = 150;
    facing = 1;
    cool = 0;
    carrying = -1;
    rescued = 0;
    hp = maximum = bossRound() ? 24 + level : 0;
    for (int i = 0; i < 6; ++i)
      if (auto *e = aliens.acquire()) {
        e->x = 80 + i * 75;
        e->y = 45;
        e->hp = 1 + level / 6;
        e->fire.timer = 70 + i * 23;
      }
  }
  void play() override {
    int dx = int(input.down(Right)) - int(input.down(Left));
    if (dx)
      facing = dx;
    x = wrap(x + dx * 2.1f);
    // Projectiles retain world motion while the camera follows the ship.
    for (auto &p : shots.items)
      if (p.active)
        p.x -= dx * 2.1f;
    for (auto &p : hostile.items)
      if (p.active)
        p.x -= dx * 2.1f;
    y = clamp(y + (int(input.down(Down)) - int(input.down(Up))) * 1.8f, 43.f, 193.f);
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, 88 + facing * 9, y, facing * 4.f, 0);
      cool = 12;
      sound(Sound::Shot);
    }
    if (input.hit(Special) && carrying < 0 && y > 178) {
      for (int i = 0; i < 5; ++i)
        if (humans[i] >= 0 && std::abs(screenX(float(humans[i])) - 88) < 14) {
          carrying = i;
          humans[i] = -1;
          sound(Sound::Power);
          break;
        }
    }
    if (carrying >= 0 && std::abs(screenX(44) - 88) < 18 && y > 172) {
      carrying = -1;
      ++rescued;
      add(500);
      sound(Sound::Wave);
    }
    for (auto &e : aliens.items)
      if (e.active) {
        float sx = screenX(e.x);
        e.y += e.y < 135 ? .15f : 0;
        e.x = wrap(e.x + clamp(88 - sx, -.45f, .45f));
        if (sx > 12 && sx < 164 && e.y < y - 25 &&
            e.fire.step(sx, e.y, 88, y, 1.6f, rng, hazardInterval(120)) && hostile.count() < 10)
          projectile(hostile, sx, e.y, e.fire.vx, e.fire.vy);
        if (near(sx, e.y, 88, y, 12))
          hit();
      }
    moveProjectiles(shots);
    moveProjectiles(hostile);
    for (auto &p : shots.items)
      if (p.active) {
        for (auto &e : aliens.items)
          if (e.active && projectileNear(p, screenX(e.x), e.y, 9)) {
            p.active = false;
            if (--e.hp == 0) {
              e.active = false;
              add(100);
              burst(p.x, p.y);
            }
            break;
          }
        if (p.active && hp && projectileNear(p, screenX(264), 90, 25)) {
          p.active = false;
          --hp;
        }
      }
    if (hp && stageTick % unsigned(hazardInterval(55)) == 0) {
      float sx = screenX(264);
      if (sx >= 0 && sx < 176)
        for (int i = -1; i <= 1; ++i)
          projectile(hostile, sx, 100, i * .6f, 2);
    }
    for (auto &p : hostile.items)
      if (p.active && projectileNear(p, 88, y, 8)) {
        p.active = false;
        hit();
      }
    if (rescued == 5 && !hp)
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
    return aliens.count() + shots.count() + hostile.count() + sparks.count() + 6;
  }
  void render(Canvas &c) const override {
    scenery(c, Cyan);
    c.line(0, 204, 175, 204, Green);
    int dock = int(screenX(44));
    c.rect(dock - 15, 194, 30, 10, Cyan);
    for (int h : humans)
      if (h >= 0) {
        int xx = int(screenX(float(h)));
        c.circle(xx, 191, 3, Yellow, true);
        c.line(xx, 194, xx, 201, Yellow);
      }
    for (const auto &e : aliens.items)
      if (e.active) {
        int xx = int(screenX(e.x));
        c.circle(xx, int(e.y), 7, Pink);
        c.rect(xx - 8, int(e.y) + 3, 16, 3, Red);
        if (e.fire.cue())
          c.line(xx, int(e.y), xx + int(e.fire.vx * 12), int(e.y + e.fire.vy * 12), Yellow);
      }
    if (!grace || tick % 10 < 5) {
      c.line(88 + facing * 9, int(y), 88 - facing * 7, int(y) - 6, Cyan);
      c.line(88 + facing * 9, int(y), 88 - facing * 7, int(y) + 6, Cyan);
      if (carrying >= 0)
        c.circle(88, int(y) + 12, 3, Yellow, true);
    }
    if (hp) {
      int xx = int(screenX(264));
      c.rect(xx - 24, 72, 48, 28, Red);
      c.circle(xx, 75, 17, Pink);
      bossBar(c, hp, maximum);
    }
    drawProjectiles(c, shots, White);
    drawProjectiles(c, hostile, Orange);
    c.text(5, 211, "RESCUED", Grey);
    c.number(53, 211, rescued, Green);
    c.text(64, 211, "/5", Grey);
  }
};
GameEntry describeRescueWing() {
  return gameEntry<RescueWing>(
      {"RESCUE WING",
       "FIVE PEOPLE ONE SAFE DOCK",
       {"D-PAD FLIES THE WRAP WORLD", "A FIRES IN FACING DIRECTION", "B PICKS UP A GOLD PERSON",
        "CARRY TO CYAN START DOCK", "BOSS: DESTROY MOTHERSHIP", "RESCUE FIVE ON 12 PLANETS"},
       Cyan,
       49,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyRescueWing() {
  RescueWing g;
  g.start({});
  // Regression checks for the gameplay audit.
  g.aliens.clear();
  g.shots.clear();
  g.hostile.clear();
  projectile(g.shots, 100, 100, 0, 0);
  g.input = {Right, Right};
  g.play();
  HOST_CHECK(std::abs(g.shots.items[0].x - 97.9f) < .001f);
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
  g.rescued = 4;
  g.carrying = 4;
  g.x = 44;
  g.y = 185;
  g.hp = 0;
  g.play();
  HOST_CHECK(g.rescued == 5 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
