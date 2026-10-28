#include "assets/Art.h"
#include "games/Registration.h"
namespace arcade {
class RobotWhack final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyRobotWhack();
#endif
  std::array<int, 9> life{};
  std::array<bool, 9> decoy{};
  int cursor = 4, spawn = 0, combo = 0, previousTarget = -1;
  int bossHP = 0;
  uint32_t stageStarted = 0;
  void wave() {
    life.fill(0);
    decoy.fill(false);
    bossHP = level % 4 == 0 ? 8 : 0;
    stageStarted = tick;
    spawn = 30;
  }
  void simulate() override {
    cursor = gridMove(input, cursor, 3, 3);
    if (--spawn <= 0) {
      spawn = std::max(18, 55 - int(level) * 2 - challenge() * 2);
      int p = rng.range(0, previousTarget < 0 ? 9 : 8);
      if (p >= previousTarget && previousTarget >= 0)
        ++p;
      unsigned occupied =
          unsigned(std::count_if(life.begin(), life.end(), [](int v) { return v > 0; }));
      if (life[p] == 0 && occupied < 2 &&
          (!bossHP || std::all_of(life.begin(), life.end(), [](int v) { return v == 0; }))) {
        int distance = std::abs(p % 3 - cursor % 3) + std::abs(p / 3 - cursor / 3);
        life[p] = std::max(30 + distance * 10, 95 - int(level) * 2 - challenge() * 8);
        previousTarget = p;
        decoy[p] = bossHP ? false : rng.range(0, 5) == 0;
      }
    }
    if (input.hit(Fire)) {
      if (life[cursor] && !decoy[cursor]) {
        life[cursor] = 0;
        combo = std::min(10, combo + 1);
        add(10 * combo);
        sound(Sound::Hit);
        if (bossHP)
          --bossHP;
      } else {
        if (life[cursor])
          life[cursor] = 0;
        combo = 0;
        loseLife();
      }
    }
    for (int i = 0; i < 9; i++)
      if (life[i] > 0 && --life[i] == 0 && !decoy[i]) {
        combo = 0;
        loseLife();
        if (ended)
          return;
      }
    if ((level % 4 == 0 && bossHP == 0) || (level % 4 != 0 && elapsed(tick, stageStarted) >= 600)) {
      nextStage();
      if (ended)
        return;
      if (!ended)
        wave();
    }
  }

public:
  bool bossEncounter() const override {
    return bossHP > 0;
  }
  int bossHealth() const override {
    return bossHP;
  }
  void start(const GameOptions &o) override {
    baseStart(o);
    life.fill(0);
    decoy.fill(false);
    cursor = 4;
    previousTarget = -1;
    spawn = combo = 0;
    wave();
  }
  size_t activeEntities() const override {
    return 9;
  }
  void render(Canvas &c) const override {
    art::scene(c, 20);
    for (int i = 0; i < 9; i++) {
      int x = 34 + i % 3 * 54, y = 64 + i / 3 * 53;
      c.circle(x, y, 21, Grey);
      if (life[i]) {
        art::sprite(c, decoy[i] ? art::Sprite::FriendRobot : art::Sprite::Robot, x - 14, y - 14, 28, 28);
        c.rect(x - 12, y + 15, life[i] * 24 / 95, 2, Yellow);
      }
      if (i == cursor)
        c.circle(x, y, 24, Yellow);
    }
    c.centered(205, "RED: HIT  GREEN: SPARE", White);
    if (bossHP) {
      c.text(5, 24, "BOSS", Pink);
      c.rect(35, 24, bossHP * 16, 4, Red);
    }
  }
};
GameEntry describeRobotWhack() {
  return gameEntry<RobotWhack>(
      {"ROBOT WHACK",
       "STOP THE RED ROBOTS",
       {"D-PAD SELECTS A HOLE", "A HITS THE SELECTED BOT", "SPARE THE GREEN ROBOTS",
        "MISSES COST ONE LIFE", "CHAIN HITS FOR X10", "4/8/12: EIGHT-HIT BOSS"},
       Red,
       20,
       Fire});
}

#ifdef ARCADE_HOST_TESTS
bool verifyRobotWhack() {
  RobotWhack g;
  g.start({});
  // Regression checks for the gameplay audit.
  g.level = 12;
  g.options.difficulty = 3;
  for (int p = 0; p < 9; ++p) {
    g.life.fill(0);
    g.cursor = p;
    g.spawn = 1;
    g.simulate();
    for (int q = 0; q < 9; ++q)
      if (g.life[q])
        HOST_CHECK(g.life[q] >= 29 + 10 * (std::abs(q % 3 - p % 3) + std::abs(q / 3 - p / 3)));
  }
  g.start({});
  g.level = 12;
  g.wave();
  g.bossHP = 1;
  g.life[g.cursor] = 40;
  g.decoy[g.cursor] = false;
  g.input.pressed = Fire;
  g.simulate();
  HOST_CHECK(g.campaignComplete() && g.gameOver() && g.stage() == 12);
  auto score = g.score();
  for (int i = 0; i < 100; ++i)
    g.update();
  HOST_CHECK(g.score() == score);
  g.start({});
  HOST_CHECK(!g.campaignComplete() && !g.gameOver() && g.stage() == 1 && g.score() == 0);
  return true;
}
#endif
} // namespace arcade
