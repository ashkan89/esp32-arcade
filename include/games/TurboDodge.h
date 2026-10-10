#pragma once
#include "arcade/Game.h"
namespace arcade {
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
} // namespace arcade
