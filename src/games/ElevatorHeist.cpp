#include "games/Campaign.h"
namespace arcade {
class ElevatorHeist final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyElevatorHeist();
#endif
  struct Agent {
    bool active = false;
    float x = 0;
    int floor = 0, hp = 1;
  };
  Pool<Agent, 10> agents;
  Pool<Projectile, 20> shots, hostile;
  std::array<bool, 5> documents{};
  float x = 88;
  int floor = 4, facing = 1, cool = 0, move = 0;
  static int ground(int f) {
    return 200 - f * 34;
  }
  void setupStage() override {
    documents.fill(false);
    agents.clear();
    shots.clear();
    hostile.clear();
    x = 88;
    floor = 4;
    facing = 1;
    cool = move = 0;
    for (int i = 0; i < 5; ++i)
      if (auto *e = agents.acquire()) {
        e->x = i % 2 ? 145 : 30;
        e->floor = i;
        e->hp = bossRound() && i == 0 ? 8 : 1 + level / 6;
      }
  }
  void play() override {
    int dx = int(input.down(Right)) - int(input.down(Left));
    if (dx)
      facing = dx;
    x = clamp(x + dx * 1.6f, 8.f, 168.f);
    if (move)
      --move;
    if (!move && std::abs(x - 88) < 10) {
      if (input.down(Up) && floor < 4) {
        ++floor;
        move = 22;
      } else if (input.down(Down) && floor > 0) {
        --floor;
        move = 22;
      }
    }
    int door = floor % 2 ? 22 : 154;
    if (std::abs(x - door) < 9 && input.hit(Up) && !documents[floor]) {
      documents[floor] = true;
      add(200);
      sound(Sound::Power);
    }
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x, ground(floor) - 10, facing * 3.f, 0);
      cool = 16;
      sound(Sound::Shot);
    }
    if (input.hit(Special) && floor > 0 && std::abs(x - 88) < 12) {
      --floor;
      move = 22;
      grace = 35;
    }
    for (auto &e : agents.items)
      if (e.active) {
        e.x += clamp(x - e.x, -.3f, .3f);
        if (stageTick % unsigned(std::max(28, 120 - int(level) * 4 - challenge() * 6)) == 0)
          projectile(hostile, e.x, ground(e.floor) - 10, x > e.x ? 2.f : -2.f, 0);
        if (e.floor == floor && std::abs(e.x - x) < 10)
          hit();
      }
    moveProjectiles(shots);
    moveProjectiles(hostile);
    for (auto &p : shots.items)
      if (p.active)
        for (auto &e : agents.items)
          if (e.active && projectileNear(p, e.x, float(ground(e.floor) - 10), 8)) {
            p.active = false;
            if (--e.hp == 0) {
              e.active = false;
              add(100);
              burst(e.x, p.y);
            }
            break;
          }
    for (auto &p : hostile.items)
      if (p.active && projectileNear(p, x, float(ground(floor) - 10), 8)) {
        p.active = false;
        hit();
      }
    bool all = true;
    for (bool d : documents)
      all &= d;
    if (all && floor == 0 && x > 155 && (!bossRound() || !agents.count()))
      clearStage();
  }

public:
  bool bossEncounter() const override {
    for (const auto &e : agents.items)
      if (e.active && e.hp > 2)
        return true;
    return false;
  }
  int bossHealth() const override {
    int hp = 0;
    for (const auto &e : agents.items)
      if (e.active)
        hp = std::max(hp, e.hp);
    return bossRound() ? hp : 0;
  }
  size_t activeEntities() const override {
    return agents.count() + shots.count() + hostile.count() + sparks.count() + 6;
  }
  void render(Canvas &c) const override {
    for (int f = 0; f < 5; ++f) {
      int yy = ground(f);
      c.line(0, yy, 175, yy, Grey);
      int door = f % 2 ? 22 : 154;
      c.rect(door - 8, yy - 25, 16, 24, documents[f] ? Green : Red);
      c.text(door - 2, yy - 17, "D", White);
    }
    c.rect(76, 36, 24, 166, Navy);
    c.rect(76, ground(floor) - 27, 24, 27, Cyan, false);
    for (const auto &e : agents.items)
      if (e.active)
        c.rect(int(e.x) - 5, ground(e.floor) - 16, 10, 16, e.hp > 2 ? Red : Pink);
    if (!grace || tick % 10 < 5)
      c.rect(int(x) - 4, ground(floor) - 17, 8, 17, White);
    drawProjectiles(c, shots, Yellow);
    drawProjectiles(c, hostile, Pink);
    if (bossRound())
      bossBar(c, bossHealth(), 8);
    scenery(c, Cyan);
    c.centered(212, "UP AT RED DOORS: FILE", Grey);
  }
};
GameEntry describeElevatorHeist() {
  return gameEntry<ElevatorHeist>(
      {"ELEVATOR HEIST",
       "FIVE FILES THEN ESCAPE",
       {"L/R WALK  A SHOOTS", "UP/DOWN IN CENTER LIFT", "UP AT RED DOOR GETS FILE",
        "B IN LIFT DROPS A FLOOR", "BOSS: CLEAR SECURITY TEAM", "FIVE FILES THEN RIGHT EXIT"},
       Cyan,
       43,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyElevatorHeist() {
  ElevatorHeist g;
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
  g.agents.clear();
  g.floor = 0;
  g.x = 165;
  g.documents.fill(true);
  g.play();
  HOST_CHECK(g.transition > 0);
  return true;
}
#endif
} // namespace arcade
