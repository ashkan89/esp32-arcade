#pragma once
#include "arcade/Game.h"
namespace arcade {
class SnakeRush final : public Game {
  friend struct RuleTests;
  struct Cell {
    int8_t x = 0, y = 0;
    bool operator==(Cell b) const {
      return x == b.x && y == b.y;
    }
  };
  static constexpr int Cols = 20, Rows = 22, Capacity = Cols * Rows;
  std::array<Cell, Capacity> body{};
  uint16_t length = 4;
  Cell food;
  int direction = 0, nextDir = 0, moveTicks = 0;
  bool turnQueued = false;
  bool wall(Cell c) const;
  bool occupied(Cell c, int count) const;
  bool placeFood();
  void step();

public:
  void start(const GameOptions &) override;
  void handle(const InputFrame &) override;
  void update() override;
  void render(Canvas &) const override;
  void cleanup() override {
    Game::cleanup();
    length = 0;
  }
  size_t activeEntities() const override {
    return length + (!ended);
  }
};
} // namespace arcade
