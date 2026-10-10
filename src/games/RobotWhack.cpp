#include "games/Registration.h"
namespace arcade {
class RobotWhack final : public TickGame {
  std::array<int, 9> life{};
  std::array<bool, 9> decoy{};
  int cursor = 4, spawn = 0, combo = 0;
  void simulate() override {
    cursor = gridMove(input, cursor, 3, 3);
    if (--spawn <= 0) {
      spawn = std::max(18, 55 - int(level) * 2);
      int p = rng.range(0, 9);
      if (life[p] == 0) {
        life[p] = std::max(35, 95 - int(level) * 2 - options.difficulty * 8);
        decoy[p] = rng.range(0, 5) == 0;
      }
    }
    if (input.hit(Fire)) {
      if (life[cursor] && !decoy[cursor]) {
        life[cursor] = 0;
        combo = std::min(10, combo + 1);
        add(10 * combo);
        sound(Sound::Hit);
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
    if (tick % 600 == 0)
      nextStage();
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    life.fill(0);
    decoy.fill(false);
    cursor = 4;
    spawn = combo = 0;
  }
  size_t activeEntities() const override {
    return 9;
  }
  void render(Canvas &c) const override {
    for (int i = 0; i < 9; i++) {
      int x = 34 + i % 3 * 54, y = 64 + i / 3 * 53;
      c.circle(x, y, 21, Grey);
      if (life[i]) {
        c.rect(x - 12, y - 12, 24, 22, decoy[i] ? Green : Red);
        c.rect(x - 7, y - 5, 4, 4, Black);
        c.rect(x + 3, y - 5, 4, 4, Black);
        c.rect(x - 12, y + 15, life[i] * 24 / 95, 2, Yellow);
      }
      if (i == cursor)
        c.circle(x, y, 24, Yellow);
    }
    c.centered(205, "RED: HIT  GREEN: SPARE", White);
  }
};
GameEntry describeRobotWhack() {
  return gameEntry<RobotWhack>(
      {"ROBOT WHACK",
       "STOP THE RED ROBOTS",
       {"D-PAD SELECTS A HOLE", "A HITS THE SELECTED BOT", "SPARE THE GREEN ROBOTS",
        "MISSES COST ONE LIFE", "CHAIN HITS FOR X10", "BOTS GET FASTER EACH WAVE"},
       Red,
       20,
       Fire});
}
} // namespace arcade
