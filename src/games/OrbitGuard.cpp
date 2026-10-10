#include "games/Registration.h"
namespace arcade {
class OrbitGuard final : public TickGame {
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
      spawn = std::max(18, 70 - int(level) * 3 - options.difficulty * 8);
      if (auto *r = rocks.acquire()) {
        r->angle = rng.range(0, 628) * 0.01f;
        r->radius = 88;
      }
    }
    for (auto &b : bolts.items)
      if (b.active) {
        b.radius += 3;
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
    if (tick % 900 == 0)
      nextStage();
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    rocks.clear();
    bolts.clear();
    angle = 0;
    spawn = cooldown = shield = 0;
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
    c.circle(88, 119, 13, Cyan, true);
    c.circle(88, 119, 24, Grey);
    int x = 88 + int(std::cos(angle) * 24), y = 119 + int(std::sin(angle) * 24);
    c.circle(x, y, 4, Yellow, true);
    for (const auto &r : rocks.items)
      if (r.active)
        c.circle(88 + int(std::cos(r.angle) * r.radius), 119 + int(std::sin(r.angle) * r.radius), 5,
                 Red, true);
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
        "B CLEARS ROCKS WITH PULSE", "PULSE RECHARGES IN 3S", "THREE CORE HITS END RUN"},
       Cyan,
       13,
       Left | Right | Fire | Special});
}
} // namespace arcade
