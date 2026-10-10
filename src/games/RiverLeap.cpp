#include "games/Registration.h"
namespace arcade {
class RiverLeap final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyRiverLeap();
#endif
  struct Mover {
    bool active = false;
    float x = 0;
    int row = 0, width = 0;
  };
  Pool<Mover, 24> movers;
  float x = 88;
  int row = 0, grace = 0;
  std::array<bool, 4> homes{};
  float speed(int r) const {
    return (r % 2 ? 1 : -1) * (0.45f + std::min(0.7f, level * 0.04f) + options.difficulty * 0.12f);
  }
  void board() {
    movers.clear();
    for (int r = 1; r <= 7; r++)
      for (int i = 0; i < 3; i++)
        if (auto *m = movers.acquire()) {
          m->x = float(i * 78 + (r % 2) * 24);
          m->row = r;
          m->width = r >= 5 ? 54 : 26;
        }
    homes.fill(false);
    row = 0;
    x = 88;
    grace = 0;
  }
  void crash() {
    if (grace)
      return;
    loseLife();
    row = 0;
    x = 88;
    grace = 50;
  }
  void simulate() override {
    if (grace)
      grace--;
    if (input.nav(Left))
      x -= 17;
    if (input.nav(Right))
      x += 17;
    if (input.nav(Up))
      row = std::min(8, row + 1);
    if (input.nav(Down))
      row = std::max(0, row - 1);
    x = clamp(x, 6.0f, float(W - 6));
    bool carried = false;
    for (auto &m : movers.items)
      if (m.active) {
        m.x += speed(m.row);
        if (m.x > W + 60)
          m.x = -60;
        if (m.x < -60)
          m.x = W + 60;
        if (m.row == row) {
          if (row <= 3 && overlaps({x - 5, float(198 - row * 20), 10, 10},
                                   {m.x, float(196 - row * 20), float(m.width), 14}))
            crash();
          if (row >= 5 && x >= m.x && x <= m.x + m.width) {
            carried = true;
            x += speed(row);
          }
        }
      }
    if (row >= 5 && row <= 7 && (!carried || x < 5 || x > W - 5))
      crash();
    if (row == 8) {
      int home = clamp(int(x) / 44, 0, 3);
      if (homes[home])
        crash();
      else {
        homes[home] = true;
        add(100);
        row = 0;
        x = 88;
        sound(Sound::Power);
        bool full = true;
        for (bool v : homes)
          full &= v;
        if (full) {
          add(400);
          nextStage();
          if (ended)
            return;
          board();
        }
      }
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    board();
  }
  size_t activeEntities() const override {
    return movers.count() + 1;
  }
  void cleanup() override {
    Game::cleanup();
    movers.clear();
  }
  void render(Canvas &c) const override {
    c.rect(0, 49, W, 60, 0x092b);
    c.rect(0, 135, W, 63, 0x18c3);
    for (int i = 0; i < 4; i++)
      c.rect(i * 44 + 9, 28, 26, 13, homes[i] ? Yellow : Green);
    for (const auto &m : movers.items)
      if (m.active)
        c.rect(int(m.x), 196 - m.row * 20, m.width, 14, m.row >= 5 ? Orange : Red);
    c.circle(int(x), 203 - row * 20, 5, Green, true);
    c.centered(212, "UP: HOP TOWARD HOMES", White);
  }
};
GameEntry describeRiverLeap() {
  return gameEntry<RiverLeap>(
      {"RIVER LEAP",
       "REACH ALL FOUR HOMES",
       {"D-PAD HOPS BETWEEN ROWS", "DODGE CARS ON THE ROAD", "RIDE LOGS OVER WATER",
        "BANK IS SAFE TO REST", "USE EACH HOME ONLY ONCE", "FILL FOUR HOMES TO LEVEL"},
       Green,
       12,
       Up | Down | Left | Right});
}

#ifdef ARCADE_HOST_TESTS
bool verifyRiverLeap() {
  RiverLeap g;
  g.start({});
  g.level = 12;
  g.homes.fill(true);
  g.homes[3] = false;
  g.row = 8;
  g.x = 150;
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
