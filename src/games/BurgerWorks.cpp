#include "assets/Art.h"
#include "games/Campaign.h"
namespace arcade {
class BurgerWorks final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyBurgerWorks();
#endif
  struct Tray {
    int floor = 0, mask = 0, fall = 0, struck = 0;
    bool done = false;
  };
  std::array<Tray, 6> trays{};
  struct Chef {
    bool active = false;
    int x = 0, floor = 0, stun = 0, hp = 1;
  };
  Pool<Chef, 6> chefs;
  int x = 15, floor = 0, pepper = 3, delay = 0, attackCooldown = 0;
  static int ground(int f) {
    return 199 - f * 40;
  }
  void setupStage() override {
    x = 15;
    floor = 0;
    pepper = 3;
    delay = 0;
    attackCooldown = 0;
    chefs.clear();
    for (int i = 0; i < 6; ++i)
      trays[i] = {1 + i / 2, 0, 0, 0, false};
    for (int i = 0; i < std::min(6, 2 + int(level) / 3); ++i)
      if (auto *e = chefs.acquire()) {
        e->x = 155 - i * 15;
        e->floor = 1 + i % 3;
        e->hp = bossRound() && i == 0 ? 3 : 1;
      }
  }
  void play() override {
    // Recovery after ingredients are gone keeps the commander fight winnable.
    if (bossRound() && stageTick % 600 == 0 && pepper < 3)
      ++pepper;
    if (attackCooldown)
      --attackCooldown;
    if (input.hit(Fire) && !attackCooldown) {
      attackCooldown = 25;
      for (auto &e : chefs.items)
        if (e.active && e.stun && e.floor == floor && std::abs(e.x - x) < 22) {
          if (--e.hp == 0) {
            e.active = false;
            add(150);
          }
          sound(Sound::Hit);
        }
    }
    if (++delay >= 3) {
      delay = 0;
      x = clamp(x + (int(input.down(Right)) - int(input.down(Left))), 7, 168);
    }
    if (input.hit(Up) && floor < 4 && (x < 20 || x > 150))
      ++floor;
    if (input.hit(Down) && floor > 0 && (x < 20 || x > 150))
      --floor;
    if (input.hit(Special) && pepper) {
      --pepper;
      for (auto &e : chefs.items)
        if (e.active && e.floor == floor && std::abs(e.x - x) < 45)
          e.stun = 180;
      sound(Sound::Power);
    }
    for (int i = 0; i < 6; ++i) {
      auto &t = trays[i];
      if (t.done)
        continue;
      int left = 30 + (i % 2) * 65;
      if (!t.fall && floor == t.floor && x >= left && x < left + 48) {
        int bit = (x - left) / 12;
        t.mask |= 1 << bit;
        if (t.mask == 15) {
          t.fall = 1;
          sound(Sound::Hit);
        }
      }
      if (t.fall) {
        t.fall += 2;
        for (size_t k = 0; k < chefs.items.size(); ++k) {
          auto &e = chefs.items[k];
          if (!(t.struck & (1 << k)) && e.active && e.x >= left && e.x < left + 48 &&
              std::abs(ground(t.floor) + t.fall - ground(e.floor)) < 8) {
            t.struck |= 1 << k;
            if (--e.hp == 0)
              e.active = false;
            else
              e.stun = 120;
            add(200);
          }
        }
        if (t.fall >= t.floor * 40) {
          t.done = true;
          add(100);
        }
      }
    }
    for (auto &e : chefs.items)
      if (e.active) {
        if (e.stun) {
          --e.stun;
          continue;
        }
        if (stageTick % unsigned(std::max(2, 6 - int(level) / 4 - challenge() / 2)) == 0)
          e.x += (e.floor == floor ? x : (e.x < 88 ? 15 : 160)) > e.x ? 1 : -1;
        e.x = clamp(e.x, 7, 168);
        if (stageTick % 60 == 0 && e.floor != floor && (e.x < 20 || e.x > 150))
          e.floor += floor > e.floor ? 1 : -1;
        if (e.floor == floor && std::abs(e.x - x) < 8) {
          hit();
          x = 15;
          floor = 0;
        }
      }
    bool done = true;
    for (const auto &t : trays)
      done &= t.done;
    if (done) {
      if (!bossRound() || !chefs.count())
        clearStage();
    }
  }

public:
  bool bossEncounter() const override {
    return bossRound() && chefs.count() > 0;
  }
  int bossHealth() const override {
    int n = 0;
    for (const auto &e : chefs.items)
      if (e.active)
        n = std::max(n, e.hp);
    return bossRound() ? n : 0;
  }
  size_t activeEntities() const override {
    return chefs.count() + sparks.count() + 7;
  }
  void render(Canvas &c) const override {
    art::scene(c, 34);
    for (int f = 0; f < 5; ++f)
      art::platform(c, 5, ground(f), 166, 5, art::Sprite::SteelPlatform);
    for (int l : {15, 160}) {
      c.line(l - 3, 39, l - 3, 199, Cyan);
      c.line(l + 3, 39, l + 3, 199, Cyan);
      for (int yy = 43; yy < 199; yy += 8)
        c.line(l - 3, yy, l + 3, yy, Cyan);
    }
    for (int i = 0; i < 6; ++i) {
      const auto &t = trays[i];
      if (!t.done)
        for (int b = 0; b < 4; ++b)
          art::sprite(c, (t.mask & (1 << b)) ? art::Sprite::Lettuce : i / 2 == 0 ? art::Sprite::Bun
                          : i / 2 == 1 ? art::Sprite::Cheese : art::Sprite::Patty,
                      30 + i % 2 * 65 + b * 12, ground(t.floor) + t.fall - 6, 11, 6);
    }
    for (const auto &e : chefs.items)
      if (e.active)
        art::object(c, art::Sprite::Sausage, e.x, ground(e.floor) - 8, 14, 18);
    art::sprite(c, art::Sprite::Chef, x - 8, ground(floor) - 19, 16, 19);
    c.text(5, 211, "B PEPPER", Grey);
    c.number(65, 211, pepper, Yellow);
    if (bossRound()) {
      c.text(5, 24, "BOSS", Pink);
      c.rect(35, 24, bossHealth() * 42, 4, Red);
    }
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "ORDER COMPLETE", Green);
    }
  }
};
GameEntry describeBurgerWorks() {
  return gameEntry<BurgerWorks>(
      {"BURGER WORKS",
       "WALK THE BURGER LAYERS",
       {"L/R WALK OVER INGREDIENTS", "UP/DOWN AT EDGE LADDERS", "WALK ALL FOUR TRAY PARTS",
        "B PEPPER STUNS NEAR CHEFS", "A HITS STUNNED NEAR CHEFS", "BOSS: CLEAR CHEFS AND FOOD"},
       Orange,
       34,
       Left | Right | Up | Down | Special | Fire});
}

#ifdef ARCADE_HOST_TESTS
bool verifyBurgerWorks() {
  BurgerWorks g;
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
  for (auto &t : g.trays)
    t.done = true;
  g.chefs.clear();
  auto *e = g.chefs.acquire();
  e->x = g.x;
  e->floor = g.floor;
  e->stun = 100;
  e->hp = 1;
  g.input.pressed = Fire;
  g.play();
  HOST_CHECK(!g.chefs.count() && g.transition > 0);
  g.start({});
  g.level = 4;
  g.setupStage();
  g.pepper = 0;
  g.stageTick = 600;
  g.play();
  HOST_CHECK(g.pepper == 1);
  g.stageTick = 601;
  g.input.pressed = Special;
  g.play();
  HOST_CHECK(g.pepper == 0);
  return true;
}
#endif
} // namespace arcade
