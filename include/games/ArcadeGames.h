#pragma once
#include "arcade/Game.h"
namespace arcade {
class NeonBreaker final : public Game {
  friend struct RuleTests;
  std::array<uint8_t, 48> bricks{};
  float paddle = 70, ballX = 88, ballY = 185, vx = 1.5f, vy = -2.5f;
  bool launched = false;
  int wide = 0, slow = 0, paddleDir = 1;
  float dropX = 0, dropY = 0;
  int dropKind = -1;
  void layout();
  void lose();

public:
  void start(const GameOptions &) override;
  void update() override;
  void render(Canvas &) const override;
  void cleanup() override {
    Game::cleanup();
    bricks.fill(0);
    dropKind = -1;
  }
  size_t activeEntities() const override {
    size_t n = 1;
    for (auto b : bricks)
      n += b != 0;
    return n + (dropKind >= 0);
  }
};
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
class TurboDodge final : public Game {
  friend struct RuleTests;
  struct Obstacle {
    bool active = false;
    float y = 0;
    uint8_t lane = 0;
    bool coin = false;
  };
  Pool<Obstacle, 16> obstacles;
  int lane = 1, spawn = 0, invincible = 0, coins = 0;
  float carX = 81;
  void spawnRow();

public:
  void start(const GameOptions &) override;
  void update() override;
  void render(Canvas &) const override;
  void cleanup() override {
    Game::cleanup();
    obstacles.clear();
  }
  size_t activeEntities() const override {
    return obstacles.count() + 1;
  }
};
class FlapEscape final : public Game {
  friend struct RuleTests;
  struct Gate {
    bool active = false;
    float x = 0;
    int gapY = 0;
    bool passed = false;
  };
  Pool<Gate, 4> gates;
  float y = 110, vy = 0;
  int spawn = 0;
  bool flying = false;

public:
  void start(const GameOptions &) override;
  void update() override;
  void render(Canvas &) const override;
  void cleanup() override {
    Game::cleanup();
    gates.clear();
  }
  size_t activeEntities() const override {
    return gates.count() + 1;
  }
};
} // namespace arcade
