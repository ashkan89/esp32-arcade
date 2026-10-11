#include "games/Campaign.h"
namespace arcade {
class TunnelDigger final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyTunnelDigger();
#endif
  struct Monster {
    bool active = false;
    int p = 0, inflate = 0, hp = 1;
  };
  Pool<Monster, 12> enemies;
  std::array<bool, 143> dug{};
  int player = 71, direction = 1, pump = 0, rock = 0, rockY = 0, rockX = 0, rockHits = 0;
  void setupStage() override {
    enemies.clear();
    dug.fill(false);
    player = 71;
    dug[player] = true;
    pump = 0;
    rock = 2;
    rockY = -1;
    rockHits = 0;
    direction = 1;
    for (int i = 0; i < std::min(12, 3 + int(level) / 2); ++i)
      if (auto *e = enemies.acquire()) {
        e->p = 13 + (i % 3) * 3 + (i / 3) * 33;
        dug[e->p] = true;
        e->hp = bossRound() && i == 0 ? 5 : 1;
      }
  }
  void play() override {
    if (tick % 7 == 0) {
      auto movement = input;
      movement.repeated |= input.held;
      int p = gridMove(movement, player, 11, 13);
      if (p != player) {
        direction = p - player;
        player = p;
        dug[p] = true;
        sound(Sound::Move);
      }
    }
    if (input.hit(Special) && rock && rockY < 0) {
      --rock;
      rockX = player % 11;
      rockY = player / 11 * 13 + 35;
      rockHits = 0;
    }
    if (rockY >= 0) {
      rockY += 2;
      for (size_t k = 0; k < enemies.items.size(); ++k) {
        auto &e = enemies.items[k];
        if (!(rockHits & (1 << k)) && e.active && e.p % 11 == rockX &&
            std::abs(41 + e.p / 11 * 13 - rockY) < 8) {
          rockHits |= 1 << k;
          if (--e.hp == 0)
            e.active = false;
          add(150);
          burst(float(15 + rockX * 14), float(rockY));
        }
      }
      if (rockY > 208)
        rockY = -1;
    }
    bool pumping = input.down(Fire);
    if (pumping)
      sound(stageTick % 20 == 0 ? Sound::Shot : Sound::None);
    for (auto &e : enemies.items)
      if (e.active) {
        int dx = e.p % 11 - player % 11, dy = e.p / 11 - player / 11;
        bool aimed = (direction == 1 && dy == 0 && dx > 0 && dx <= 3) ||
                     (direction == -1 && dy == 0 && dx < 0 && dx >= -3) ||
                     (direction == 11 && dx == 0 && dy > 0 && dy <= 3) ||
                     (direction == -11 && dx == 0 && dy < 0 && dy >= -3);
        if (pumping && aimed) {
          if (++e.inflate >= 45) {
            e.inflate = 0;
            if (--e.hp == 0) {
              e.active = false;
              add(100);
              burst(float(15 + e.p % 11 * 14), float(41 + e.p / 11 * 13));
            }
          }
        } else {
          if (e.inflate)
            e.inflate--;
          if (stageTick % unsigned(std::max(10, 36 - int(level) - challenge() * 2)) == 0) {
            int target =
                e.p + (std::abs(dx) > std::abs(dy) ? (dx > 0 ? -1 : 1) : (dy > 0 ? -11 : 11));
            if (target >= 0 && target < 143 &&
                std::abs(target % 11 - e.p % 11) + std::abs(target / 11 - e.p / 11) == 1) {
              if (dug[target] || stageTick % 108 == 0) {
                e.p = target;
                dug[target] = true;
              }
            }
          }
        }
        if (e.active && e.p == player) {
          hit();
          player = 71;
          dug[player] = true;
        }
      }
    if (!enemies.count())
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return bossRound() && enemies.count() > 0;
  }
  int bossHealth() const override {
    int hp = 0;
    for (const auto &e : enemies.items)
      if (e.active && bossRound())
        hp = std::max(hp, e.hp);
    return hp;
  }
  size_t activeEntities() const override {
    return 144 + enemies.count() + sparks.count();
  }
  void render(Canvas &c) const override {
    for (int p = 0; p < 143; ++p)
      c.rect(8 + p % 11 * 14, 35 + p / 11 * 13, 13, 12,
             dug[p] ? Black : uint16_t(0x6204 + (p / 33) * 0x1000));
    for (const auto &e : enemies.items)
      if (e.active)
        c.circle(15 + e.p % 11 * 14, 41 + e.p / 11 * 13, 4 + e.inflate / 12, e.hp > 1 ? Red : Pink,
                 true);
    c.circle(15 + player % 11 * 14, 41 + player / 11 * 13, 5, Cyan, true);
    if (input.down(Fire)) {
      int dx = direction == 1    ? 1
               : direction == -1 ? -1
                                 : 0,
          dy = direction == 11    ? 1
               : direction == -11 ? -1
                                  : 0;
      c.line(15 + player % 11 * 14, 41 + player / 11 * 13, 15 + player % 11 * 14 + dx * 42,
             41 + player / 11 * 13 + dy * 39, White);
    }
    if (rockY >= 0)
      c.circle(15 + rockX * 14, rockY, 6, Grey, true);
    c.text(8, 211, "ROCKS", Grey);
    c.number(50, 211, rock, Yellow);
    if (bossRound())
      bossBar(c, bossHealth(), 5);
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "STAGE CLEAR", Cyan);
    }
  }
};
GameEntry describeTunnelDigger() {
  return gameEntry<TunnelDigger>(
      {"TUNNEL DIGGER",
       "PUMP THE UNDERGROUND",
       {"D-PAD DIGS AND AIMS", "HOLD A TO INFLATE A FOE", "FACE FOES WITHIN 3 CELLS",
        "B DROPS ONE OF TWO ROCKS", "BOSS NEEDS FIVE INFLATIONS", "CLEAR 12 DIG SITES"},
       Orange,
       31,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyTunnelDigger() {
  TunnelDigger g;
  g.start({});
  // Regression checks for the gameplay audit.
  for (int stage = 1; stage <= 12; ++stage) {
    g.level = uint16_t(stage);
    g.setupStage();
    for (const auto &e : g.enemies.items)
      if (e.active)
        HOST_CHECK(e.p >= 0 && e.p < int(g.dug.size()));
  }
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
  g.enemies.clear();
  auto *e = g.enemies.acquire();
  e->p = g.player + 1;
  e->hp = 1;
  e->inflate = 44;
  g.direction = 1;
  g.tick = 1;
  g.stageTick = 1;
  g.input.held = Fire;
  g.play();
  HOST_CHECK(!g.enemies.count() && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
