#pragma once
#include "games/Campaign.h"
namespace arcade {
class NeonBreaker final : public Game {
  friend struct RuleTests;
  std::array<uint8_t, 48> bricks{};
  float paddle = 70, ballX = 88, ballY = 185, vx = 1.5f, vy = -2.5f;
  bool launched = false;
  int wide = 0, slow = 0;
  float dropX = 0, dropY = 0;
  int dropKind = -1;
  Pool<Projectile, 12> hazards;
  int bossHP = 0, bossMaximum = 0, bossGrace = 0;
  float bossX = 88;
  void layout();
  void lose();

public:
  bool bossEncounter() const override {
    return bossHP > 0;
  }
  int bossHealth() const override {
    return bossHP;
  }
  void start(const GameOptions &) override;
  void update() override;
  void render(Canvas &) const override;
  void cleanup() override {
    Game::cleanup();
    bricks.fill(0);
    dropKind = -1;
    hazards.clear();
    bossHP = 0;
  }
  size_t activeEntities() const override {
    size_t n = 1;
    for (auto b : bricks)
      n += b != 0;
    return n + (dropKind >= 0) + hazards.count();
  }
};
} // namespace arcade
