#include "games/Registration.h"
namespace arcade {
class MemoryMatch final : public TickGame {
  std::array<uint8_t, 20> cards{}, matched{};
  int cursor = 0, first = -1, second = -1, wait = 0, pairs = 0;
  uint32_t started = 0;
  void board() {
    for (int i = 0; i < 20; i++)
      cards[i] = uint8_t(i / 2);
    for (int i = 19; i > 0; i--)
      std::swap(cards[i], cards[rng.range(0, i + 1)]);
    matched.fill(0);
    first = second = -1;
    wait = pairs = 0;
    started = tick;
  }
  void simulate() override {
    cursor = gridMove(input, cursor, 4, 5);
    if (wait) {
      if (--wait == 0) {
        if (cards[first] == cards[second]) {
          matched[first] = matched[second] = 1;
          pairs++;
          add(100);
          sound(Sound::Power);
        } else
          sound(Sound::Hit);
        first = second = -1;
        if (pairs == 10) {
          add(uint32_t(std::max(0, 500 - int((tick - started) / 60))));
          nextStage();
          board();
        }
      }
      return;
    }
    if (input.hit(Fire) && !matched[cursor] && cursor != first) {
      if (first < 0)
        first = cursor;
      else {
        second = cursor;
        wait = cards[first] == cards[second] ? 20 : 60;
      }
    }
    if (tick - started >
        uint32_t(std::max(30, 150 - int(level) * 5 - options.difficulty * 10)) * 60) {
      loseLife();
      if (!ended)
        board();
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    cursor = 0;
    board();
  }
  size_t activeEntities() const override {
    return 20;
  }
  void render(Canvas &c) const override {
    for (int i = 0; i < 20; i++) {
      int x = 15 + i % 4 * 38, y = 33 + i / 4 * 33;
      bool visible = matched[i] || i == first || i == second;
      c.rect(x, y, 32, 28, visible ? uint16_t(Cyan + (cards[i] * 0x1000)) : Navy);
      if (visible)
        c.number(x + 13, y + 10, cards[i] + 1, Black);
      else
        c.text(x + 13, y + 10, "?", Grey);
      if (i == cursor)
        c.rect(x - 2, y - 2, 36, 32, Yellow, false);
    }
    c.centered(206, "A: FLIP A CARD", White);
  }
};
GameEntry describeMemoryMatch() {
  return gameEntry<MemoryMatch>(
      {"MEMORY MATCH",
       "FIND THE TEN PAIRS",
       {"D-PAD SELECTS A CARD", "A FLIPS THE CARD", "MATCH TWO EQUAL SYMBOLS",
        "WRONG PAIRS HIDE AGAIN", "CLEAR BEFORE TIME RUNS", "FASTER CLEARS GIVE BONUS"},
       Cyan,
       19,
       Fire});
}
} // namespace arcade
