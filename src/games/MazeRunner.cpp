#include "games/Registration.h"
namespace arcade {
class MazeRunner final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyMazeRunner();
#endif
  std::array<uint8_t, 17 * 19> maze{}, gems{};
  std::array<int, 3> enemies{};
  int player = 18, left = 0, grace = 0;
  void board() {
    maze.fill(1);
    gems.fill(0);
    std::array<int, 323> stack{};
    int n = 1;
    stack[0] = 18;
    maze[18] = 0;
    while (n) {
      int p = stack[n - 1];
      std::array<int, 4> next{};
      int count = 0;
      for (int d : {-2, 2, -34, 34}) {
        int q = p + d;
        if (q % 17 > 0 && q % 17 < 16 && q / 17 > 0 && q / 17 < 18 &&
            std::abs(q % 17 - p % 17) + std::abs(q / 17 - p / 17) == 2 && maze[q])
          next[count++] = q;
      }
      if (!count) {
        n--;
        continue;
      }
      int q = next[rng.range(0, count)];
      maze[q] = maze[(p + q) / 2] = 0;
      stack[n++] = q;
    }
    left = 0;
    for (int i = 0; i < 323; i++)
      if (!maze[i] && i != 18) {
        gems[i] = 1;
        left++;
      }
    player = 18;
    enemies = {304, 300, 270};
    for (auto &e : enemies)
      while (maze[e])
        e--;
    grace = 120;
  }
  void simulate() override {
    if (grace > 0)
      grace--;
    int target = gridMove(input, player, 17, 19);
    if (!maze[target])
      player = target;
    if (gems[player]) {
      gems[player] = 0;
      left--;
      add(10);
      sound(Sound::Move);
    }
    if (tick % uint32_t(std::max(8, 25 - int(level) - challenge() * 3)) == 0) {
      // BFS distances give pursuers a route through the generated corridors.
      std::array<int, 323> distance{}, queue{};
      distance.fill(-1);
      int head = 0, tail = 1;
      queue[0] = player;
      distance[player] = 0;
      while (head < tail) {
        int p = queue[head++];
        for (int d : {-1, 1, -17, 17}) {
          int q = p + d;
          if (q < 0 || q >= 323 || std::abs(q % 17 - p % 17) + std::abs(q / 17 - p / 17) != 1 ||
              maze[q] || distance[q] >= 0)
            continue;
          distance[q] = distance[p] + 1;
          queue[tail++] = q;
        }
      }
      for (auto &e : enemies) {
        int best = e;
        for (int d : {-1, 1, -17, 17}) {
          int q = e + d;
          if (q >= 0 && q < 323 && !maze[q] && distance[q] >= 0 && distance[q] < distance[best])
            best = q;
        }
        e = best;
      }
    }
    for (auto e : enemies)
      if (e == player && !grace) {
        loseLife();
        player = 18;
        grace = 180;
        break;
      }
    if (left == 0 && !ended) {
      add(500);
      nextStage();
      if (ended)
        return;
      board();
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    board();
  }
  size_t activeEntities() const override {
    return size_t(left + 4);
  }
  void render(Canvas &c) const override {
    for (int y = 0; y < 19; y++)
      for (int x = 0; x < 17; x++) {
        int p = y * 17 + x;
        if (maze[p])
          c.rect(3 + x * 10, 27 + y * 9, 9, 8, 0x2114);
        else if (gems[p])
          c.pixel(7 + x * 10, 31 + y * 9, Yellow);
      }
    for (auto e : enemies)
      c.circle(7 + e % 17 * 10, 31 + e / 17 * 9, 3, Red, true);
    if (!grace || tick % 10 < 5)
      c.circle(7 + player % 17 * 10, 31 + player / 17 * 9, 3, Green, true);
    c.centered(208, "COLLECT EVERY GOLD DOT", White);
  }
};
GameEntry describeMazeRunner() {
  return gameEntry<MazeRunner>(
      {"MAZE RUNNER",
       "OUTRUN THE RED HUNTERS",
       {"D-PAD MOVES IN CORRIDORS", "COLLECT EVERY GOLD DOT", "HUNTERS FOLLOW THE MAZE",
        "NEW MAZE AFTER EACH CLEAR", "RESPAWN GIVES BRIEF GRACE", "LIVES DEPEND ON MODE"},
       Green,
       11,
       Left | Right | Up | Down});
}

#ifdef ARCADE_HOST_TESTS
bool verifyMazeRunner() {
  MazeRunner g;
  g.start({});
  g.level = 12;
  g.gems.fill(0);
  g.gems[g.player] = 1;
  g.left = 1;
  g.tick = 1;
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
