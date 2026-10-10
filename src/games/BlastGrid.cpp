#include "games/Registration.h"
namespace arcade {
class BlastGrid final : public TickGame {
  std::array<uint8_t, 99> wall{}, flame{};
  std::array<int, 3> bots{};
  int player = 12, bomb = -1, fuse = 0, grace = 0;
  bool blocked(int p) const {
    return p < 0 || p >= 99 || wall[p] != 0;
  }
  void board() {
    for (int y = 0; y < 11; y++)
      for (int x = 0; x < 9; x++) {
        int p = y * 9 + x;
        wall[p] = (x == 0 || x == 8 || y == 0 || y == 10 || (x % 2 == 0 && y % 2 == 0))
                      ? 2
                      : uint8_t(rng.range(0, 4) == 0);
      }
    player = 10;
    for (int p : {10, 11, 19, 70, 71, 79, 80, 16, 25})
      wall[p] = 0;
    bots = {16, 70, 80};
    flame.fill(0);
    bomb = -1;
    fuse = grace = 0;
  }
  void simulate() override {
    if (grace)
      grace--;
    for (auto &v : flame)
      if (v)
        v--;
    int target = gridMove(input, player, 9, 11);
    if (!blocked(target) && target != bomb)
      player = target;
    if (input.hit(Fire) && bomb < 0) {
      bomb = player;
      fuse = 100;
      sound(Sound::Shot);
    }
    if (bomb >= 0 && --fuse == 0) {
      flame[bomb] = 30;
      for (int d : {-1, 1, -9, 9})
        for (int i = 1; i <= 2; i++) {
          int p = bomb + d * i;
          if (p < 0 || p >= 99 || wall[p] == 2)
            break;
          flame[p] = 30;
          if (wall[p] == 1) {
            wall[p] = 0;
            add(10);
            break;
          }
        }
      bomb = -1;
      sound(Sound::Explosion);
    }
    if (tick % uint32_t(std::max(10, 28 - int(level) - options.difficulty * 4)) == 0)
      for (auto &bot : bots)
        if (bot >= 0) {
          int d = std::array<int, 4>{{-1, 1, -9, 9}}[rng.range(0, 4)], q = bot + d;
          if (!blocked(q) && q != bomb)
            bot = q;
        }
    bool danger = flame[player] != 0;
    int left = 0;
    for (auto &bot : bots)
      if (bot >= 0) {
        if (flame[bot]) {
          bot = -1;
          add(100);
        } else {
          left++;
          danger |= bot == player;
        }
      }
    if (danger && !grace) {
      loseLife();
      player = 10;
      grace = 100;
    }
    if (!left && !ended) {
      add(300);
      nextStage();
      board();
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    board();
  }
  size_t activeEntities() const override {
    return 104;
  }
  void render(Canvas &c) const override {
    for (int p = 0; p < 99; p++) {
      int x = 7 + p % 9 * 18, y = 23 + p / 9 * 17;
      if (wall[p])
        c.rect(x, y, 16, 15, wall[p] == 2 ? Grey : Orange);
      if (flame[p])
        c.rect(x, y, 16, 15, Yellow);
    }
    if (bomb >= 0)
      c.circle(15 + bomb % 9 * 18, 30 + bomb / 9 * 17, 5, tick % 12 < 6 ? Red : White, true);
    for (auto bot : bots)
      if (bot >= 0)
        c.circle(15 + bot % 9 * 18, 30 + bot / 9 * 17, 5, Red, true);
    if (!grace || tick % 10 < 5)
      c.circle(15 + player % 9 * 18, 30 + player / 9 * 17, 5, Cyan, true);
    c.centered(211, "A BOMB - ESCAPE THE BLAST", White);
  }
};
GameEntry describeBlastGrid() {
  return gameEntry<BlastGrid>(
      {"BLAST GRID",
       "OUTWIT THE ROAMING BOTS",
       {"D-PAD MOVES ON THE GRID", "A PLANTS ONE TIMED BOMB", "BLAST REACHES TWO CELLS",
        "STONE BLOCKS STOP FIRE", "CRATES BREAK FOR POINTS", "CLEAR THREE BOTS TO LEVEL"},
       Orange,
       15,
       Fire});
}
} // namespace arcade
