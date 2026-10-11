#include "games/Campaign.h"
namespace arcade {
class VectorDrift final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyVectorDrift();
#endif
  struct Rock {
    bool active = false;
    float x = 0, y = 0, vx = 0, vy = 0;
    int size = 1;
  };
  Pool<Rock, 32> rocks;
  Pool<Projectile, 24> shots, hostile;
  float x = 88, y = 150, vx = 0, vy = 0, angle = -1.57f, bx = 88;
  int cool = 0, hp = 0, maximum = 0, warp = 0;
  void rock(float xx, float yy, int size) {
    if (auto *r = rocks.acquire()) {
      r->x = xx;
      r->y = yy;
      r->size = size;
      float a = rng.range(0, 628) * 0.01f;
      r->vx = std::cos(a) * hazardSpeed(0.45f + level * .04f);
      r->vy = std::sin(a) * hazardSpeed(.65f);
    }
  }
  void setupStage() override {
    rocks.clear();
    shots.clear();
    hostile.clear();
    x = 88;
    y = 160;
    vx = vy = 0;
    cool = warp = 0;
    hp = maximum = bossRound() ? 24 + level * 2 : 0;
    for (int i = 0; i < (hp ? 4 : 3 + int(level) / 2); ++i)
      rock(float(rng.range(10, 166)), float(rng.range(40, 90)), 2);
  }
  void play() override {
    angle += (int(input.down(Right)) - int(input.down(Left))) * .06f;
    if (input.down(Up)) {
      vx += std::cos(angle) * .06f;
      vy += std::sin(angle) * .06f;
    }
    vx = clamp(vx * .995f, -2.8f, 2.8f);
    vy = clamp(vy * .995f, -2.8f, 2.8f);
    x += vx;
    y += vy;
    if (x < 0)
      x += W;
    if (x >= W)
      x -= W;
    if (y < 32)
      y += 180;
    if (y > 212)
      y -= 180;
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x, y, std::cos(angle) * 4, std::sin(angle) * 4);
      cool = 12;
      sound(Sound::Shot);
    }
    if (warp)
      --warp;
    if (input.hit(Special) && !warp) {
      x = 88;
      y = 190;
      vx = vy = 0;
      grace = 90;
      warp = 300;
    }
    moveProjectiles(shots);
    moveProjectiles(hostile);
    for (auto &r : rocks.items)
      if (r.active) {
        r.x += r.vx;
        r.y += r.vy;
        if (r.x < 0)
          r.x += W;
        if (r.x > W)
          r.x -= W;
        if (r.y < 32)
          r.y += 180;
        if (r.y > 212)
          r.y -= 180;
        if (near(x, y, r.x, r.y, 6 + r.size * 4))
          hit();
        for (auto &p : shots.items)
          if (p.active && near(p.x, p.y, r.x, r.y, 5 + r.size * 4)) {
            float xx = r.x, yy = r.y;
            int size = r.size;
            r.active = p.active = false;
            add(30 * (3 - size));
            burst(xx, yy);
            if (size) {
              rock(xx - 3, yy, size - 1);
              rock(xx + 3, yy, size - 1);
            }
            break;
          }
      }
    if (hp) {
      bx = 88 + std::sin(stageTick * .015f) * 55;
      if (stageTick % unsigned(hazardInterval(60)) == 0)
        for (int i = -1; i <= 1; ++i)
          projectile(hostile, bx, 65, clamp((x - bx) / 65, -1.4f, 1.4f) + i * .5f, 1.8f);
      for (auto &p : shots.items)
        if (p.active && near(p.x, p.y, bx, 52, 19)) {
          p.active = false;
          if (--hp == 0) {
            burst(bx, 52);
            rocks.clear();
            clearStage();
            break;
          }
        }
      if (near(x, y, bx, 52, 24))
        hit();
    }
    for (auto &p : hostile.items)
      if (p.active && near(x, y, p.x, p.y, 8)) {
        p.active = false;
        hit();
      }
    if (!hp && !rocks.count())
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
    return rocks.count() + shots.count() + hostile.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    scenery(c, Cyan);
    for (const auto &r : rocks.items)
      if (r.active) {
        int radius = 5 + r.size * 4;
        c.circle(int(r.x), int(r.y), radius, Grey);
        c.line(int(r.x) - radius, int(r.y), int(r.x), int(r.y) - radius, White);
      }
    if (!grace || tick % 10 < 5) {
      int xx = int(x), yy = int(y);
      c.line(xx + int(std::cos(angle) * 9), yy + int(std::sin(angle) * 9),
             xx + int(std::cos(angle + 2.5f) * 7), yy + int(std::sin(angle + 2.5f) * 7), Cyan);
      c.line(xx + int(std::cos(angle) * 9), yy + int(std::sin(angle) * 9),
             xx + int(std::cos(angle - 2.5f) * 7), yy + int(std::sin(angle - 2.5f) * 7), Cyan);
    }
    if (hp) {
      c.circle(int(bx), 52, 19, Pink);
      c.circle(int(bx), 52, 9, Red, true);
      bossBar(c, hp, maximum);
    }
    drawProjectiles(c, shots, White);
    drawProjectiles(c, hostile, Red);
    c.centered(211, warp ? "WARP RECHARGING" : "UP THRUST  B WARP", Grey);
  }
};
GameEntry describeVectorDrift() {
  return gameEntry<VectorDrift>(
      {"VECTOR DRIFT",
       "THRUST THROUGH THE BELT",
       {"L/R ROTATE  UP THRUST", "A FIRES IN YOUR DIRECTION", "B WARPS - 5S RECHARGE",
        "ROCKS SPLIT INTO FRAGMENTS", "STAGES 4/8/12: STAR FORT", "CLEAR 12 SECTORS TO WIN"},
       Cyan,
       25,
       Left | Right | Up | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyVectorDrift() {
  VectorDrift g;
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
  g.rocks.clear();
  g.hp = 1;
  g.bx = 88;
  g.stageTick = 1;
  projectile(g.shots, 88, 52, 0, 0);
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
