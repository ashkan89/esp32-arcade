#include "assets/Visuals.h"
#include "games/Registration.h"
namespace arcade {
class MemoryMatch final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyMemoryMatch();
#endif
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
          if (ended)
            return;
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
      visual::card(c, x, y, 32, 28, visible ? Cyan : visual::Edge, i == cursor);
      if (visible) {
        c.number(x + 13, y + 10, cards[i] + 1, matched[i] ? Green : White);
        c.line(x + 5, y + 23, x + 26, y + 23, matched[i] ? Green : Cyan);
      } else
        visual::alien(c, x + 10, y + 9, visual::Muted, 0);
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

#ifdef ARCADE_HOST_TESTS
bool verifyMemoryMatch() {
  MemoryMatch g;
  g.start({});
  g.level = 12;
  g.cards[0] = g.cards[1] = 1;
  g.first = 0;
  g.second = 1;
  g.pairs = 9;
  g.wait = 1;
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
