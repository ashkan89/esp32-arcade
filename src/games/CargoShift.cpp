#include "games/Registration.h"
namespace arcade {
class CargoShift final : public TickGame {
  std::array<uint8_t, 80> board{};
  std::array<bool, 80> goal{};
  int player = 0, moves = 0;
  struct Snapshot {
    std::array<uint8_t, 80> board;
    int player, moves;
  };
  std::array<Snapshot, 32> history{};
  int count = 0;
  void puzzle() {
    // Hand-authored solvable boards. Rotate/mirror boards to vary subsequent cycles.
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
    if (count == 32) {
      for (int i = 1; i < 32; i++)
        history[i - 1] = history[i];
      count = 31;
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
    for (int p = 0; p < 80; p++) {
      int x = 16 + p % 8 * 18, y = 25 + p / 8 * 18;
      if (board[p] == 1)
        c.rect(x, y, 17, 17, Grey);
      else {
        if (goal[p])
          c.circle(x + 8, y + 8, 5, Green);
        if (board[p] == 2) {
          c.rect(x + 1, y + 1, 15, 15, goal[p] ? Green : Orange);
          c.line(x + 3, y + 3, x + 13, y + 13, Black);
        }
      }
    }
    c.circle(24 + player % 8 * 18, 33 + player / 8 * 18, 5, Cyan, true);
    c.text(8, 211, "A RESET  B UNDO", White);
  }
};
GameEntry describeCargoShift() {
  return gameEntry<CargoShift>(
      {"CARGO SHIFT",
       "PUSH CRATES ONTO TARGETS",
       {"D-PAD MOVES THE WORKER", "PUSH ONE CRATE AT A TIME", "GREEN CIRCLES ARE TARGETS",
        "B UNDOES LAST 32 MOVES", "A RESETS THIS PUZZLE", "FEWER MOVES SCORE HIGHER"},
       Orange,
       23,
       Special | Fire});
}
} // namespace arcade
