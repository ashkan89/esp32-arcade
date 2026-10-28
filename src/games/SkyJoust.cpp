#include "assets/Art.h"
#include "games/Campaign.h"
namespace arcade {
class SkyJoust final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifySkyJoust();
#endif
  struct Rider {
    bool active = false;
    float x = 0, y = 0, vy = 0;
    int hp = 1;
  };
  Pool<Rider, 12> riders;
  float x = 88, y = 130, vy = 0;
  int eggs = 0;
  void setupStage() override {
    riders.clear();
    x = 88;
    y = 130;
    vy = 0;
    eggs = 0;
    for (int i = 0; i < std::min(12, 3 + int(level) / 2); ++i)
      if (auto *e = riders.acquire()) {
        e->x = 20 + i % 4 * 40;
        e->y = 60 + i / 4 * 26;
        e->hp = bossRound() && i == 0 ? 5 : 1;
      }
  }
  void play() override {
    x += (int(input.down(Right)) - int(input.down(Left))) * 2;
    if (x < 3)
      x = 173;
    if (x > 173)
      x = 3;
    if (input.hit(Fire)) {
      vy = -2.7f;
      sound(Sound::Move);
    }
    vy = std::min(3.f, vy + .08f);
    y += vy;
    if (y < 38) {
      y = 38;
      vy = .3f;
    }
    if (y > 205) {
      hit();
      y = 120;
      vy = 0;
    }
    for (auto &e : riders.items)
      if (e.active) {
        e.x += clamp(x - e.x, -.7f, .7f);
        e.vy = clamp(e.vy + .06f, -2.8f, 2.8f);
        if (e.y > 175 || stageTick % unsigned(hazardInterval(75 + int(e.x) % 30)) == 0)
          e.vy = -2.2f - level * .035f;
        e.y = clamp(e.y + e.vy, 38.f, 202.f);
        if (near(x, y, e.x, e.y, 13)) {
          if (y < e.y - 4 && vy >= -.2f) {
            if (--e.hp == 0) {
              e.active = false;
              add(150);
              ++eggs;
            } else {
              e.y = clamp(e.y + 26, 38.f, 202.f);
              e.vy = .5f;
            }
            vy = -2.5f;
            grace = 35;
            burst(e.x, e.y);
          } else if (!grace) {
            hit();
            y = 150;
            vy = -2;
          }
        }
      }
    if (input.hit(Special) && eggs) {
      --eggs;
      grace = 120;
      sound(Sound::Power);
    }
    if (!riders.count())
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return bossRound() && riders.count() > 0;
  }
  int bossHealth() const override {
    if (!bossRound())
      return 0;
    int n = 0;
    for (const auto &e : riders.items)
      if (e.active)
        n = std::max(n, e.hp);
    return n;
  }
  size_t activeEntities() const override {
    return riders.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    art::scene(c, 33);
    scenery(c, Yellow);
    c.rect(0, 210, 176, 10, Red);
    for (int i = 0; i < 3; ++i)
      art::platform(c, i * 60 + 5, 163 - i % 2 * 35, 42, 7);
    for (const auto &e : riders.items)
      if (e.active) {
        art::object(c, art::Sprite::Bird, int(e.x), int(e.y), 19, 17, true);
        art::object(c, art::Sprite::EnemySamurai, int(e.x), int(e.y) - 9, 8, 10, true);
        c.line(int(e.x) - 10, int(e.y) + 3, int(e.x) + 10, int(e.y) + 3, White);
      }
    if (!grace || tick % 10 < 5) {
      art::object(c, art::Sprite::Bird, int(x), int(y), 20, 20);
      c.line(int(x) - 12, int(y) + (tick % 12 < 6 ? -3 : 4), int(x) + 12, int(y) + 3, Cyan);
      art::object(c, art::Sprite::Samurai, int(x), int(y) - 9, 8, 10);
    }
    if (bossRound())
      bossBar(c, bossHealth(), 5);
    c.text(5, 213, "B SHIELD TOKENS", White);
    c.number(130, 213, eggs, Yellow);
  }
};
GameEntry describeSkyJoust() {
  return gameEntry<SkyJoust>(
      {"SKY JOUST",
       "WIN FROM HIGHER GROUND",
       {"L/R STEERS  TAP A TO FLAP", "STRIKE FROM ABOVE TO WIN", "LOWER RIDER LOSES A LIFE",
        "KILLS EARN B SHIELD TOKENS", "BOSS NEEDS FIVE HIGH HITS", "WIN ALL 12 SKY ARENAS"},
       Yellow,
       33,
       Left | Right | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifySkyJoust() {
  SkyJoust g;
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
  g.riders.clear();
  auto *e = g.riders.acquire();
  e->x = 88;
  e->y = 110;
  e->hp = 1;
  g.x = 88;
  g.y = 101;
  g.vy = 0;
  g.stageTick = 1;
  g.play();
  HOST_CHECK(!g.riders.count() && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
