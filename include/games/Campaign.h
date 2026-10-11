#pragma once
#include "assets/Visuals.h"
#include "games/Registration.h"
namespace arcade {
// Shared presentation and campaign lifecycle; each title owns its rules and boss AI.
class CampaignGame : public TickGame {
protected:
  int grace = 90, transition = 0;
  uint32_t stageTick = 0;
  struct Spark {
    bool active = false;
    float x = 0, y = 0, vx = 0, vy = 0;
    int ttl = 0;
  };
  Pool<Spark, 24> sparks;
  virtual void play() = 0;
  virtual void setupStage() = 0;
  void simulate() final {
    for (auto &s : sparks.items)
      if (s.active) {
        s.x += s.vx;
        s.y += s.vy;
        if (--s.ttl <= 0)
          s.active = false;
      }
    if (grace > 0)
      --grace;
    if (transition) {
      if (--transition == 0) {
        advanceStage();
        if (!ended) {
          stageTick = 0;
          grace = 90;
          setupStage();
        }
      }
      return;
    }
    ++stageTick;
    play();
  }
  void hit() {
    if (grace || ended || transition)
      return;
    loseLife();
    grace = 100;
  }
  void burst(float x, float y) {
    sound(Sound::Explosion);
    if (options.effects)
      for (int i = 0; i < 8; ++i)
        if (auto *s = sparks.acquire()) {
          s->x = x;
          s->y = y;
          s->vx = rng.range(-18, 19) * 0.1f;
          s->vy = rng.range(-18, 19) * 0.1f;
          s->ttl = 24;
        }
  }
  void clearStage() {
    if (transition || ended)
      return;
    add(500 * level);
    transition = 90;
    grace = 100;
    sound(Sound::Wave);
  }
  void beginCampaign(const GameOptions &o) {
    baseStart(o);
    health = o.difficulty == 0 ? 5 : o.difficulty == 2 ? 2 : 3;
    grace = 90;
    transition = 0;
    stageTick = 0;
    sparks.clear();
    setupStage();
  }
  bool bossRound() const {
    return level % 4 == 0;
  }
  void scenery(Canvas &c, uint16_t accent) const {
    visual::stars(c, options.effects ? tick : 0);
    for (const auto &s : sparks.items)
      if (s.active)
        c.rect(int(s.x), int(s.y), 2, 2, Orange);
    if (transition) {
      c.rect(15, 99, 146, 25, Navy);
      c.centered(108, "STAGE CLEAR", accent);
    }
  }
  void bossBar(Canvas &c, int hp, int maximum) const {
    c.text(5, 24, "BOSS", Pink);
    c.rect(35, 24, 136, 6, Grey, false);
    c.rect(36, 25, clamp(hp, 0, maximum) * 134 / std::max(1, maximum), 4, Red);
  }
  static void ship(Canvas &c, float x, float y, uint16_t color) {
    visual::ship(c, int(x) - 7, int(y) - 7, color);
  }

public:
  void start(const GameOptions &o) override {
    beginCampaign(o);
  }
  void cleanup() override {
    Game::cleanup();
    sparks.clear();
  }
};
struct Projectile {
  bool active = false;
  float x = 0, y = 0, vx = 0, vy = 0;
  int life = 180, damage = 1, owner = -1;
};
template <size_t N>
inline void projectile(Pool<Projectile, N> &pool, float x, float y, float vx, float vy,
                       int damage = 1, int owner = -1) {
  if (auto *p = pool.acquire()) {
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->damage = damage;
    p->owner = owner;
  }
}
template <size_t N> inline void moveProjectiles(Pool<Projectile, N> &pool) {
  for (auto &p : pool.items)
    if (p.active) {
      p.x += p.vx;
      p.y += p.vy;
      if (--p.life <= 0 || p.x < 0 || p.x >= W || p.y < Top || p.y >= H)
        p.active = false;
    }
}
template <size_t N>
inline void drawProjectiles(Canvas &c, const Pool<Projectile, N> &pool, uint16_t color) {
  for (const auto &p : pool.items)
    if (p.active)
      c.circle(int(p.x), int(p.y), 2, color, true);
}
inline bool near(float x, float y, float xx, float yy, float radius) {
  return (x - xx) * (x - xx) + (y - yy) * (y - yy) < radius * radius;
}
} // namespace arcade
