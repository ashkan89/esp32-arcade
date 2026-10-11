#include "assets/Visuals.h"
#include "games/Registration.h"
namespace arcade {
class BlockCascade final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyBlockCascade();
#endif
  std::array<uint8_t, 200> cells{};
  int px = 3, py = 0, rotation = 0, kind = 0, next = 0, fall = 0, lines = 0;
  uint16_t shape(int k, int r) const {
    static const uint16_t pieces[7] = {0x00f0, 0x0066, 0x0072, 0x0036, 0x0063, 0x0071, 0x0074};
    uint16_t mask = pieces[k];
    // I uses a 4x4 pivot; other pieces rotate around a 3x3 pivot.
    if (k == 1)
      return mask;
    for (int turn = 0; turn < r; turn++) {
      uint16_t result = 0;
      int width = k == 0 ? 4 : 3;
      for (int y = 0; y < width; y++)
        for (int x = 0; x < width; x++)
          if (mask & (1u << (y * 4 + x)))
            result |= uint16_t(1u << (x * 4 + width - 1 - y));
      mask = result;
    }
    return mask;
  }
  bool fits(int x, int y, int r) const {
    uint16_t mask = shape(kind, r);
    for (int j = 0; j < 4; j++)
      for (int i = 0; i < 4; i++)
        if (mask & (1u << (j * 4 + i))) {
          int xx = x + i, yy = y + j;
          if (xx < 0 || xx >= 10 || yy >= 20 || yy < 0 || cells[yy * 10 + xx])
            return false;
        }
    return true;
  }
  void spawn() {
    kind = next;
    next = rng.range(0, 7);
    px = 3;
    py = rotation = fall = 0;
    if (!fits(px, py, rotation)) {
      ended = true;
      health = 0;
      sound(Sound::Over);
    }
  }
  void lock() {
    uint16_t mask = shape(kind, rotation);
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++)
        if (mask & (1u << (y * 4 + x)))
          cells[(py + y) * 10 + px + x] = uint8_t(kind + 1);
    int cleared = 0;
    for (int y = 19; y >= 0; y--) {
      bool full = true;
      for (int x = 0; x < 10; x++)
        full &= cells[y * 10 + x] != 0;
      if (full) {
        for (int row = y; row > 0; row--)
          for (int x = 0; x < 10; x++)
            cells[row * 10 + x] = cells[(row - 1) * 10 + x];
        for (int x = 0; x < 10; x++)
          cells[x] = 0;
        cleared++;
        y++;
      }
    }
    if (cleared) {
      add(uint32_t(cleared * cleared * 100) * level);
      lines += cleared;
      progressTo(uint16_t(std::min(13, 1 + lines / 10)));
      sound(Sound::Power);
    }
    spawn();
  }
  void simulate() override {
    int dx = int(input.nav(Right)) - int(input.nav(Left));
    if (dx && fits(px + dx, py, rotation))
      px += dx;
    if (input.hit(Fire)) {
      int r = (rotation + 1) % 4;
      for (int kick : {0, -1, 1, -2, 2})
        if (fits(px + kick, py, r)) {
          px += kick;
          rotation = r;
          break;
        }
    }
    if (input.hit(Special)) {
      while (fits(px, py + 1, rotation)) {
        py++;
        add(2);
      }
      lock();
      return;
    }
    if (++fall >= std::max(3, 38 - int(level) * 2 - options.difficulty * 4) || input.nav(Down)) {
      fall = 0;
      if (fits(px, py + 1, rotation))
        py++;
      else
        lock();
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    cells.fill(0);
    health = 1;
    lines = 0;
    next = rng.range(0, 7);
    spawn();
  }
  size_t activeEntities() const override {
    return 204;
  }
  void render(Canvas &c) const override {
    static const uint16_t colors[] = {Black, Cyan, Yellow, Pink, Green, Red, Orange, White};
    c.rect(7, 27, 102, 182, Grey, false);
    for (int y = 0; y < 20; y++)
      for (int x = 0; x < 10; x++)
        if (cells[y * 10 + x])
          visual::tile(c, 8 + x * 10, 28 + y * 9, 9, 8, colors[cells[y * 10 + x]]);
    int ghost = py;
    while (fits(px, ghost + 1, rotation))
      ghost++;
    uint16_t mask = shape(kind, rotation);
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++)
        if (mask & (1u << (y * 4 + x))) {
          c.rect(8 + (px + x) * 10, 28 + (ghost + y) * 9, 9, 8, Grey, false);
          visual::tile(c, 8 + (px + x) * 10, 28 + (py + y) * 9, 9, 8, colors[kind + 1]);
        }
    c.text(118, 40, "NEXT", Cyan);
    mask = shape(next, 0);
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++)
        if (mask & (1u << (y * 4 + x)))
          visual::tile(c, 118 + x * 10, 60 + y * 9, 9, 8, colors[next + 1]);
    c.text(116, 132, "LINES", Grey);
    c.number(120, 146, lines, White);
  }
};
GameEntry describeBlockCascade() {
  return gameEntry<BlockCascade>(
      {"BLOCK CASCADE",
       "CLEAR THE FALLING WALL",
       {"L/R MOVE - DOWN DROPS", "A ROTATES WITH WALL KICK", "B HARD DROPS THE PIECE",
        "GHOST SHOWS LANDING", "CLEAR ROWS FOR COMBOS", "TOPPING OUT ENDS RUN"},
       Pink,
       9,
       Left | Right | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyBlockCascade() {
  BlockCascade g;
  g.start({});
  g.level = 12;
  g.lines = 119;
  for (int x = 0; x < 10; ++x)
    g.cells[190 + x] = 1;
  g.lock();
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
