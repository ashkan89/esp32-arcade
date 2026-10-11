#include "games/Campaign.h"
namespace arcade {
class AlpineRush final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyAlpineRush();
#endif
  struct Obstacle {
    bool active = false;
    float x = 0, y = 0;
    int kind = 0;
  };
  Pool<Obstacle, 24> obstacles;
  float x = 88, speed = 1.4f;
  int jump = 0, passed = 0, missed = 0;
  void setupStage() override {
    obstacles.clear();
    x = 88;
    speed = 1.4f;
    jump = passed = missed = 0;
  }
  void play() override {
    speed = clamp(speed + (input.down(Up)     ? .015f
                           : input.down(Down) ? -.025f
                                              : .004f),
                  .8f, 2.4f + level * .035f);
    x = clamp(x + (int(input.down(Right)) - int(input.down(Left))) * 2.f, 8.f, 168.f);
    if (jump)
      --jump;
    if (input.hit(Fire) && !jump) {
      jump = 45;
      sound(Sound::Move);
    }
    if (stageTick % unsigned(hazardInterval(70)) == 0)
      if (auto *e = obstacles.acquire()) {
        e->kind = 0;
        e->x = float(rng.range(8, 169));
        e->y = 30;
      }
    if (stageTick % unsigned(hazardInterval(150)) == 0)
      if (auto *e = obstacles.acquire()) {
        e->kind = 1;
        e->x = float(rng.range(30, 146));
        e->y = 30;
      }
    for (auto &e : obstacles.items)
      if (e.active) {
        float old = e.y;
        e.y += speed;
        if (old < 185 && e.y >= 185) {
          if (e.kind == 1) {
            if (std::abs(x - e.x) < 22) {
              ++passed;
              add(150);
              sound(Sound::Power);
            } else {
              ++missed;
              hit();
            }
          } else if (std::abs(x - e.x) < 11 && !jump) {
            hit();
            speed = .8f;
          } else if (jump && std::abs(x - e.x) < 15)
            add(75);
        }
        if (e.y > 218)
          e.active = false;
      }
    if (passed >= 6)
      clearStage();
    if (stageTick >= 60 * 90) {
      grace = 0;
      hit();
      stageTick = 0;
    }
  }

public:
  size_t activeEntities() const override {
    return obstacles.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    c.rect(0, 32, 176, 179, 0xc71c);
    for (int i = 0; i < 20; ++i)
      c.line((i * 31) % 176, 35 + int((i * 23 + tick) % 172), (i * 31) % 176 + 2,
             37 + int((i * 23 + tick) % 172), White);
    for (const auto &e : obstacles.items)
      if (e.active) {
        if (e.kind == 0) {
          c.line(int(e.x), int(e.y) - 12, int(e.x) - 8, int(e.y) + 3, Green);
          c.line(int(e.x), int(e.y) - 12, int(e.x) + 8, int(e.y) + 3, Green);
          c.rect(int(e.x) - 2, int(e.y), 4, 7, Orange);
        } else {
          c.line(int(e.x) - 24, int(e.y) - 13, int(e.x) - 24, int(e.y) + 3, Red);
          c.line(int(e.x) + 24, int(e.y) - 13, int(e.x) + 24, int(e.y) + 3, Red);
          c.line(int(e.x) - 24, int(e.y) - 13, int(e.x) + 24, int(e.y) - 13, Red);
        }
      }
    int yy = 185 - (jump ? int(std::sin(jump * 3.14159f / 45) * 18) : 0);
    c.circle(int(x), yy - 8, 4, Pink, true);
    c.rect(int(x) - 3, yy - 4, 6, 9, Cyan);
    c.line(int(x) - 5, yy + 2, int(x) - 3, yy + 14, Navy);
    c.line(int(x) + 3, yy + 2, int(x) + 5, yy + 14, Navy);
    c.text(5, 212, "GATES", Grey);
    c.number(42, 212, passed, Green);
    c.text(57, 212, "/6", Grey);
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "SLOPE COMPLETE", Green);
    }
  }
};
GameEntry describeAlpineRush() {
  return gameEntry<AlpineRush>(
      {"ALPINE RUSH",
       "CARVE SIX SLALOM GATES",
       {"L/R STEERS THE SKIS", "UP ACCELERATES DOWN BRAKES", "A JUMPS OVER TREES",
        "PASS BETWEEN RED FLAGS", "MISSING A GATE COSTS LIFE", "FINISH ALL 12 SLOPES"},
       Cyan,
       44,
       Left | Right | Up | Down | Fire});
}

#ifdef ARCADE_HOST_TESTS
bool verifyAlpineRush() {
  AlpineRush g;
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

  g.passed = 5;
  auto *e = g.obstacles.acquire();
  e->kind = 1;
  e->x = g.x;
  e->y = 184;
  g.speed = 2;
  g.stageTick = 1;
  g.play();
  HOST_CHECK(g.passed == 6 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
