#include "games/PuzzleRules.h"
#include "games/Registration.h"
namespace arcade {
class GridMerge final : public TickGame {
  std::array<uint8_t, 16> cells{};
  bool won = false;
  void spawn() {
    std::array<int, 16> empty{};
    int n = 0;
    for (int i = 0; i < 16; i++)
      if (!cells[i])
        empty[n++] = i;
    if (n)
      cells[empty[rng.range(0, n)]] = rng.range(0, 10) ? 1 : 2;
  }
  bool possible() const {
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++) {
        int p = y * 4 + x;
        if (!cells[p] || (x < 3 && cells[p] == cells[p + 1]) || (y < 3 && cells[p] == cells[p + 4]))
          return true;
      }
    return false;
  }
  void simulate() override {
    int direction = -1;
    if (input.hit(Left))
      direction = 0;
    else if (input.hit(Right))
      direction = 1;
    else if (input.hit(Up))
      direction = 2;
    else if (input.hit(Down))
      direction = 3;
    if (direction < 0)
      return;
    auto before = cells;
    for (int row = 0; row < 4; row++) {
      std::array<uint8_t, 4> line{};
      std::array<int, 4> positions{};
      for (int i = 0; i < 4; i++) {
        int p = direction < 2 ? row * 4 + (direction == 0 ? i : 3 - i)
                              : (direction == 2 ? i : 3 - i) * 4 + row;
        positions[i] = p;
        line[i] = cells[p];
      }
      add(mergeLine(line));
      for (int i = 0; i < 4; i++)
        cells[positions[i]] = line[i];
    }
    if (before != cells) {
      spawn();
      sound(Sound::Move);
    }
    for (auto v : cells)
      if (v >= 11 && !won) {
        won = true;
        add(2048);
        sound(Sound::Wave);
      }
    if (!possible()) {
      ended = true;
      health = 0;
      sound(Sound::Over);
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    cells.fill(0);
    won = false;
    health = 1;
    spawn();
    spawn();
  }
  size_t activeEntities() const override {
    return 16;
  }
  void render(Canvas &c) const override {
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++) {
        int p = y * 4 + x, xx = 6 + x * 42, yy = 42 + y * 39;
        uint8_t v = cells[p];
        c.rect(xx, yy, 39, 36, v ? uint16_t(0x2104 + v * 0x0840) : Navy);
        if (v)
          c.number(xx + 3, yy + 14, uint32_t(1) << v, White);
      }
    c.centered(205, won ? "2048! KEEP MERGING" : "D-PAD: SLIDE ALL TILES", won ? Yellow : Grey);
  }
};
GameEntry describeGridMerge() {
  return gameEntry<GridMerge>(
      {"GRID MERGE",
       "BUILD THE 2048 TILE",
       {"D-PAD SLIDES THE BOARD", "EQUAL TILES MERGE ONCE", "VALID MOVES ADD A TILE",
        "SCORE THE MERGED VALUES", "2048 GIVES A BONUS", "NO MOVES ENDS THE RUN"},
       Orange,
       18,
       Left | Right | Up | Down});
}
} // namespace arcade
