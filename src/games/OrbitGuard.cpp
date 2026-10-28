#include "assets/Art.h"
#include "games/Registration.h"
namespace arcade {
class OrbitGuard final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyOrbitGuard();
#endif
  struct Rock {
    bool active = false;
    float angle = 0, radius = 0;
  };
  struct Bolt {
    bool active = false;
    float angle = 0, radius = 0;
  };
  Pool<Rock, 20> rocks;
  Pool<Bolt, 16> bolts;
  float angle = 0;
  int spawn = 0, cooldown = 0, shield = 0;
  int bossHP = 0, bossMaximum = 0;
  float bossAngle = 0;
  uint32_t stageStarted = 0;
  void setupStage() {
    rocks.clear();
    bolts.clear();
    bossHP = bossMaximum = level % 4 == 0 ? 16 + int(level) : 0;
    bossAngle = 0;
    stageStarted = tick;
    spawn = 30;
  }
  void simulate() override {
    angle += (int(input.down(Right)) - int(input.down(Left))) * 0.055f;
    if (input.down(Fire) && --cooldown <= 0) {
      cooldown = 12;
      if (auto *b = bolts.acquire()) {
        b->angle = angle;
        b->radius = 24;
      }
      sound(Sound::Shot);
    }
    if (shield > 0)
      shield--;
    if (input.hit(Special) && shield == 0) {
      shield = 180;
      rocks.clear();
      sound(Sound::Power);
    }
    if (--spawn <= 0) {
      spawn = std::max(18, 70 - int(level) * 3 - challenge() * 8);
      if (auto *r = rocks.acquire()) {
        r->angle = bossHP ? bossAngle : rng.range(0, 628) * 0.01f;
        r->radius = 88;
      }
    }
    for (auto &b : bolts.items)
      if (b.active) {
        b.radius += 3;
        float delta = std::atan2(std::sin(b.angle - bossAngle), std::cos(b.angle - bossAngle));
        if (bossHP && b.radius >= 78 && std::abs(delta) < .2f) {
          b.active = false;
          --bossHP;
          add(80);
          sound(Sound::Hit);
        }
        if (b.radius > 100)
          b.active = false;
      }
    for (auto &r : rocks.items)
      if (r.active) {
        r.radius -= 0.35f + std::min(0.7f, level * 0.035f);
        for (auto &b : bolts.items)
          if (b.active) {
            float delta = std::atan2(std::sin(r.angle - b.angle), std::cos(r.angle - b.angle));
            if (std::abs(delta) * r.radius < 9 && std::abs(r.radius - b.radius) < 5) {
              r.active = b.active = false;
              add(20);
              sound(Sound::Hit);
              break;
            }
          }
        if (r.active && r.radius < 14) {
          r.active = false;
          loseLife();
          if (ended)
            return;
        }
      }
    bossAngle += .007f + (bossHP < bossMaximum / 2 ? .005f : 0);
    if ((level % 4 == 0 && bossHP == 0) || (level % 4 != 0 && elapsed(tick, stageStarted) >= 900)) {
      nextStage();
      if (ended)
        return;
      if (!ended)
        setupStage();
    }
  }

public:
  bool bossEncounter() const override {
    return bossHP > 0;
  }
  int bossHealth() const override {
    return bossHP;
  }
  void start(const GameOptions &o) override {
    baseStart(o);
    rocks.clear();
    bolts.clear();
    angle = 0;
    spawn = cooldown = shield = 0;
    setupStage();
  }
  size_t activeEntities() const override {
    return rocks.count() + bolts.count() + 1;
  }
  void cleanup() override {
    Game::cleanup();
    rocks.clear();
    bolts.clear();
  }
  void render(Canvas &c) const override {
    art::scene(c, 13);
    if (bossHP) {
      int xx = 88 + int(std::cos(bossAngle) * 80), yy = 119 + int(std::sin(bossAngle) * 80);
      c.circle(xx, yy, 9, Pink, true);
      c.text(5, 24, "BOSS", Pink);
      c.rect(35, 24, bossHP * 136 / bossMaximum, 4, Red);
    }
    art::object(c, art::Sprite::Planet, 88, 119, 28, 26);
    c.circle(88, 119, 24, Grey);
    int x = 88 + int(std::cos(angle) * 24), y = 119 + int(std::sin(angle) * 24);
    art::object(c, art::Sprite::Jet, x, y, 11, 11);
    for (const auto &r : rocks.items)
      if (r.active)
        art::object(c, art::Sprite::Rock, 88 + int(std::cos(r.angle) * r.radius), 119 + int(std::sin(r.angle) * r.radius), 12, 12);
    for (const auto &b : bolts.items)
      if (b.active)
        c.circle(88 + int(std::cos(b.angle) * b.radius), 119 + int(std::sin(b.angle) * b.radius), 2,
                 White, true);
    c.centered(211, shield ? "PULSE RECHARGING" : "B: CLEAR EMERGENCY PULSE",
               shield ? Grey : Green);
  }
};
GameEntry describeOrbitGuard() {
  return gameEntry<OrbitGuard>(
      {"ORBIT GUARD",
       "DEFEND THE REACTOR",
       {"L/R ROTATES THE TURRET", "HOLD A TO FIRE OUTWARD", "ROCKS APPROACH THE CORE",
        "B CLEARS ROCKS WITH PULSE", "PULSE RECHARGES IN 3S", "4/8/12: REACTOR BOSS"},
       Cyan,
       13,
       Left | Right | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyOrbitGuard() {
  OrbitGuard g;
  g.start({});
  g.level = 12;
  g.setupStage();
  g.bossHP = 1;
  auto *b = g.bolts.acquire();
  b->angle = g.bossAngle;
  b->radius = 76;
  g.simulate();
  HOST_CHECK(g.campaignComplete() && g.gameOver() && g.stage() == 12);
  auto score = g.score();
  for (int i = 0; i < 100; ++i)
    g.update();
  HOST_CHECK(g.score() == score);
  g.start({});
  HOST_CHECK(!g.campaignComplete() && !g.gameOver() && g.stage() == 1 && g.score() == 0);
  return true;
}
#endif
} // namespace arcade
