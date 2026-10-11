#include "games/Campaign.h"
namespace arcade {
class CrystalCourier final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyCrystalCourier();
#endif
  std::array<uint8_t, 81> tiles{};
  std::array<int, 3> hunters{};
  int player = 40, remaining = 0, power = 0, hp = 0, maximum = 0;
  static int xx(int p) {
    return 88 + (p % 9 - p / 9) * 9;
  }
  static int yy(int p) {
    return 46 + (p % 9 + p / 9) * 8;
  }
  void setupStage() override {
    tiles.fill(1);
    tiles[40] = 0;
    player = 40;
    remaining = 80;
    hunters = {{0, 8, 80}};
    power = 0;
    hp = maximum = bossRound() ? 3 : 0;
  }
  void play() override {
    if (power)
      --power;
    if (input.hit(Fire) && !power) {
      power = hazardInterval(120);
      sound(Sound::Power);
    }
    int target = gridMove(input, player, 9, 9);
    if (target != player) {
      player = target;
      if (tiles[player]) {
        tiles[player] = 0;
        --remaining;
        add(20);
        sound(Sound::Move);
      }
    }
    if (stageTick % unsigned(std::max(10, 35 - int(level) - challenge() * 2)) == 0)
      for (int i = 0; i < 3; ++i) {
        int p = hunters[i], dx = player % 9 - p % 9, dy = player / 9 - p / 9;
        int next = p + (std::abs(dx) > std::abs(dy) ? (dx > 0 ? 1 : -1) : (dy > 0 ? 9 : -9));
        if (next >= 0 && next < 81 && std::abs(next % 9 - p % 9) + std::abs(next / 9 - p / 9) == 1)
          hunters[i] = next;
      }
    for (int i = 0; i < 3; ++i)
      if (hunters[i] == player) {
        if (power) {
          add(100);
          hunters[i] = i == 0 ? 0 : i == 1 ? 8 : 80;
          if (i == 0 && hp) {
            --hp;
            power = 0;
          }
        } else {
          hit();
          player = 40;
          hunters = {{0, 8, 80}};
          break;
        }
      }
    if (!remaining && !hp)
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
    return 85;
  }
  void render(Canvas &c) const override {
    for (int p = 0; p < 81; ++p) {
      int x = xx(p), y = yy(p);
      uint16_t color = (p % 9 + p / 9) % 2 ? Navy : 0x2188;
      c.line(x - 8, y, x, y - 7, color);
      c.line(x, y - 7, x + 8, y, color);
      c.line(x + 8, y, x, y + 7, color);
      c.line(x, y + 7, x - 8, y, color);
      if (tiles[p]) {
        c.line(x, y - 3, x + 3, y, Cyan);
        c.line(x + 3, y, x, y + 3, Cyan);
        c.line(x, y + 3, x - 3, y, Cyan);
        c.line(x - 3, y, x, y - 3, Cyan);
      }
    }
    for (int i = 0; i < 3; ++i)
      c.circle(xx(hunters[i]), yy(hunters[i]) - 5, i == 0 && hp ? 6 : 4, i == 0 && hp ? Red : Pink,
               true);
    c.circle(xx(player), yy(player) - 5, 5, power ? Yellow : White, true);
    if (hp)
      bossBar(c, hp, maximum);
    c.centered(207, power ? "CRYSTAL ARMOR ACTIVE" : "A: TEMPORARY ARMOR", Grey);
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "DELIVERY COMPLETE", Green);
    }
  }
};
GameEntry describeCrystalCourier() {
  return gameEntry<CrystalCourier>(
      {"CRYSTAL COURIER",
       "COLLECT THE CRYSTAL COURT",
       {"D-PAD MOVES DIAGONAL TILES", "COLLECT ALL EIGHTY CRYSTALS", "A ACTIVATES 2S OF ARMOR",
        "ARMOR TAGS THE HUNTERS", "BOSS: TAG RED GOLEM 3 TIMES", "DELIVER ON 12 COURTS"},
       Cyan,
       46,
       Left | Right | Up | Down | Fire});
}

#ifdef ARCADE_HOST_TESTS
bool verifyCrystalCourier() {
  CrystalCourier g;
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
  g.remaining = 0;
  g.hp = 1;
  g.power = 100;
  g.hunters = {{g.player, 8, 80}};
  g.stageTick = 1;
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
