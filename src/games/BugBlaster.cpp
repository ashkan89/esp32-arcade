#include "games/Campaign.h"
namespace arcade {
class BugBlaster final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyBugBlaster();
#endif
  struct Segment {
    bool active = false;
    float x = 0, y = 0;
    int dir = 1, hp = 1, vertical = 1;
  };
  Pool<Segment, 24> bugs;
  Pool<Projectile, 24> shots;
  std::array<uint8_t, 120> mushrooms{};
  float x = 88, y = 200;
  int cool = 0;
  void setupStage() override {
    bugs.clear();
    shots.clear();
    mushrooms.fill(0);
    x = 88;
    y = 200;
    cool = 0;
    for (int i = 0; i < 18; ++i)
      mushrooms[rng.range(0, 100)] = 2;
    for (int i = 0; i < std::min(24, 8 + int(level)); ++i)
      if (auto *s = bugs.acquire()) {
        s->x = 12 + i * 7;
        s->y = 40;
        s->hp = bossRound() ? 3 : 1;
      }
  }
  void play() override {
    x = clamp(x + (int(input.down(Right)) - int(input.down(Left))) * 2.4f, 6.f, 170.f);
    y = clamp(y + (int(input.down(Down)) - int(input.down(Up))) * 1.8f, 176.f, 211.f);
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(shots, x, y, 0, -4.5f);
      cool = 10;
      sound(Sound::Shot);
    }
    moveProjectiles(shots);
    for (auto &s : bugs.items)
      if (s.active) {
        float nx = s.x + s.dir * (.5f + level * .07f + challenge() * .12f);
        int cell = clamp((int(s.y) - 35) / 15, 0, 11) * 10 + clamp(int(nx) / 18, 0, 9);
        if (nx < 5 || nx > 171 || mushrooms[cell]) {
          s.dir = -s.dir;
          s.y += 15 * s.vertical;
          if (s.y > 205) {
            s.y = 205;
            s.vertical = -1;
          }
          if (s.y < 178 && s.vertical < 0) {
            s.y = 178;
            s.vertical = 1;
          }
        } else
          s.x = nx;
        if (bossRound() && stageTick % 240 < 60)
          s.x = clamp(s.x + (x > s.x ? .2f : -.2f), 5.f, 171.f);
        if (near(s.x, s.y, x, y, 10))
          hit();
        for (auto &p : shots.items)
          if (p.active && projectileNear(p, s.x, s.y, 7)) {
            p.active = false;
            if (--s.hp == 0) {
              s.active = false;
              add(60);
              burst(s.x, s.y);
              int q = clamp((int(s.y) - 35) / 15, 0, 11) * 10 + clamp(int(s.x) / 18, 0, 9);
              mushrooms[q] = 2;
            }
            break;
          }
      }
    for (auto &p : shots.items)
      if (p.active) {
        int row = (int(p.y) - 35) / 15;
        if (p.y >= 35 && row < 12) {
          int q = row * 10 + clamp(int(p.x) / 18, 0, 9);
          if (mushrooms[q]) {
            --mushrooms[q];
            p.active = false;
            add(2);
          }
        }
      }
    if (!bugs.count())
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return bossRound() && bugs.count() > 0;
  }
  int bossHealth() const override {
    int n = 0;
    for (const auto &s : bugs.items)
      if (s.active)
        n += s.hp;
    return bossRound() ? n : 0;
  }
  size_t activeEntities() const override {
    return bugs.count() + shots.count() + sparks.count() + 121;
  }
  void render(Canvas &c) const override {
    for (int i = 0; i < 120; ++i)
      if (mushrooms[i]) {
        int xx = i % 10 * 18 + 4, yy = 35 + i / 10 * 15;
        c.circle(xx + 4, yy + 4, 4, Pink, true);
        c.rect(xx + 3, yy + 5, 3, 5, White);
      }
    for (const auto &s : bugs.items)
      if (s.active) {
        c.circle(int(s.x), int(s.y), 5, bossRound() ? Red : Green, true);
        c.pixel(int(s.x) - 2, int(s.y) - 2, White);
      }
    if (!grace || tick % 10 < 5)
      ship(c, x, y, Cyan);
    drawProjectiles(c, shots, Yellow);
    scenery(c, Green);
    if (bossRound())
      bossBar(c, bossHealth(), 3 * std::min(24, 8 + int(level)));
  }
};
GameEntry describeBugBlaster() {
  return gameEntry<BugBlaster>(
      {"BUG BLASTER",
       "STOP THE SEGMENT SWARM",
       {"D-PAD MOVES IN LOWER FIELD", "HOLD A TO SHOOT THE SWARM", "MUSHROOMS TURN THE BUGS",
        "SHOOT MUSHROOMS TO OPEN", "EVERY FOURTH: ARMORED BUG", "CLEAR 12 SWARMS TO WIN"},
       Green,
       26,
       Left | Right | Up | Down | Fire});
}

#ifdef ARCADE_HOST_TESTS
bool verifyBugBlaster() {
  BugBlaster g;
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
  g.bugs.clear();
  auto *e = g.bugs.acquire();
  e->x = 50;
  e->y = 100;
  e->hp = 1;
  projectile(g.shots, 50, 100, 0, 0);
  g.play();
  HOST_CHECK(g.bugs.count() == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
