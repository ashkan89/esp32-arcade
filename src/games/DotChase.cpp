#include "arcade/Behavior.h"
#include "games/Campaign.h"
namespace arcade {
class DotChase final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyDotChase();
#endif
  std::array<uint8_t, 221> cells{};
  std::array<int, 4> ghosts{};
  int player = 14, remaining = 0, power = 0, movement = 0;
  int hp = 0, maximum = 0;
  int observedPlayer = 14;
  bool wall(int p) const {
    return p < 0 || p >= 221 || cells[p] == 1;
  }
  bool adjacent(int a, int b) const {
    return std::abs(a % 13 - b % 13) + std::abs(a / 13 - b / 13) == 1;
  }
  void setupStage() override {
    remaining = 0;
    power = movement = 0;
    player = 14;
    observedPlayer = player;
    for (int y = 0; y < 17; ++y)
      for (int x = 0; x < 13; ++x) {
        int p = y * 13 + x;
        bool blocked = !x || !y || x == 12 || y == 16 || (x % 2 == 0 && y % 2 == 0);
        cells[p] = blocked ? 1 : 2;
        remaining += !blocked;
      }
    for (int p : {14, 24, 196, 206})
      cells[p] = 3;
    // Mirrored wall clusters vary the court without disconnecting its corridors.
    for (int i = 0; i < int(level) % 4; ++i) {
      int p = (3 + i * 2) * 13 + ((level % 2) ? 4 : 8);
      if (cells[p] >= 2) {
        cells[p] = 1;
        --remaining;
      }
    }
    ghosts = {{109, 111, 135, 137}};
    hp = maximum = bossRound() ? 3 : 0;
  }
  void play() override {
    if (power)
      --power;
    if (++movement >= 7) {
      movement = 0;
      auto movementInput = input;
      movementInput.repeated |= input.held;
      int p = gridMove(movementInput, player, 13, 17);
      if (!wall(p) && adjacent(p, player))
        player = p;
      if (cells[player] >= 2) {
        if (cells[player] == 3) {
          power = 300;
          sound(Sound::Power);
        } else
          sound(Sound::Move);
        cells[player] = 0;
        --remaining;
        add(10);
      }
    }
    if (stageTick % unsigned(std::max(8, 20 - int(level) / 2 - challenge() * 2)) == 0) {
      for (int i = 0; i < 4; ++i) {
        int target = player;
        if (!power && i == 1) {
          int projected = player + (player - observedPlayer);
          if (!wall(projected) && adjacent(projected, player))
            target = projected;
        }
        if (!power && i == 2 &&
            std::abs(ghosts[i] % 13 - player % 13) + std::abs(ghosts[i] / 13 - player / 13) < 5)
          target = 206;
        if (!power && i == 3 && (stageTick / 240) % 2 == 0)
          target = 24;
        int best = routeStep<221>(ghosts[i], target, 13, [&](int p) { return wall(p); }, power);
        const int ds[] = {-1, 1, -13, 13};
        if (i == 3 && rng.range(0, 3) == 0) {
          int p = ghosts[i] + ds[rng.range(0, 4)];
          if (!wall(p) && adjacent(p, ghosts[i]))
            best = p;
        }
        ghosts[i] = best;
      }
      observedPlayer = player;
    }
    for (int i = 0; i < 4; ++i)
      if (ghosts[i] == player) {
        if (power) {
          add(200);
          ghosts[i] = 109 + (i % 2) * 2 + (i / 2) * 26;
          if (i == 0 && hp > 0) {
            --hp;
            power = 0;
            burst(float(10 + player % 13 * 13), float(29 + player / 13 * 10));
          }
        } else {
          hit();
          player = 14;
          ghosts = {{109, 111, 135, 137}};
          break;
        }
      }
    // Boss stages refresh the four energizers until the king is defeated.
    if (hp && !power && stageTick % 360 == 0)
      for (int p : {14, 24, 196, 206})
        if (cells[p] == 0) {
          cells[p] = 3;
          ++remaining;
        }
    if (remaining == 0 && hp == 0)
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return hp > 0;
  }
  int bossHealth() const override {
    return hp;
  }
  size_t activeEntities() const override {
    return 226;
  }
  void render(Canvas &c) const override {
    for (int p = 0; p < 221; ++p) {
      int x = 3 + p % 13 * 13, y = 35 + p / 13 * 10;
      if (cells[p] == 1)
        c.rect(x, y, 12, 9, Navy);
      else if (cells[p] >= 2)
        c.circle(x + 6, y + 4, cells[p] == 3 ? 3 : 1, Yellow, true);
    }
    c.circle(9 + player % 13 * 13, 39 + player / 13 * 10, 4, Yellow, true);
    for (int i = 0; i < 4; ++i) {
      int x = 9 + ghosts[i] % 13 * 13, y = 39 + ghosts[i] / 13 * 10;
      c.circle(x, y, 4, power ? Cyan : i == 0 && hp ? Red : Pink, true);
      c.pixel(x - 2, y - 1, White);
      c.pixel(x + 2, y - 1, White);
    }
    if (hp)
      bossBar(c, hp, maximum);
    c.centered(211, power ? "ENERGIZED: TAG THE GHOSTS" : "CLEAR DOTS / 12 MAZES",
               power ? Cyan : Grey);
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "STAGE CLEAR", Yellow);
    }
  }
};
GameEntry describeDotChase() {
  return gameEntry<DotChase>(
      {"DOT CHASE",
       "DOTS GHOSTS AND POWER",
       {"D-PAD MOVES THROUGH MAZE", "CLEAR EVERY GOLD DOT", "LARGE DOTS POWER YOU UP",
        "TAG BLUE GHOSTS FOR BONUS", "BOSS: TAG KING THREE TIMES", "CLEAR 12 MAZES TO WIN"},
       Yellow,
       30,
       Left | Right | Up | Down});
}

#ifdef ARCADE_HOST_TESTS
bool verifyDotChase() {
  DotChase g;
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
  HOST_CHECK(g.bossHealth() > 0);
  g.hp = 1;
  g.power = 100;
  g.remaining = 0;
  g.cells[g.player] = 0;
  g.ghosts = {{g.player, 111, 135, 137}};
  g.stageTick = 1;
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
