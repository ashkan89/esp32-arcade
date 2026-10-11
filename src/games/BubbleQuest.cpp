#include "games/Campaign.h"
namespace arcade {
class BubbleQuest final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyBubbleQuest();
#endif
  struct Foe {
    bool active = false;
    float x = 0, y = 0;
    float vy = 0;
    int dir = 1, trap = 0, hp = 1;
  };
  Pool<Foe, 12> foes;
  Pool<Projectile, 16> bubbles;
  float x = 30, y = 194, vy = 0;
  int facing = 1, cool = 0;
  void setupStage() override {
    foes.clear();
    bubbles.clear();
    x = 30;
    y = 194;
    vy = 0;
    cool = 0;
    for (int i = 0; i < std::min(12, 3 + int(level) / 2); ++i)
      if (auto *e = foes.acquire()) {
        e->x = 30 + i % 3 * 55;
        e->y = 194 - i / 3 * 42;
        e->dir = i % 2 ? 1 : -1;
        e->hp = bossRound() && i == 0 ? 5 : 1;
      }
  }
  static int platform(float xx, int row) {
    return row == 0 || (row % 2 ? xx < 135 : xx > 40) ? 204 - row * 42 : -100;
  }
  void play() override {
    int dx = int(input.down(Right)) - int(input.down(Left));
    if (dx)
      facing = dx;
    x = clamp(x + dx * 1.8f, 6.f, 170.f);
    bool grounded = false;
    for (int r = 0; r < 5; ++r)
      grounded |= std::abs(y + 10 - platform(x, r)) < 1;
    if (input.hit(Fire) && grounded) {
      vy = -4.1f;
      sound(Sound::Move);
    }
    float old = y;
    y += vy;
    vy = std::min(4.f, vy + .13f);
    if (vy >= 0)
      for (int r = 0; r < 5; ++r) {
        int yy = platform(x, r);
        if (old + 10 <= yy && y + 10 >= yy) {
          y = float(yy - 10);
          vy = 0;
        }
      }
    if (y > 215) {
      hit();
      y = 194;
      vy = 0;
    }
    if (cool)
      --cool;
    if (input.down(Special) && !cool) {
      projectile(bubbles, x + facing * 9, y, 2.3f * facing, -.12f);
      cool = 18;
      sound(Sound::Shot);
    }
    moveProjectiles(bubbles);
    for (auto &p : bubbles.items)
      if (p.active) {
        p.vx *= .98f;
        p.vy -= .007f;
      }
    for (auto &e : foes.items)
      if (e.active) {
        if (e.trap) {
          --e.trap;
          e.y = std::max(float(Top + 12), e.y - .3f);
          if (near(x, y, e.x, e.y, 15)) {
            e.trap = 0;
            if (--e.hp == 0) {
              e.active = false;
              add(150);
              burst(e.x, e.y);
            } else {
              e.x = clamp(e.x + facing * 22, 8.f, 168.f);
              e.vy = 0;
              grace = 60;
            }
          }
        } else {
          float oldY = e.y;
          e.vy = std::min(4.f, e.vy + .13f);
          e.y += e.vy;
          for (int r = 0; r < 5; ++r) {
            int yy = platform(e.x, r);
            if (oldY + 10 <= yy && e.y + 10 >= yy) {
              e.y = float(yy - 10);
              e.vy = 0;
            }
          }
          e.x += e.dir * hazardSpeed(.45f + level * .035f);
          if (e.x < 8 || e.x > 168) {
            e.dir = -e.dir;
            e.x = clamp(e.x, 8.f, 168.f);
          }
          for (auto &p : bubbles.items)
            if (p.active && projectileNear(p, e.x, e.y, 9)) {
              p.active = false;
              e.trap = hazardInterval(300);
              break;
            }
          if (!e.trap && near(x, y, e.x, e.y, 11)) {
            hit();
            x = 30;
            y = 194;
            vy = 0;
          }
        }
      }
    if (!foes.count())
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return bossRound() && foes.count() > 0;
  }
  int bossHealth() const override {
    int n = 0;
    for (const auto &e : foes.items)
      if (e.active)
        n = std::max(n, e.hp);
    return bossRound() ? n : 0;
  }
  size_t activeEntities() const override {
    return foes.count() + bubbles.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    for (int r = 0; r < 5; ++r) {
      int left = r && r % 2 == 0 ? 40 : 0, right = r % 2 ? 135 : 176;
      c.rect(left, 204 - r * 42, right - left, 4, Cyan);
    }
    for (const auto &e : foes.items)
      if (e.active) {
        c.circle(int(e.x), int(e.y), 5, e.hp > 1 ? Red : Orange, true);
        if (e.trap)
          c.circle(int(e.x), int(e.y), 10, Pink);
      }
    for (const auto &p : bubbles.items)
      if (p.active)
        c.circle(int(p.x), int(p.y), 5, Pink);
    if (!grace || tick % 10 < 5) {
      c.circle(int(x), int(y), 7, Green, true);
      c.pixel(int(x) + facing * 3, int(y) - 3, White);
    }
    if (bossRound())
      bossBar(c, bossHealth(), 5);
    scenery(c, Green);
    c.centered(212, "A JUMP  B BUBBLES", Grey);
  }
};
GameEntry describeBubbleQuest() {
  return gameEntry<BubbleQuest>(
      {"BUBBLE QUEST",
       "TRAP AND POP THE FOES",
       {"L/R MOVES  A JUMPS", "HOLD B TO BLOW BUBBLES", "TOUCH TRAPPED FOES TO POP",
        "BUBBLES FLOAT UP PLATFORMS", "BOSS NEEDS FIVE TRAP POPS", "CLEAR 12 BUBBLE ROOMS"},
       Green,
       35,
       Left | Right | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyBubbleQuest() {
  BubbleQuest g;
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
  auto *e = g.foes.acquire();
  e->x = g.x;
  e->y = g.y;
  e->trap = 20;
  e->hp = 1;
  g.play();
  HOST_CHECK(!g.foes.count() && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
