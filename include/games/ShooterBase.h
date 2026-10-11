#pragma once
#include "arcade/Behavior.h"
#include "arcade/Game.h"
namespace arcade {
enum class ShooterMode : uint8_t { Invaders, Sky, Star, Meteor };
class Shooter : public Game {
  friend struct RuleTests;

protected:
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
    int cue = 0, actionTimer = 0, pattern = 0;
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
  float observedX = 80, observedVx = 0;
  uint32_t lastAttack = 0;
  int lastPattern = 0;
  int weapon = 0, invincible = 0, shield = 0, rapid = 0, dual = 0, combo = 1;
  int cooldown = 0, shotTimer = 0, spawnTimer = 0, moveTimer = 0, dir = 1, clearTimer = 0,
      spawned = 0, bombs = 1;
  uint32_t lastKill = 0;
  uint32_t waveStarted = 0;
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
  virtual void updateEnemies() = 0;
  void updateBoss(Enemy &e);
  void formationFire();
  bool aimedFire(Enemy &e);
  void collisions();

public:
  bool bossEncounter() const override {
    return bossStage && bossHealth() > 0;
  }
  int bossHealth() const override {
    for (const auto &e : enemies.items)
      if (e.active && e.kind == 4)
        return e.hp;
    return 0;
  }
  explicit Shooter(ShooterMode m) : mode(m) {}
  void start(const GameOptions &) override;
  void update() override;
  void render(Canvas &) const override;
  void cleanup() override;
  size_t activeEntities() const override {
    return shots.count() + hostile.count() + enemies.count() + particles.count() + powers.count();
  }
};
} // namespace arcade
