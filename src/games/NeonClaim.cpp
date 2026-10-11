#include "games/Campaign.h"
namespace arcade {
class NeonClaim final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyNeonClaim();
#endif
  std::array<uint8_t, 320> cells{};
  int player = 0, anchor = 0, claimed = 0;
  bool drawing = false;
  float ex = 8, ey = 9, vx = .08f, vy = .06f;
  int timer = 0;
  void setupStage() override {
    cells.fill(0);
    for (int y = 0; y < 20; ++y)
      for (int x = 0; x < 16; ++x)
        if (!x || !y || x == 15 || y == 19)
          cells[y * 16 + x] = 1;
    player = anchor = 0;
    claimed = 0;
    drawing = false;
    ex = 8;
    ey = 9;
    vx = hazardSpeed(.06f + level * .003f);
    vy = hazardSpeed(.05f + level * .003f);
    timer = hazardInterval(60 * 120);
  }
  void abandon() {
    for (auto &v : cells)
      if (v == 2)
        v = 0;
    player = anchor;
    drawing = false;
    hit();
  }
  void fill() {
    std::array<bool, 320> reachable{};
    std::array<int, 320> queue{};
    int head = 0, tail = 0, origin = clamp(int(ey), 1, 18) * 16 + clamp(int(ex), 1, 14);
    if (cells[origin] != 0) {
      abandon();
      return;
    }
    queue[tail++] = origin;
    reachable[origin] = true;
    while (head < tail) {
      int p = queue[head++];
      const int ds[] = {-1, 1, -16, 16};
      for (int d : ds) {
        int q = p + d;
        if (q >= 0 && q < 320 && std::abs(q % 16 - p % 16) + std::abs(q / 16 - p / 16) == 1 &&
            !cells[q] && !reachable[q]) {
          reachable[q] = true;
          queue[tail++] = q;
        }
      }
    }
    int gain = 0;
    for (int p = 0; p < 320; ++p)
      if (cells[p] == 2 || (!cells[p] && !reachable[p])) {
        cells[p] = 1;
        ++gain;
      }
    add(gain * 10);
    drawing = false;
    anchor = player;
    claimed = 0;
    for (int y = 1; y < 19; ++y)
      for (int x = 1; x < 15; ++x)
        claimed += cells[y * 16 + x] == 1;
    sound(Sound::Power);
    if (claimed * 100 >= 252 * 70)
      clearStage();
  }
  void play() override {
    if (--timer <= 0) {
      timer = hazardInterval(60 * 120);
      abandon();
      return;
    }
    if (input.hit(Special) && drawing) {
      for (auto &v : cells)
        if (v == 2)
          v = 0;
      player = anchor;
      drawing = false;
    }
    if (tick % 4 == 0) {
      float nx = ex + vx, ny = ey + vy;
      int q = clamp(int(ny), 0, 19) * 16 + clamp(int(nx), 0, 15);
      if (nx < 1 || nx >= 15 || cells[clamp(int(ey), 0, 19) * 16 + clamp(int(nx), 0, 15)] == 1)
        vx = -vx;
      else
        ex = nx;
      if (ny < 1 || ny >= 19 || cells[q] == 1)
        vy = -vy;
      else
        ey = ny;
      int p = clamp(int(ey), 0, 19) * 16 + clamp(int(ex), 0, 15);
      if (drawing && cells[p] == 2)
        abandon();
    }
    int target = gridMove(input, player, 16, 20);
    if (target == player)
      return;
    if (cells[target] == 2) {
      abandon();
      return;
    }
    if (!cells[target]) {
      if (!drawing && !input.down(Fire))
        return;
      if (!drawing) {
        anchor = player;
        drawing = true;
      }
      cells[target] = 2;
      player = target;
    } else {
      player = target;
      if (drawing)
        fill();
      else
        anchor = player;
    }
  }

public:
  size_t activeEntities() const override {
    return 322;
  }
  void render(Canvas &c) const override {
    for (int p = 0; p < 320; ++p)
      c.rect(8 + p % 16 * 10, 31 + p / 16 * 9, 9, 8,
             cells[p] == 1   ? uint16_t(0x19ad + level * 32)
             : cells[p] == 2 ? Yellow
                             : Black);
    c.circle(13 + int(ex * 10), 35 + int(ey * 9), 4, Pink, true);
    c.rect(10 + player % 16 * 10, 33 + player / 16 * 9, 5, 4, White);
    c.text(5, 211, "CLAIM", Grey);
    c.number(42, 211, claimed * 100 / 252, Green);
    c.text(66, 211, "%", Green);
    c.number(135, 211, timer / 60, Yellow);
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "STAGE CLEAR", Cyan);
    }
  }
};
GameEntry describeNeonClaim() {
  return gameEntry<NeonClaim>(
      {"NEON CLAIM",
       "CAPTURE SEVENTY PERCENT",
       {"D-PAD MOVES ALONG BORDERS", "HOLD A TO START A NEW LINE", "JOIN BORDER TO CLAIM AREA",
        "PINK SPARK BREAKS YOUR LINE", "B CANCELS AN OPEN LINE", "CLAIM 70% ON 12 BOARDS"},
       Cyan,
       29,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyNeonClaim() {
  NeonClaim g;
  g.start({});
  for (int stage = 1; stage <= 12; ++stage) {
    HOST_CHECK(g.stage() == stage && !g.gameOver());
    HOST_CHECK(g.activeEntities() < 500);
    g.clearStage();
    g.pause();
    for (int i = 0; i < 100; ++i)
      g.update();
    HOST_CHECK(g.transition == 90);
    g.resume();
    for (int i = 0; i < 90; ++i)
      g.update();
  }
  HOST_CHECK(g.campaignComplete() && g.stage() == 12);
  auto finalScore = g.score();
  for (int i = 0; i < 100; ++i)
    g.update();
  HOST_CHECK(g.score() == finalScore);
  g.start({});
  HOST_CHECK(!g.gameOver() && !g.campaignComplete() && g.score() == 0);
  g.level = 4;
  g.setupStage();
  g.stageTick = 1;

  g.level = 1;
  g.setupStage();
  for (int y = 1; y < 19; ++y)
    g.cells[y * 16 + 12] = 2;
  g.drawing = true;
  g.player = 316;
  g.ex = 13;
  g.ey = 8;
  g.fill();
  HOST_CHECK(g.claimed == 216 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
