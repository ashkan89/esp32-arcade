#include "games/Campaign.h"
namespace arcade {
class NightRacer final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyNightRacer();
#endif
  struct Car {
    bool active = false;
    float lane = 0, depth = 0;
    float targetLane = 0;
  };
  Pool<Car, 16> cars;
  float position = 0, speed = 0, distance = 0;
  int time = 0, collision = 0;
  float curve() const {
    return std::sin(stageTick * .006f + level) * .45f;
  }
  void setupStage() override {
    cars.clear();
    position = 0;
    speed = 1;
    distance = 0;
    time = 60 * 55;
    collision = 0;
  }
  void play() override {
    speed = clamp(speed + (input.down(Fire)      ? .012f
                           : input.down(Special) ? -.025f
                                                 : -.003f),
                  .4f, 2.5f + level * .035f);
    position += (int(input.down(Right)) - int(input.down(Left))) * .025f - curve() * speed * .008f;
    if (std::abs(position) > 1) {
      speed = std::max(.4f, speed - .04f);
      position = clamp(position, -1.25f, 1.25f);
    }
    distance += speed * .035f;
    if (collision)
      --collision;
    if (stageTick % unsigned(std::max(34, 130 - int(level) * 5 - challenge() * 6)) == 0)
      if (auto *e = cars.acquire()) {
        e->lane = rng.range(-7, 8) * .1f;
        e->depth = .05f;
        e->targetLane = clamp(e->lane + float(rng.range(-2, 3)) * .15f, -.7f, .7f);
      }
    for (auto &e : cars.items)
      if (e.active) {
        e.depth += speed * .0035f;
        // Opponents commit early; their lane is stable throughout the collision band.
        if (e.depth < .65f)
          e.lane += clamp(e.targetLane - e.lane, -.004f, .004f);
        if (e.depth > .92f && e.depth < 1.02f && std::abs(position - e.lane) < .23f && !collision) {
          hit();
          speed = .4f;
          collision = 90;
          e.active = false;
        } else if (e.depth > 1.05f) {
          e.active = false;
          add(100);
        }
      }
    if (distance >= 110) {
      add(uint32_t(std::max(0, time) / 6));
      clearStage();
    }
    if (--time <= 0) {
      grace = 0;
      hit();
      time = 60 * 30;
    }
  }

public:
  size_t activeEntities() const override {
    return cars.count() + 1;
  }
  void render(Canvas &c) const override {
    c.circle(141, 55, 10, Grey, true);
    float bend = curve();
    for (int yy = 70; yy < 210; ++yy) {
      float d = (yy - 70) / 140.f;
      int half = 5 + int(d * d * 78),
          center = 88 + int(bend * (1 - d) * 60) - int(position * d * 20);
      c.line(center - half, yy, center + half, yy, Navy);
      if ((yy + int(distance * 8)) % 14 < 7) {
        c.pixel(center - half, yy, White);
        c.pixel(center + half, yy, White);
      }
      if ((yy + int(distance * 8)) % 22 < 8)
        c.pixel(center, yy, Yellow);
    }
    for (const auto &e : cars.items)
      if (e.active) {
        int yy = 70 + int(e.depth * 140), width = 4 + int(e.depth * 16),
            center = 88 + int(bend * (1 - e.depth) * 60) +
                     int((e.lane - position * .25f) * e.depth * 70);
        c.rect(center - width / 2, yy - width, width, width * 3 / 2, Pink);
        c.rect(center - width / 3, yy - width + 2, width * 2 / 3, width / 3, Cyan);
      }
    c.rect(69, 188, 38, 24, collision ? Red : Green);
    c.rect(76, 191, 24, 7, Cyan);
    c.rect(68, 205, 6, 8, Grey);
    c.rect(102, 205, 6, 8, Grey);
    c.text(3, 35, "TIME", Grey);
    c.number(32, 35, std::max(0, time) / 60, Yellow);
    c.text(93, 35, "KM/H", Grey);
    c.number(126, 35, uint32_t(speed * 80), Green);
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "CHECKPOINT CLEAR", Green);
    }
  }
};
GameEntry describeNightRacer() {
  return gameEntry<NightRacer>(
      {"NIGHT RACER",
       "CHASE THE NEON CHECKPOINT",
       {"L/R STEERS YOUR CAR", "HOLD A TO ACCELERATE", "HOLD B TO BRAKE FOR CURVES",
        "KEEP WITHIN ROAD MARKERS", "PASS CARS BEFORE DEADLINE", "REACH ALL 12 CHECKPOINTS"},
       Green,
       47,
       Left | Right | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyNightRacer() {
  NightRacer g;
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

  g.distance = 111;
  g.play();
  HOST_CHECK(g.transition > 0);
  g.start({});
  g.position = 0;
  g.speed = 2;
  auto *e = g.cars.acquire();
  e->lane = 0;
  e->depth = .93f;
  g.grace = 0;
  int lives = g.health;
  g.play();
  HOST_CHECK(g.health == lives - 1 && g.collision > 0);
  return true;
}
#endif
} // namespace arcade
