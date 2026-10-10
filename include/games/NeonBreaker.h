#pragma once
#include "arcade/Game.h"
namespace arcade {
class NeonBreaker final : public Game {
  friend struct RuleTests;
  std::array<uint8_t, 48> bricks{};
  float paddle = 70, ballX = 88, ballY = 185, vx = 1.5f, vy = -2.5f;
  bool launched = false;
  int wide = 0, slow = 0;
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
} // namespace arcade
