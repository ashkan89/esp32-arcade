#include "games/Campaign.h"
namespace arcade {
class BarrelClimb final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyBarrelClimb();
#endif
  struct Barrel {
    bool active = false;
    float x = 0;
    int floor = 0, dir = 1;
  };
  Pool<Barrel, 20> barrels;
  float x = 20, jump = 0, vy = 0;
  int floor = 0, hp = 0, maximum = 0, hammer = 0;
  static int ground(int f) {
    return 202 - f * 37;
  }
  static int ladder(int f) {
    return f % 2 ? 25 : 149;
  }
  void setupStage() override {
    x = 20;
    floor = 0;
    jump = vy = 0;
    hammer = 0;
    barrels.clear();
    hp = maximum = bossRound() ? 5 + level / 4 : 0;
  }
  void play() override {
    x = clamp(x + (int(input.down(Right)) - int(input.down(Left))) * 1.6f, 7.f, 169.f);
    if (input.hit(Fire) && jump == 0) {
      vy = 3.6f;
      sound(Sound::Move);
    }
    if (vy != 0 || jump > 0) {
      jump += vy;
      vy -= .15f;
      if (jump <= 0) {
        jump = vy = 0;
      }
    }
    if (input.down(Up) && jump == 0 && floor < 4 && std::abs(x - ladder(floor)) < 9) {
      ++floor;
      grace = 30;
      sound(Sound::Power);
    }
    if (input.down(Down) && jump == 0 && floor > 0 && std::abs(x - ladder(floor - 1)) < 9) {
      --floor;
      grace = 30;
    }
    if (hammer)
      --hammer;
    if (input.hit(Special) && !hammer) {
      hammer = 30;
      if (floor == 4 && x > 110 && hp) {
        --hp;
        add(150);
        burst(144, 45);
      }
    }
    if (stageTick % unsigned(std::max(30, 100 - int(level) * 5)) == 0)
      if (auto *b = barrels.acquire()) {
        b->x = 155;
        b->floor = 4;
        b->dir = -1;
      }
    for (auto &b : barrels.items)
      if (b.active) {
        b.x += b.dir * (1 + level * .06f);
        if (b.x < 6 || b.x > 170) {
          if (b.floor == 0)
            b.active = false;
          else {
            --b.floor;
            b.dir = -b.dir;
            b.x = clamp(b.x, 6.f, 170.f);
          }
        }
        if (b.active && b.floor == floor && std::abs(x - b.x) < 10 && jump < 12) {
          if (hammer) {
            b.active = false;
            add(50);
          } else
            hit();
        }
      }
    if (floor == 4 && x > 115 && !hp)
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
    return barrels.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    for (int f = 0; f < 5; ++f) {
      c.rect(4, ground(f), 168, 4, Pink);
      if (f < 4) {
        int l = ladder(f);
        c.line(l - 4, ground(f) - 37, l - 4, ground(f), Cyan);
        c.line(l + 4, ground(f) - 37, l + 4, ground(f), Cyan);
        for (int y = ground(f) - 34; y < ground(f); y += 7)
          c.line(l - 4, y, l + 4, y, Cyan);
      }
    }
    c.rect(134, ground(4) - 22, 22, 22, hp ? Red : Green);
    c.text(138, ground(4) - 15, hp ? "K" : "G", White);
    for (const auto &b : barrels.items)
      if (b.active) {
        c.circle(int(b.x), ground(b.floor) - 6, 5, Orange, true);
        c.line(int(b.x) - 3, ground(b.floor) - 9, int(b.x) + 3, ground(b.floor) - 3, Yellow);
      }
    if (!grace || tick % 10 < 5) {
      int y = ground(floor) - int(jump) - 11;
      c.rect(int(x) - 4, y, 8, 10, Cyan);
      c.circle(int(x), y - 2, 3, White, true);
      if (hammer)
        c.rect(int(x) + 5, y - 7, 9, 5, Yellow);
    }
    if (hp)
      bossBar(c, hp, maximum);
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "RESCUE COMPLETE", Green);
    }
    c.centered(212, "A JUMP  B HAMMER", Grey);
  }
};
GameEntry describeBarrelClimb() {
  return gameEntry<BarrelClimb>(
      {"BARREL CLIMB",
       "RESCUE ABOVE THE ROLLERS",
       {"L/R WALK  A JUMPS", "UP/DOWN CLIMBS AT LADDERS", "B SWINGS A SHORT HAMMER",
        "REACH THE TOP GREEN EXIT", "BOSS: HAMMER THE TOP KING", "RESCUE ON ALL 12 TOWERS"},
       Orange,
       32,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyBarrelClimb() {
  BarrelClimb g;
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
  g.floor = 4;
  g.x = 135;
  g.input.pressed = Special;
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
