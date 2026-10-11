#include "games/PuzzleRules.h"
#include "games/Registration.h"
namespace arcade {
class MineGrid final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyMineGrid();
#endif
  std::array<uint8_t, 81> mines{}, open{}, flag{};
  int cursor = 40, remaining = 0;
  bool planted = false;
  void board() {
    mines.fill(0);
    open.fill(0);
    flag.fill(0);
    planted = false;
    remaining = 0;
    cursor = 40;
  }
  void plant(int first) {
    int count = 10 + challenge() * 3 + std::min(8, int(level));
    int placed = 0;
    while (placed < count) {
      int p = rng.range(0, 81);
      if (!mines[p] && (std::abs(p % 9 - first % 9) > 1 || std::abs(p / 9 - first / 9) > 1)) {
        mines[p] = 1;
        placed++;
      }
    }
    remaining = 81 - count;
    planted = true;
  }
  void reveal(int p) {
    if (open[p] || flag[p])
      return;
    if (mines[p]) {
      open[p] = 1;
      loseLife();
      return;
    }
    std::array<int, 81> queue{};
    int head = 0, tail = 0;
    queue[tail++] = p;
    open[p] = 1;
    remaining--;
    add(5);
    while (head < tail) {
      int cell = queue[head++];
      if (adjacentMines(mines, cell))
        continue;
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
          int x = cell % 9 + dx, y = cell / 9 + dy;
          if (x < 0 || x >= 9 || y < 0 || y >= 9)
            continue;
          int q = y * 9 + x;
          if (!mines[q] && !open[q] && !flag[q]) {
            open[q] = 1;
            remaining--;
            add(5);
            queue[tail++] = q;
          }
        }
    }
  }
  void simulate() override {
    cursor = gridMove(input, cursor, 9, 9);
    if (input.hit(Special) && !open[cursor])
      flag[cursor] ^= 1;
    if (input.hit(Fire) && !flag[cursor]) {
      if (!planted)
        plant(cursor);
      reveal(cursor);
      if (remaining == 0 && !ended) {
        add(500);
        nextStage();
        if (ended)
          return;
        board();
      }
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    health = 1;
    board();
  }
  size_t activeEntities() const override {
    return 81;
  }
  void render(Canvas &c) const override {
    for (int p = 0; p < 81; p++) {
      int x = 7 + (p % 9) * 18, y = 34 + (p / 9) * 18;
      c.rect(x, y, 16, 16, open[p] ? Grey : Navy);
      if (open[p]) {
        if (mines[p])
          c.circle(x + 8, y + 8, 5, Red, true);
        else if (int n = adjacentMines(mines, p))
          c.number(x + 5, y + 4, n, Black);
      } else if (flag[p]) {
        c.line(x + 5, y + 3, x + 5, y + 13, White);
        c.rect(x + 6, y + 3, 7, 5, Red);
      }
      if (p == cursor)
        c.rect(x - 1, y - 1, 18, 18, Yellow, false);
    }
    c.centered(204, "A OPEN  B FLAG", White);
  }
};
GameEntry describeMineGrid() {
  return gameEntry<MineGrid>(
      {"MINE GRID",
       "UNCOVER THE SAFE CELLS",
       {"D-PAD MOVES THE CURSOR", "A REVEALS - B FLAGS", "FIRST OPEN IS SAFE",
        "NUMBERS COUNT NEIGHBORS", "OPEN ALL SAFE CELLS", "ONE MINE ENDS THE RUN"},
       Yellow,
       10,
       Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyMineGrid() {
  MineGrid g;
  int previous = 0;
  for (uint8_t mode = 0; mode < DifficultyCount; ++mode) {
    GameOptions o;
    o.difficulty = mode;
    g.start(o);
    g.plant(40);
    int count = 0;
    for (int p = 0; p < 81; ++p) {
      count += g.mines[p];
      if (std::abs(p % 9 - 4) <= 1 && std::abs(p / 9 - 4) <= 1)
        HOST_CHECK(!g.mines[p]);
    }
    HOST_CHECK(count > previous && g.remaining == 81 - count);
    previous = count;
  }
  g.start({});
  g.level = 12;
  g.planted = true;
  g.mines.fill(0);
  g.open.fill(1);
  g.cursor = 0;
  g.open[0] = 0;
  g.remaining = 1;
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
