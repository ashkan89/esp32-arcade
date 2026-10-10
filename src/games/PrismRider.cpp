#include "games/Campaign.h"
namespace arcade {
class PrismRider final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyPrismRider();
#endif
  struct Climber {
    bool active = false;
    int lane = 0, hp = 1;
    float depth = 0;
  };
  Pool<Climber, 24> foes;
  Pool<Projectile, 24> bolts;
  int lane = 0, spawned = 0, cool = 0, jump = 0, bossLane = 0, hp = 0, maximum = 0;
  static int px(int l, float d) {
    return 88 + int(std::cos(l * 6.283185f / 12) * (12 + d * 67));
  }
  static int py(int l, float d) {
    return 118 + int(std::sin(l * 6.283185f / 12) * (12 + d * 75));
  }
  void setupStage() override {
    foes.clear();
    bolts.clear();
    spawned = cool = jump = 0;
    lane = 0;
    bossLane = 6;
    hp = maximum = bossRound() ? 18 + level : 0;
  }
  void play() override {
    if (input.nav(Left))
      lane = (lane + 11) % 12;
    if (input.nav(Right))
      lane = (lane + 1) % 12;
    if (jump)
      --jump;
    if (input.hit(Special) && !jump) {
      lane = (lane + 6) % 12;
      grace = 90;
      jump = 300;
    }
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(bolts, float(lane), 1, 0, -.045f);
      cool = 10;
      sound(Sound::Shot);
    }
    if (hp && stageTick % 90 == 0)
      bossLane = (bossLane + 1 + int(level) / 4) % 12;
    int interval = std::max(15, 65 - int(level) * 3);
    if (stageTick % unsigned(interval) == 0 && (hp || spawned < 16 + level * 2))
      if (auto *e = foes.acquire()) {
        e->lane = hp ? bossLane : rng.range(0, 12);
        e->hp = level > 6 ? 2 : 1;
        ++spawned;
      }
    for (auto &p : bolts.items)
      if (p.active) {
        p.y += p.vy;
        if (p.y <= 0) {
          if (hp && int(p.x) == bossLane) {
            if (--hp == 0) {
              foes.clear();
              clearStage();
            }
            burst(float(px(bossLane, 0)), float(py(bossLane, 0)));
          }
          p.active = false;
        }
      }
    for (auto &e : foes.items)
      if (e.active) {
        e.depth += .0018f + level * .00012f + options.difficulty * .0002f;
        if (level >= 5 && stageTick % 150 == 0)
          e.lane = (e.lane + 1) % 12;
        for (auto &p : bolts.items)
          if (p.active && int(p.x) == e.lane && std::abs(p.y - e.depth) < .055f) {
            p.active = false;
            if (--e.hp == 0) {
              e.active = false;
              add(70);
              burst(float(px(e.lane, e.depth)), float(py(e.lane, e.depth)));
            }
            break;
          }
        if (e.active && e.depth >= 1) {
          e.active = false;
          if (e.lane == lane)
            hit();
          else {
            loseLife();
            grace = 90;
          }
        }
      }
    if (!bossRound() && spawned >= 16 + level * 2 && !foes.count())
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
    return foes.count() + bolts.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    for (int l = 0; l < 12; ++l) {
      c.line(px(l, 0), py(l, 0), px(l, 1), py(l, 1), Navy);
      c.line(px(l, 1), py(l, 1), px((l + 1) % 12, 1), py((l + 1) % 12, 1), Cyan);
    }
    for (const auto &e : foes.items)
      if (e.active)
        c.circle(px(e.lane, e.depth), py(e.lane, e.depth), 4, e.hp > 1 ? Pink : Green, true);
    for (const auto &p : bolts.items)
      if (p.active)
        c.circle(px(int(p.x), p.y), py(int(p.x), p.y), 2, Yellow, true);
    if (!grace || tick % 10 < 5)
      c.circle(px(lane, 1), py(lane, 1), 6, White);
    if (hp) {
      c.circle(px(bossLane, 0), py(bossLane, 0), 7, Red, true);
      bossBar(c, hp, maximum);
    }
    scenery(c, Cyan);
    c.centered(211, jump ? "JUMP RECHARGING" : "B: HALF-TUBE JUMP", Grey);
  }
};
GameEntry describePrismRider() {
  return gameEntry<PrismRider>(
      {"PRISM RIDER",
       "DEFEND THE NEON TUBE",
       {"L/R MOVES AROUND THE RIM", "A FIRES INTO YOUR LANE", "B JUMPS HALF A TUBE",
        "STOP CLIMBERS BEFORE RIM", "BOSS CORE CHANGES LANES", "CLEAR 12 TUBES TO WIN"},
       Pink,
       28,
       Left | Right | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyPrismRider() {
  PrismRider g;
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
  g.foes.clear();
  g.hp = 1;
  g.stageTick = 1;
  g.bossLane = 2;
  projectile(g.bolts, 2, .01f, 0, -.045f);
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
