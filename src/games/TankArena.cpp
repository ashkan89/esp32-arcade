#include "games/Registration.h"
namespace arcade {
class TankArena final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyTankArena();
#endif
  struct Tank {
    bool active = false;
    float x = 0, y = 0;
    int hp = 2;
    int attackTimer = 90, aim = 2, detour = 1, detourAxis = 0, detourTicks = 0;
  };
  struct Shell {
    bool active = false;
    float x = 0, y = 0, vx = 0, vy = 0;
    bool hostile = false;
  };
  Pool<Tank, 8> enemies;
  Pool<Shell, 32> shells;
  float x = 88, y = 180;
  int direction = 0, cooldown = 0, grace = 0;
  static Rect cover(int i) {
    return {float(20 + i % 2 * 108), float(65 + i / 2 * 70), 28, 18};
  }
  bool free(Rect box) const {
    if (box.x < 2 || box.y < Top || box.x + box.w > W - 2 || box.y + box.h > H - 2)
      return false;
    for (int i = 0; i < 4; i++)
      if (overlaps(box, cover(i)))
        return false;
    return true;
  }
  void wave() {
    enemies.clear();
    shells.clear();
    for (int i = 0; i < (level % 4 == 0 ? 1 : std::min(8, 3 + int(level))); i++)
      if (auto *e = enemies.acquire()) {
        e->x = 8 + i % 4 * 46;
        e->y = 28 + i / 4 * 22;
        e->hp = level % 4 == 0 ? 20 + int(level) : 2;
        e->attackTimer = 70 + i * 17;
        e->detour = i % 2 ? 1 : -1;
      }
    grace = 90;
  }
  bool enemyFree(const Tank &tank, Rect box) const {
    if (!free(box))
      return false;
    for (const auto &other : enemies.items)
      if (&other != &tank && other.active && overlaps(box, {other.x, other.y, 12, 12}))
        return false;
    return true;
  }
  void shoot(float xx, float yy, int dir, bool hostile) {
    static const int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
    if (auto *b = shells.acquire()) {
      b->x = xx;
      b->y = yy;
      b->vx = dx[dir] * 3.0f;
      b->vy = dy[dir] * 3.0f;
      b->hostile = hostile;
    }
  }
  void simulate() override {
    if (grace)
      grace--;
    int dx = int(input.down(Right)) - int(input.down(Left)),
        dy = int(input.down(Down)) - int(input.down(Up));
    if (dx)
      direction = dx > 0 ? 1 : 3;
    else if (dy)
      direction = dy > 0 ? 2 : 0;
    if (free({x + dx * 1.4f, y, 12, 12}))
      x += dx * 1.4f;
    if (free({x, y + dy * 1.4f, 12, 12}))
      y += dy * 1.4f;
    if (cooldown)
      cooldown--;
    if (input.down(Fire) && !cooldown) {
      shoot(x + 6, y + 6, direction, false);
      cooldown = 18;
      sound(Sound::Shot);
    }
    for (auto &e : enemies.items)
      if (e.active) {
        float sx = clamp(x - e.x, -.35f, .35f), sy = clamp(y - e.y, -.35f, .35f);
        if (e.detourTicks) {
          --e.detourTicks;
          if (e.detourAxis == 1)
            sx = e.detour * .35f;
          else
            sy = e.detour * .35f;
        }
        bool canX = enemyFree(e, {e.x + sx, e.y, 12, 12});
        if (canX)
          e.x += sx;
        bool canY = enemyFree(e, {e.x, e.y + sy, 12, 12});
        if (canY)
          e.y += sy;
        if (!canX && !canY)
          e.detour = -e.detour;
        if (!canX) {
          e.detourAxis = 2;
          e.detourTicks = 90;
        } else if (!canY) {
          e.detourAxis = 1;
          e.detourTicks = 90;
        }
        if (overlaps({x, y, 12, 12}, {e.x, e.y, 12, 12}) && !grace) {
          loseLife();
          grace = 90;
        }
        if (--e.attackTimer == 18)
          e.aim = std::abs(x - e.x) > std::abs(y - e.y) ? (x > e.x ? 1 : 3) : (y > e.y ? 2 : 0);
        if (e.attackTimer <= 0) {
          if (shells.count() < 16)
            shoot(e.x + 6, e.y + 6, e.aim, true);
          e.attackTimer = hazardInterval(level % 4 == 0 ? 65 : 130) + rng.range(0, 30);
        }
      }
    for (auto &b : shells.items)
      if (b.active) {
        float ox = b.x, oy = b.y;
        b.x += b.vx;
        b.y += b.vy;
        if (!free({b.x - 1, b.y - 1, 2, 2})) {
          b.active = false;
          continue;
        }
        if (b.hostile) {
          if (!grace && swept(ox, oy, b.x, b.y, {x, y, 12, 12}, 1)) {
            b.active = false;
            loseLife();
            grace = 90;
          }
        } else
          for (auto &e : enemies.items)
            if (e.active && swept(ox, oy, b.x, b.y, {e.x, e.y, 12, 12}, 1)) {
              b.active = false;
              if (--e.hp == 0) {
                e.active = false;
                add(100);
                sound(Sound::Explosion);
              }
              break;
            }
      }
    if (!enemies.count() && !ended) {
      nextStage();
      if (ended)
        return;
      wave();
    }
  }

public:
  bool bossEncounter() const override {
    return level % 4 == 0 && enemies.count() > 0;
  }
  int bossHealth() const override {
    for (const auto &e : enemies.items)
      if (e.active && level % 4 == 0)
        return e.hp;
    return 0;
  }
  void start(const GameOptions &o) override {
    baseStart(o);
    x = 82;
    y = 180;
    direction = cooldown = 0;
    wave();
  }
  size_t activeEntities() const override {
    return enemies.count() + shells.count() + 1;
  }
  void cleanup() override {
    Game::cleanup();
    enemies.clear();
    shells.clear();
  }
  void render(Canvas &c) const override {
    if (bossEncounter()) {
      c.text(5, 24, "BOSS", Pink);
      c.rect(35, 24, bossHealth() * 136 / (20 + int(level)), 4, Red);
    }
    for (int i = 0; i < 4; i++) {
      auto r = cover(i);
      c.rect(int(r.x), int(r.y), 28, 18, Grey);
    }
    for (const auto &e : enemies.items)
      if (e.active) {
        c.rect(int(e.x), int(e.y), 12, 12, Red);
        if (e.attackTimer <= 18) {
          static const int dx[] = {0, 15, 0, -15}, dy[] = {-15, 0, 15, 0};
          c.line(int(e.x) + 6, int(e.y) + 6, int(e.x) + 6 + dx[e.aim], int(e.y) + 6 + dy[e.aim],
                 Yellow);
        }
        if (level % 4 == 0)
          c.rect(int(e.x) - 2, int(e.y) - 2, 16, 16, Pink, false);
      }
    if (!grace || tick % 10 < 5) {
      c.rect(int(x), int(y), 12, 12, Green);
      static const int dx[] = {0, 10, 0, -10}, dy[] = {-10, 0, 10, 0};
      c.line(int(x) + 6, int(y) + 6, int(x) + 6 + dx[direction], int(y) + 6 + dy[direction], White);
    }
    for (const auto &b : shells.items)
      if (b.active)
        c.circle(int(b.x), int(b.y), 2, b.hostile ? Pink : Yellow, true);
  }
};
GameEntry describeTankArena() {
  return gameEntry<TankArena>(
      {"TANK ARENA",
       "CLEAR THE ARMORED PATROL",
       {"D-PAD MOVES AND AIMS", "HOLD A TO FIRE SHELLS", "COVER BLOCKS BOTH SIDES",
        "BOSS TANKS TAKE MANY HITS", "CLEAR PATROL TO ADVANCE", "TWELVE WAVES TO WIN"},
       Green,
       16,
       Fire});
}

#ifdef ARCADE_HOST_TESTS
bool verifyTankArena() {
  TankArena g;
  g.start({});
  g.enemies.clear();
  auto *pursuer = g.enemies.acquire();
  pursuer->x = 30;
  pursuer->y = 50;
  pursuer->detour = 1;
  auto *neighbor = g.enemies.acquire();
  neighbor->x = 60;
  neighbor->y = 50;
  HOST_CHECK(!g.enemyFree(*pursuer, {neighbor->x, neighbor->y, 12, 12}));
  neighbor->active = false;
  g.x = 30;
  g.y = 180;
  for (int t = 0; t < 600; ++t) {
    g.grace = 100;
    ++g.tick;
    g.simulate();
    HOST_CHECK(g.free({pursuer->x, pursuer->y, 12, 12}));
  }
  HOST_CHECK(pursuer->y > 90);
  g.start({});
  g.level = 12;
  g.wave();
  g.enemies.items[0].hp = 1;
  g.shoot(g.enemies.items[0].x + 6, g.enemies.items[0].y + 6, 2, false);
  g.tick = 1;
  g.simulate();
  HOST_CHECK(g.campaignComplete() && g.gameOver() && g.stage() == 12);
  auto score = g.score();
  for (int i = 0; i < 100; ++i)
    g.update();
  HOST_CHECK(g.score() == score);
  g.start({});
  HOST_CHECK(!g.campaignComplete() && !g.gameOver() && g.stage() == 1 && g.score() == 0);
  return true;
}
#endif
} // namespace arcade
