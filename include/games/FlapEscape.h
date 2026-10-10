#pragma once
#include "arcade/Game.h"
namespace arcade {
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
