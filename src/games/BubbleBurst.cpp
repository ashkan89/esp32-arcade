#include "assets/Art.h"
#include "games/Registration.h"
namespace arcade {
class BubbleBurst final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyBubbleBurst();
#endif
  std::array<uint8_t, 108> grid{};
  float bx = 88, by = 200, vx = 0, vy = 0, aim = -1.57f;
  uint8_t color = 1;
  int misses = 0;
  bool flying = false;
  void board() {
    grid.fill(0);
    for (int p = 0; p < 36; p++)
      grid[p] = uint8_t(rng.range(1, 5));
    flying = false;
    misses = 0;
    color = uint8_t(rng.range(1, 5));
  }
  void settle() {
    int row = clamp(int((by - 34) / 14 + 0.5f), 0, 11),
        col = clamp(int((bx - 18) / 17 + 0.5f), 0, 8), p = row * 9 + col;
    if (grid[p]) {
      int best = -1;
      float distance = 1e9f;
      for (int q = 0; q < 108; q++)
        if (!grid[q]) {
          int r = q / 9, c = q % 9;
          float d = std::pow(18 + c * 17 - bx, 2) + std::pow(34 + r * 14 - by, 2);
          if (d < distance) {
            distance = d;
            best = q;
          }
        }
      if (best < 0) {
        ended = true;
        health = 0;
        return;
      }
      p = best;
    }
    grid[p] = color;
    std::array<int, 108> queue{};
    std::array<bool, 108> seen{};
    int head = 0, tail = 1;
    queue[0] = p;
    seen[p] = true;
    while (head < tail) {
      int q = queue[head++];
      for (int d : {-1, 1, -9, 9}) {
        int n = q + d;
        if (n < 0 || n >= 108 || std::abs(n % 9 - q % 9) + std::abs(n / 9 - q / 9) != 1 ||
            seen[n] || grid[n] != color)
          continue;
        seen[n] = true;
        queue[tail++] = n;
      }
    }
    if (tail >= 3) {
      for (int i = 0; i < tail; i++)
        grid[queue[i]] = 0;
      add(uint32_t(tail * 20));
      misses = 0;
      sound(Sound::Power);
    } else
      misses++;
    // Drop disconnected clusters after any pop.
    seen.fill(false);
    head = tail = 0;
    for (int q = 0; q < 9; q++)
      if (grid[q]) {
        queue[tail++] = q;
        seen[q] = true;
      }
    while (head < tail) {
      int q = queue[head++];
      for (int d : {-1, 1, -9, 9}) {
        int n = q + d;
        if (n < 0 || n >= 108 || std::abs(n % 9 - q % 9) + std::abs(n / 9 - q / 9) != 1 ||
            seen[n] || !grid[n])
          continue;
        seen[n] = true;
        queue[tail++] = n;
      }
    }
    for (int q = 0; q < 108; q++)
      if (grid[q] && !seen[q]) {
        grid[q] = 0;
        add(30);
      }
    if (misses >= (challenge() >= 5 ? 2 : challenge() >= 3 ? 3 : 4)) {
      for (int q = 99; q < 108; q++)
        if (grid[q]) {
          ended = true;
          health = 0;
        }
      for (int q = 107; q >= 9; q--)
        grid[q] = grid[q - 9];
      for (int q = 0; q < 9; q++)
        grid[q] = uint8_t(rng.range(1, 5));
      misses = 0;
    }
    bool any = false;
    for (int q = 0; q < 108; q++) {
      any |= grid[q] != 0;
      if (q >= 99 && grid[q]) {
        ended = true;
        health = 0;
      }
    }
    if (!any) {
      add(500);
      nextStage();
      if (ended)
        return;
      board();
    }
    flying = false;
    color = uint8_t(rng.range(1, 5));
  }
  void simulate() override {
    if (!flying) {
      aim = clamp(aim + (int(input.down(Right)) - int(input.down(Left))) * 0.025f, -2.9f, -0.24f);
      if (input.hit(Fire)) {
        bx = 88;
        by = 200;
        vx = std::cos(aim) * 3;
        vy = std::sin(aim) * 3;
        flying = true;
        sound(Sound::Shot);
      }
      return;
    }
    bx += vx;
    by += vy;
    if (bx < 10 || bx > 166) {
      bx = clamp(bx, 10.0f, 166.0f);
      vx = -vx;
    }
    bool hit = by <= 34;
    for (int p = 0; p < 108 && !hit; p++)
      if (grid[p])
        hit = std::pow(bx - (18 + p % 9 * 17), 2) + std::pow(by - (34 + p / 9 * 14), 2) < 190;
    if (hit)
      settle();
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    health = 1;
    aim = -1.57f;
    board();
  }
  size_t activeEntities() const override {
    return 109;
  }
  void render(Canvas &c) const override {
    art::scene(c, 17);
    static constexpr uint16_t colors[] = {Black, Red, Green, Cyan, Yellow};
    for (int p = 0; p < 108; p++)
      if (grid[p])
        art::tinted(c, art::Sprite::Bubble, 11 + p % 9 * 17, 27 + p / 9 * 14, 15, 15, colors[grid[p]]);
    c.line(88, 200, 88 + int(std::cos(aim) * 26), 200 + int(std::sin(aim) * 26), White);
    art::tinted(c, art::Sprite::Bubble, (flying ? int(bx) : 88) - 6, (flying ? int(by) : 200) - 6, 13, 13, colors[color]);
    c.text(5, 211, "MISSES", Grey);
    c.number(47, 211, misses, Yellow);
  }
};
GameEntry describeBubbleBurst() {
  return gameEntry<BubbleBurst>(
      {"BUBBLE BURST",
       "POP THE COLOR CLUSTERS",
       {"L/R AIMS - A FIRES", "THREE MATCHING COLORS POP", "WALLS REFLECT THE SHOT",
        "LOOSE CLUSTERS FALL AWAY", "2-4 MISSES ADD A ROW", "CLEAR ALL BUBBLES TO LEVEL"},
       Pink,
       17,
       Fire | Left | Right});
}

#ifdef ARCADE_HOST_TESTS
bool verifyBubbleBurst() {
  BubbleBurst g;
  g.start({});
  g.level = 12;
  g.grid.fill(0);
  g.grid[0] = g.grid[1] = 1;
  g.bx = 52;
  g.by = 34;
  g.color = 1;
  g.settle();
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
