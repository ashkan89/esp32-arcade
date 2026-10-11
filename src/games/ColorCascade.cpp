#include "games/PuzzleRules.h"
#include "games/Registration.h"
namespace arcade {
class ColorCascade final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyColorCascade();
#endif
  std::array<uint8_t, 64> cells{};
  int cursor = 0, chosen = -1, moves = 30, combo = 0, wait = 0;
  uint64_t clearing = 0;
  void fill() {
    for (int attempt = 0; attempt < 8; ++attempt) {
      for (int p = 0; p < 64; ++p) {
        std::array<uint8_t, 5> candidates{};
        int n = 0;
        for (uint8_t v = 1; v <= 5; ++v)
          if (!(p % 8 >= 2 && cells[p - 1] == v && cells[p - 2] == v) &&
              !(p >= 16 && cells[p - 8] == v && cells[p - 16] == v))
            candidates[n++] = v;
        cells[p] = candidates[rng.range(0, n)];
      }
      if (legalMove())
        return;
    }
    for (int p = 0; p < 64; ++p)
      cells[p] = uint8_t((p % 8 + 2 * (p / 8)) % 5 + 1);
    cells[0] = cells[2] = cells[9] = 1;
    cells[1] = 2; // Swap 1 and 9 creates a legal row.
  }
  bool legalMove() const {
    for (int p = 0; p < 64; p++)
      for (int d : {1, 8}) {
        int q = p + d;
        if (q >= 64 || (d == 1 && p % 8 == 7))
          continue;
        auto test = cells;
        std::swap(test[p], test[q]);
        if (matchRuns(test))
          return true;
      }
    return false;
  }
  void simulate() override {
    if (wait) {
      if (--wait)
        return;
      for (int x = 0; x < 8; x++) {
        int target = 7;
        for (int y = 7; y >= 0; y--)
          if (!(clearing & (uint64_t(1) << (y * 8 + x))))
            cells[target-- * 8 + x] = cells[y * 8 + x];
        while (target >= 0)
          cells[target-- * 8 + x] = uint8_t(rng.range(1, 6));
      }
      clearing = matchRuns(cells);
      if (clearing) {
        combo++;
        scoreMatch();
        return;
      }
      combo = 0;
      if (points >= uint32_t(level) * 1500) {
        nextStage();
        if (ended)
          return;
        moves += 15 - challenge();
      }
      if (moves == 0) {
        ended = true;
        health = 0;
        sound(Sound::Over);
      } else if (!legalMove())
        fill();
      return;
    }
    cursor = gridMove(input, cursor, 8, 8);
    if (input.hit(Special))
      chosen = -1;
    if (input.hit(Fire)) {
      if (chosen < 0) {
        chosen = cursor;
        return;
      }
      if (std::abs(chosen % 8 - cursor % 8) + std::abs(chosen / 8 - cursor / 8) == 1) {
        std::swap(cells[chosen], cells[cursor]);
        clearing = matchRuns(cells);
        if (clearing) {
          moves--;
          combo = 1;
          scoreMatch();
        } else
          std::swap(cells[chosen], cells[cursor]);
      }
      chosen = -1;
    }
  }
  void scoreMatch() {
    for (int p = 0; p < 64; p++)
      if (clearing & (uint64_t(1) << p))
        add(30 * uint32_t(combo));
    wait = 22;
    sound(Sound::Power);
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    fill();
    cursor = 0;
    chosen = -1;
    moves = 30 - challenge() * 2;
    combo = wait = 0;
    clearing = 0;
    health = 1;
  }
  size_t activeEntities() const override {
    return 64;
  }
  void render(Canvas &c) const override {
    static const uint16_t colors[] = {Black, Red, Green, Cyan, Yellow, Pink};
    for (int p = 0; p < 64; p++) {
      int x = 9 + p % 8 * 20, y = 32 + p / 8 * 20;
      uint16_t color = colors[cells[p]];
      if (wait && (clearing & (uint64_t(1) << p)) && tick % 8 < 4)
        color = White;
      c.rect(x + 2, y + 2, 14, 14, color);
      if (p == chosen)
        c.rect(x, y, 18, 18, White, false);
      if (p == cursor)
        c.rect(x - 1, y - 1, 20, 20, Yellow, false);
    }
    c.text(8, 205, "MOVES", Grey);
    c.number(47, 205, moves, White);
    c.text(80, 205, "A SWAP", White);
  }
};
GameEntry describeColorCascade() {
  return gameEntry<ColorCascade>(
      {"COLOR CASCADE",
       "CHAIN THE COLOR MATCHES",
       {"D-PAD SELECTS A GEM", "A SELECTS THEN SWAPS", "SWAP WITH A NEIGHBOR",
        "MATCH THREE OR MORE", "CASCADES MULTIPLY POINTS", "1500/STAGE ADDS MORE MOVES"},
       Pink,
       24,
       Fire});
}

#ifdef ARCADE_HOST_TESTS
bool verifyColorCascade() {
  ColorCascade g;
  for (unsigned seed = 1; seed <= 100; ++seed) {
    GameOptions o;
    o.seed = seed;
    g.start(o);
    HOST_CHECK(!matchRuns(g.cells) && g.legalMove());
  }
  int previous = 100;
  for (uint8_t mode = 0; mode < DifficultyCount; ++mode) {
    GameOptions o;
    o.difficulty = mode;
    g.start(o);
    HOST_CHECK(g.moves > 0 && g.moves < previous);
    previous = g.moves;
  }
  g.start({});
  g.level = 12;
  g.fill();
  g.points = 18000;
  g.wait = 1;
  g.clearing = 0;
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
