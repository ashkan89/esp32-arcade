#pragma once
#include "arcade/Game.h"
namespace arcade {
enum class ShooterMode : uint8_t { Sly, Sky, Star, Meteor };
class Shooter : public Game {
  friend struct RuleTests;
  struct Shot {
    bool active = false;
    float x = 0, y = 0, oldX = 0, oldY = 0, vx = 0, vy = 0;
    uint8_t damage = 1;
  };
  struct Enemy {
    bool active = false;
    float x = 0, y = 0, homeX = 0, homeY = 0, vx = 0, vy = 0;
    int hp = 1;
    uint8_t kind = 0, size = 0;
    uint32_t age = 0;
  };
  struct Particle {
    bool active = false;
    float x = 0, y = 0, vx = 0, vy = 0;
    int life = 0;
    uint16_t color = Orange;
  };
  struct Power {
    bool active = false;
    float x = 0, y = 0;
    uint8_t kind = 0;
  };
  ShooterMode mode;
  Pool<Shot, 32> shots;
  Pool<Shot, 48> hostile;
  Pool<Enemy, 32> enemies;
  Pool<Particle, 48> particles;
  Pool<Power, 4> powers;
  std::array<std::array<uint8_t, 20 * 12>, 3> barriers{};
  float px = 80, py = 198;
  int autoDir = 1, weapon = 0, invincible = 0, shield = 0, rapid = 0, dual = 0, combo = 1;
  int cooldown = 0, shotTimer = 0, spawnTimer = 0, moveTimer = 0, dir = 1, clearTimer = 0,
      spawned = 0, bombs = 1;
  uint32_t lastKill = 0;
  bool bossStage = false, damaged = false;
  uint16_t shake = 0;
  bool ufoActive = false, beam = false;
  float ufoX = -30;
  int ufoTimer = 0;
  void wave();
  void fire();
  void enemyFire(float x, float y, float vx, float vy);
  void explode(float x, float y, uint16_t color);
  void hurt();
  void killed(Enemy &e);
  void power(uint8_t kind);
  bool barrier(float x, float y, int damage);
  void spawnSky();
  void spawnRock(float x, float y, uint8_t size, float vx = 0);
  void updateEnemies();
  void collisions();

public:
  explicit Shooter(ShooterMode m) : mode(m) {}
  void start(const GameOptions &) override;
  void update() override;
  void render(Canvas &) const override;
  void cleanup() override;
  size_t activeEntities() const override {
    return shots.count() + hostile.count() + enemies.count() + particles.count() + powers.count();
  }
};
class SlyInvaders final : public Shooter {
public:
  SlyInvaders() : Shooter(ShooterMode::Sly) {}
};
class SkyFighters final : public Shooter {
public:
  SkyFighters() : Shooter(ShooterMode::Sky) {}
};
class StarAssault final : public Shooter {
public:
  StarAssault() : Shooter(ShooterMode::Star) {}
};
class MeteorPatrol final : public Shooter {
public:
  MeteorPatrol() : Shooter(ShooterMode::Meteor) {}
};
} // namespace arcade
