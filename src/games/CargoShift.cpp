#include "assets/Art.h"
#include "games/Registration.h"
namespace arcade {
class CargoShift final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyCargoShift();
  std::array<int8_t, 800> solutionWalk{};
  int solutionSteps = 0;
#endif
  std::array<uint8_t, 80> board{};
  std::array<bool, 80> goal{};
  int player = 0, moves = 0;
  struct Snapshot {
    std::array<uint8_t, 80> board;
    int player, moves;
  };
  std::array<Snapshot, 32> history{};
  int count = 0;
  void generatedPuzzle() {
    rng.state = derivedSeed(options.seed, uint32_t(level));
    // Start solved, then reverse legal pushes. The reverse walk is a constructive solution.
    board.fill(0);
    goal.fill(false);
    for (int y = 0; y < 10; ++y)
      for (int x = 0; x < 8; ++x)
        if (x == 0 || x == 7 || y == 0 || y == 9)
          board[y * 8 + x] = 1;
    for (int p : {26, 29, 43}) {
      goal[p] = true;
      board[p] = 2;
    }
    player = 36;
#ifdef ARCADE_HOST_TESTS
    solutionSteps = 0;
#endif
    int pulls = 0;
    for (int step = 0; step < 800; ++step) {
      int d = std::array<int, 4>{{-1, 1, -8, 8}}[rng.range(0, 4)], next = player + d;
      if (next < 0 || next >= 80 || board[next])
        continue;
      int behind = player - d;
      if (behind >= 0 && behind < 80 && board[behind] == 2 && rng.range(0, 3) != 0) {
        board[player] = 2;
        board[behind] = 0;
        ++pulls;
      }
      player = next;
#ifdef ARCADE_HOST_TESTS
      solutionWalk[solutionSteps++] = int8_t(-d);
#endif
      if (pulls >= 8 + int(level) * 2 + challenge() * 3)
        break;
    }
    bool solved = true;
    for (int p = 0; p < 80; ++p)
      if (goal[p] && board[p] != 2)
        solved = false;
    if (solved) {
      board[26] = 0;
      board[34] = 2;
      player = 42;
#ifdef ARCADE_HOST_TESTS
      solutionWalk[0] = -8;
      solutionSteps = 1;
#endif
    }
    moves = count = 0;
  }
  void puzzle() {
    if (level >= 4) {
      generatedPuzzle();
      return;
    }
    // Three hand-authored opening puzzles, followed by nine reverse-generated warehouses.
    static const char *maps[] = {"########"
                                 "#      #"
                                 "# . .  #"
                                 "# $ $  #"
                                 "#  @   #"
                                 "#      #"
                                 "#      #"
                                 "#      #"
                                 "#      #"
                                 "########",
                                 "########"
                                 "#      #"
                                 "#  .   #"
                                 "#  $   #"
                                 "#.$@$. #"
                                 "#      #"
                                 "#      #"
                                 "#      #"
                                 "#      #"
                                 "########",
                                 "########"
                                 "#      #"
                                 "# .  . #"
                                 "# $  $ #"
                                 "#  ##  #"
                                 "#  @   #"
                                 "# . $  #"
                                 "#      #"
                                 "#      #"
                                 "########"};
    const char *map = maps[(level - 1) % 3];
    bool mirror = ((level - 1) / 3) % 2;
    board.fill(0);
    goal.fill(false);
    for (int y = 0; y < 10; y++)
      for (int x = 0; x < 8; x++) {
        int p = y * 8 + x, source = y * 8 + (mirror ? 7 - x : x);
        char ch = map[source];
        board[p] = ch == '#' ? 1 : ch == '$' ? 2 : 0;
        goal[p] = ch == '.';
        if (ch == '@')
          player = p;
      }
    moves = count = 0;
  }
  void simulate() override {
    if (input.hit(Fire)) {
      puzzle();
      return;
    }
    if (input.hit(Special) && count) {
      const auto &s = history[--count];
      board = s.board;
      player = s.player;
      moves = s.moves;
      return;
    }
    int target = gridMove(input, player, 8, 10);
    if (target == player || board[target] == 1)
      return;
    int beyond = target + (target - player);
    if (board[target] == 2 && (beyond < 0 || beyond >= 80 || board[beyond]))
      return;
    if (count == 32 - challenge() * 4) {
      for (int i = 1; i < 32 - challenge() * 4; i++)
        history[i - 1] = history[i];
      count = 31 - challenge() * 4;
    }
    history[count++] = {board, player, moves};
    if (board[target] == 2) {
      board[beyond] = 2;
      board[target] = 0;
    }
    player = target;
    moves++;
    sound(Sound::Move);
    bool solved = true;
    for (int p = 0; p < 80; p++)
      if (goal[p] && board[p] != 2)
        solved = false;
    if (solved) {
      add(uint32_t(std::max(100, 1000 - moves * 10)));
      nextStage();
      if (ended)
        return;
      puzzle();
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    health = 1;
    puzzle();
  }
  size_t activeEntities() const override {
    return 81;
  }
  void render(Canvas &c) const override {
    art::scene(c, 23);
    for (int p = 0; p < 80; p++) {
      int x = 16 + p % 8 * 18, y = 25 + p / 8 * 18;
      if (board[p] == 1)
        art::sprite(c, art::Sprite::Wall, x, y, 17, 17);
      else {
        if (goal[p])
          c.circle(x + 8, y + 8, 5, Green);
        if (board[p] == 2) {
          art::sprite(c, art::Sprite::Crate, x, y, 17, 17);
          if (goal[p]) c.rect(x, y, 17, 17, Green, false);
        }
      }
    }
    art::object(c, art::Sprite::Hero, 24 + player % 8 * 18, 33 + player / 8 * 18, 15, 17);
    c.text(8, 211, "A RESET  B UNDO", White);
  }
};
GameEntry describeCargoShift() {
  return gameEntry<CargoShift>(
      {"CARGO SHIFT",
       "PUSH CRATES ONTO TARGETS",
       {"D-PAD MOVES THE WORKER", "PUSH ONE CRATE AT A TIME", "GREEN CIRCLES ARE TARGETS",
        "B UNDOES RECENT MOVES", "A RESETS THIS PUZZLE", "FEWER MOVES SCORE HIGHER"},
       Orange,
       23,
       Special | Fire});
}
} // namespace arcade

#ifdef ARCADE_HOST_TESTS
namespace arcade {
bool verifyCargoShift() {
  CargoShift g;
  for (unsigned difficulty = 0; difficulty < DifficultyCount; ++difficulty)
    for (unsigned seed = 1; seed <= 30; ++seed)
      for (int stage = 4; stage <= 12; ++stage) {
        GameOptions o;
        o.seed = seed;
        o.difficulty = uint8_t(difficulty);
        g.start(o);
        g.level = uint16_t(stage);
        g.puzzle();
        auto original = g.board;
        int originalPlayer = g.player;
        auto witness = g.solutionWalk;
        int steps = g.solutionSteps;
        g.puzzle();
        HOST_CHECK(g.board == original && g.player == originalPlayer);
        for (int i = steps - 1; i >= 0 && !g.gameOver() && g.stage() == stage; --i) {
          int d = witness[i];
          uint16_t b = d == -1 ? Left : d == 1 ? Right : d == -8 ? Up : Down;
          g.handle({b, b});
          g.update();
        }
        HOST_CHECK(g.campaignComplete() || g.stage() == stage + 1);
      }
  return true;
}
} // namespace arcade
#endif
