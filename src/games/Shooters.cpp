#include "games/Shooters.h"
#include "assets/Sprites.h"
#include <cstdio>
namespace arcade {
void Shooter::start(const GameOptions &o) {
  baseStart(o);
  shots.clear();
  hostile.clear();
  enemies.clear();
  particles.clear();
  powers.clear();
  px = 80;
  py = 198;
  autoDir = dir = 1;
  weapon = 0;
  invincible = shield = rapid = dual = cooldown = clearTimer = 0;
  combo = 1;
  lastKill = 0;
  shake = 0;
  bombs = 1;
  ufoActive = beam = false;
  ufoX = -30;
  ufoTimer = 0;
  wave();
}
void Shooter::wave() {
  shots.clear();
  hostile.clear();
  enemies.clear();
  powers.clear();
  damaged = false;
  spawned = spawnTimer = shotTimer = moveTimer = 0;
  dir = 1;
  bossStage = mode != ShooterMode::Meteor && level % (mode == ShooterMode::Sky ? 4 : 5) == 0;
  for (auto &s : barriers) {
    s.fill(3);
    for (int y = 7; y < 12; y++)
      for (int x = 7; x < 13; x++)
        s[y * 20 + x] = 0;
  }
  if (bossStage) {
    auto *e = enemies.acquire();
    e->kind = 4;
    e->x = 52;
    e->y = 34;
    e->hp = 40 + std::min(200, int(level) * 5);
    e->vx = 1.0f;
    return;
  }
  if (mode == ShooterMode::Sly || mode == ShooterMode::Star) {
    for (int r = 0; r < 4; r++)
      for (int c = 0; c < 6; c++) {
        auto *e = enemies.acquire();
        e->x = e->homeX = 12 + c * 24 + (level % 3 == 0 ? (r % 2) * 3 : 0);
        e->y = e->homeY = 34 + r * 18 + (level % 3 == 2 ? (c % 2) * 6 : 0);
        e->hp = r == 0 ? 2 : 1;
        e->kind = uint8_t(r == 0 ? 2 : r == 1 ? 1 : 0);
      }
  }
}
void Shooter::enemyFire(float x, float y, float vx, float vy) {
  if (auto *b = hostile.acquire()) {
    b->x = b->oldX = x;
    b->y = b->oldY = y;
    b->vx = vx;
    b->vy = vy;
  }
}
void Shooter::fire() {
  int n = weapon == 1 ? 3 : 1, volleys = dual > 0 ? 2 : 1;
  for (int v = 0; v < volleys; v++)
    for (int i = 0; i < n; i++)
      if (auto *b = shots.acquire()) {
        b->x = b->oldX = px + 8 + (volleys == 2 ? (v ? 4 : -4) : 0);
        b->y = b->oldY = py;
        b->vx = n == 3 ? (i - 1) * 1.6f : 0;
        b->vy = weapon == 2 ? -4.5f : -6.5f;
        b->damage = weapon == 2 ? 2 : 1;
      }
  sound(Sound::Shot);
}
void Shooter::explode(float x, float y, uint16_t c) {
  if (!options.effects)
    return;
  for (int i = 0; i < 10; i++)
    if (auto *p = particles.acquire()) {
      p->x = x;
      p->y = y;
      p->vx = rng.range(-20, 21) * 0.1f;
      p->vy = rng.range(-20, 21) * 0.1f;
      p->life = 18 + rng.range(0, 12);
      p->color = c;
    }
}
void Shooter::hurt() {
  if (invincible > 0 || ended)
    return;
  if (shield > 0) {
    shield = 0;
    invincible = 45;
    sound(Sound::Hit);
    return;
  }
  --health;
  damaged = true;
  invincible = 100;
  shake = options.effects ? 12 : 0;
  combo = 1;
  explode(px + 8, py + 6, Red);
  sound(Sound::Explosion);
  if (health <= 0) {
    ended = true;
    sound(Sound::Over);
  }
}
void Shooter::power(uint8_t k) {
  switch (k) {
  case 0:
    rapid = 480;
    break;
  case 1:
    health = std::min(5, health + 1);
    break;
  case 2:
    shield = 480;
    break;
  case 3:
    dual = 480;
    break;
  case 4:
    weapon = (weapon + 1) % 3;
    break;
  }
  sound(Sound::Power);
}
bool Shooter::barrier(float x, float y, int damage) {
  if (mode != ShooterMode::Sly)
    return false;
  for (unsigned s = 0; s < 3; s++) {
    int lx = int(x) - (22 + 52 * int(s)), ly = int(y) - 168;
    if (lx < 0 || ly < 0 || lx >= 20 || ly >= 12 || !barriers[s][ly * 20 + lx])
      continue;
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++) {
        int a = lx + dx, b = ly + dy;
        if (a >= 0 && a < 20 && b >= 0 && b < 12) {
          auto &hp = barriers[s][b * 20 + a];
          hp = uint8_t(std::max(0, int(hp) - damage));
        }
      }
    return true;
  }
  return false;
}
void Shooter::spawnSky() {
  int n = std::min(3, 24 - spawned);
  int pattern = (spawned / 3) % 3;
  for (int i = 0; i < n; i++)
    if (auto *e = enemies.acquire()) {
      e->x = 24 + i * 52;
      e->homeX = e->x;
      e->y = -12 - i * 18;
      e->kind = uint8_t(pattern);
      e->hp = 1 + int(level) / 5;
      e->vy = 0.7f + std::min(1.8f, level * 0.08f);
      e->vx = pattern == 2 ? (i % 2 ? -0.7f : 0.7f) : 0;
      spawned++;
    }
}
void Shooter::spawnRock(float x, float y, uint8_t size, float vx) {
  if (auto *e = enemies.acquire()) {
    e->kind = 5;
    e->size = size;
    e->x = x;
    e->y = y;
    e->vx = vx;
    e->vy = 0.7f + std::min(2.0f, level * 0.1f) + (2 - size) * 0.25f;
    e->hp = 1;
  }
}
void Shooter::killed(Enemy &e) {
  float x = e.x, y = e.y;
  int kind = e.kind, size = e.size;
  e.active = false;
  if (mode == ShooterMode::Star || mode == ShooterMode::Meteor) {
    combo = lastKill != 0 && elapsed(tick, lastKill) < 120 ? std::min(8, combo + 1) : 1;
    lastKill = tick;
  }
  add(uint32_t(kind == 4 ? 500 : kind == 5 ? 10 * (3 - size) : 10 + 5 * kind) * uint32_t(combo));
  explode(x + 7, y + 5, kind == 5 ? Grey : Orange);
  sound(Sound::Explosion);
  if (kind == 5 && size > 0) {
    spawnRock(x - 4, y, uint8_t(size - 1), -0.9f);
    spawnRock(x + 4, y, uint8_t(size - 1), 0.9f);
  } else if (kind != 5 && (kind == 4 || rng.range(0, 100) < 18)) {
    if (auto *p = powers.acquire()) {
      p->x = x + 7;
      p->y = y + 5;
      p->kind = uint8_t(rng.range(0, 5));
    }
  }
}
void Shooter::updateEnemies() {
  if (mode == ShooterMode::Sky && !bossStage && spawned < 24 && --spawnTimer <= 0) {
    spawnTimer = 95;
    spawnSky();
  }
  if (mode == ShooterMode::Meteor && --spawnTimer <= 0) {
    spawnTimer = std::max(20, 95 - int(level) * 4 - options.difficulty * 10);
    spawnRock(float(rng.range(14, W - 28)), -26, 2, float(rng.range(-4, 5)) * 0.1f);
  }
  bool step = ++moveTimer >= std::max(5, 19 - int(level));
  if (step)
    moveTimer = 0;
  if (mode == ShooterMode::Sly && step && !bossStage) {
    float mn = W, mx = 0;
    for (auto &e : enemies.items)
      if (e.active && e.kind != 3) {
        mn = std::min(mn, e.x);
        mx = std::max(mx, e.x + 14);
      }
    bool edge = (dir > 0 && mx > W - 7) || (dir < 0 && mn < 7);
    if (edge)
      dir = -dir;
    for (auto &e : enemies.items)
      if (e.active && e.kind != 3) {
        if (edge)
          e.y += 7;
        else
          e.x += dir * 3;
        if (e.y + 10 >= py) {
          health = 0;
          ended = true;
        }
      }
  }
  for (auto &e : enemies.items) {
    if (!e.active)
      continue;
    e.age++;
    if (e.kind == 4) {
      e.x += e.vx * (e.hp < 20 ? 1.6f : 1.0f);
      if (e.x < 4 || e.x > W - 76) {
        e.x = clamp(e.x, 4.0f, float(W - 76));
        e.vx = -e.vx;
      }
      if (e.age % uint32_t(e.hp < 20 ? 38 : 65) == 0) {
        int p = int(e.age / 65) % 3;
        if (p == 0) {
          for (int k = -2; k <= 2; k++)
            enemyFire(e.x + 36, e.y + 26, k * 0.7f, 2.0f + options.difficulty * 0.3f);
        } else if (p == 1) {
          for (int k = 0; k < 5; k++)
            enemyFire(12 + k * 34, e.y + 26, 0, 2.5f);
        } else
          for (int k = -1; k <= 1; k++)
            enemyFire(e.x + 36, e.y + 26, clamp((px - e.x - 28) / 60, -1.5f, 1.5f) + k * 0.4f,
                      2.5f);
      }
      continue;
    }
    if (mode == ShooterMode::Meteor) {
      e.x += e.vx;
      e.y += e.vy;
      int r = 5 + e.size * 5;
      if (e.x < r || e.x > W - r)
        e.vx = -e.vx;
      if (e.y > H + r)
        e.active = false;
      continue;
    }
    if (mode == ShooterMode::Sky) {
      e.y += e.vy;
      if (e.kind == 1)
        e.x = e.homeX + std::sin(e.age * 0.055f) * 18;
      if (e.kind == 2) {
        e.x += e.vx;
        if (e.x < 5 || e.x > W - 19)
          e.vx = -e.vx;
      }
      if (e.age % uint32_t(std::max(55, 130 - int(level) * 4)) == 0 && e.y > Top)
        enemyFire(e.x + 7, e.y + 10, clamp((px - e.x) / 120, -0.8f, 0.8f),
                  2.0f + options.difficulty * 0.4f);
      if (e.y > H) {
        e.active = false;
      }
      continue;
    }
    if (mode == ShooterMode::Star) {
      if (e.kind != 3) {
        e.x = e.homeX + std::sin(tick * 0.022f) * 8;
        if (e.age > 90 && rng.range(0, 900) < 2 + std::min(8, int(level))) {
          e.kind = 3;
          e.age = 0;
          e.vx = (px - e.x) / 90;
          e.vy = 1.4f;
        }
      } else {
        e.x += e.vx + std::sin(e.age * 0.06f) * 1.4f;
        e.y += e.vy;
        if (e.age % 70 == 0)
          enemyFire(e.x + 7, e.y + 10, 0, 2.5f);
        if (e.y > H + 10) {
          e.y = e.homeY;
          e.x = e.homeX;
          e.kind = 1;
          e.age = 0;
        }
      }
    }
    if (mode == ShooterMode::Sly && level >= 3 && e.kind != 3 && rng.range(0, 10000) < 2) {
      e.kind = 3;
      e.vx = (px - e.x) / 85;
      e.vy = 1.7f;
    }
    if (mode == ShooterMode::Sly && e.kind == 3) {
      e.x += e.vx + std::sin(e.age * 0.07f) * 0.6f;
      e.y += e.vy;
      if (e.y > H)
        e.active = false;
    }
  }
  if ((mode == ShooterMode::Sly || mode == ShooterMode::Star) && !bossStage && --shotTimer <= 0) {
    shotTimer = std::max(18, 55 - int(level) * 3 - options.difficulty * 5);
    std::array<Enemy *, 6> bottom{};
    for (unsigned i = 0; i < enemies.items.size(); i++) {
      auto &e = enemies.items[i];
      if (e.active && (bottom[i % 6] == nullptr || bottom[i % 6]->y < e.y))
        bottom[i % 6] = &e;
    }
    unsigned c = unsigned(rng.range(0, 6));
    if (bottom[c])
      enemyFire(bottom[c]->x + 7, bottom[c]->y + 10, 0, 2.5f + options.difficulty * 0.3f);
  }
}
void Shooter::collisions() {
  for (auto &b : shots.items) {
    if (!b.active)
      continue;
    // Shields are checked along the path before targets above them.
    bool hitBarrier = false;
    for (float y = std::min(b.oldY, b.y); y <= std::max(b.oldY, b.y); y += 1)
      if (barrier(b.x, y, b.damage == 2 ? 3 : 2)) {
        hitBarrier = true;
        break;
      }
    if (hitBarrier) {
      b.active = false;
      continue;
    }
    if (ufoActive && swept(b.oldX, b.oldY, b.x, b.y, {ufoX, 24, 24, 10}, 2)) {
      b.active = false;
      ufoActive = beam = false;
      add(200);
      explode(ufoX + 12, 29, Orange);
      continue;
    }
    for (auto &e : enemies.items) {
      if (!e.active)
        continue;
      float r = mode == ShooterMode::Meteor ? float(5 + e.size * 5) : 0;
      Rect box = mode == ShooterMode::Meteor
                     ? Rect{e.x - r, e.y - r, 2 * r, 2 * r}
                     : Rect{e.x, e.y, e.kind == 4 ? 72.0f : 14.0f, e.kind == 4 ? 26.0f : 10.0f};
      bool hit = mode == ShooterMode::Meteor
                     ? segmentCircle(b.oldX, b.oldY, b.x, b.y, e.x, e.y, r + b.damage)
                     : swept(b.oldX, b.oldY, b.x, b.y, box, float(b.damage));
      if (hit) {
        b.active = false;
        e.hp -= b.damage;
        if (e.hp <= 0)
          killed(e);
        else
          sound(Sound::Hit);
        break;
      }
    }
  }
  for (auto &b : hostile.items) {
    if (!b.active)
      continue;
    bool blocked = false;
    for (float y = b.oldY; y <= b.y; y += 1)
      if (barrier(b.x, y, 1)) {
        blocked = true;
        break;
      }
    if (blocked) {
      b.active = false;
      continue;
    }
    if (swept(b.oldX, b.oldY, b.x, b.y, {px + 2, py + 2, 12, 9}, 1)) {
      b.active = false;
      hurt();
    }
  }
  for (auto &e : enemies.items) {
    if (!e.active)
      continue;
    float r = mode == ShooterMode::Meteor ? float(5 + e.size * 5) : 0;
    Rect box = mode == ShooterMode::Meteor
                   ? Rect{e.x - r, e.y - r, 2 * r, 2 * r}
                   : Rect{e.x, e.y, e.kind == 4 ? 72.0f : 14.0f, e.kind == 4 ? 26.0f : 10.0f};
    Rect player{px + 2, py + 2, 12, 9};
    bool hit =
        mode == ShooterMode::Meteor ? circleRect(e.x, e.y, r, player) : overlaps(box, player);
    if (hit) {
      hurt();
      if (e.kind != 4)
        e.active = false;
    }
  }
  if (beam && ufoActive && ufoTimer % 180 > 100 &&
      overlaps({ufoX + 10, 34, 5, float(H - 34)}, {px + 2, py + 2, 12, 9}))
    hurt();
  for (auto &p : powers.items)
    if (p.active && overlaps({p.x - 4, p.y - 4, 8, 8}, {px, py, 16, 12})) {
      power(p.kind);
      p.active = false;
    }
}
void Shooter::update() {
  if (paused || ended)
    return;
  tick++;
  if (clearTimer > 0) {
    if (--clearTimer == 0) {
      level = uint16_t(std::min(999, int(level) + 1));
      bombs = 1;
      wave();
    }
    input.pressed = 0;
    return;
  }
  if (invincible > 0)
    --invincible;
  if (shield > 0)
    --shield;
  if (rapid > 0)
    --rapid;
  if (dual > 0)
    --dual;
  if (shake > 0)
    --shake;
  if (options.oneButton) {
    if (input.hit(Fire))
      autoDir = -autoDir;
    px += autoDir * 1.6f;
    if (px <= 2 || px >= W - 18)
      autoDir = -autoDir;
  } else {
    px += (int(input.down(Right)) - int(input.down(Left))) * 2.6f;
    if (options.expanded)
      py += (int(input.down(Down)) - int(input.down(Up))) * 1.8f;
  }
  px = clamp(px, 2.0f, float(W - 18));
  py = clamp(py, 150.0f, float(H - 14));
  if (input.hit(Special)) {
    if (mode == ShooterMode::Sky && bombs > 0) {
      bombs--;
      hostile.clear();
      shield = 180;
      for (auto &e : enemies.items)
        if (e.active) {
          e.hp -= 8;
          if (e.hp <= 0)
            killed(e);
        }
      sound(Sound::Power);
    } else if (mode != ShooterMode::Sky)
      weapon = (weapon + 1) % 3;
  }
  if (cooldown > 0)
    --cooldown;
  if ((input.down(Fire) || options.oneButton) && cooldown == 0) {
    fire();
    cooldown = rapid > 0 ? 5 : 12;
  }
  updateEnemies();
  for (auto &b : shots.items)
    if (b.active) {
      b.oldX = b.x;
      b.oldY = b.y;
      b.x += b.vx;
      b.y += b.vy;
    }
  for (auto &b : hostile.items)
    if (b.active) {
      b.oldX = b.x;
      b.oldY = b.y;
      b.x += b.vx;
      b.y += b.vy;
    }
  for (auto &p : powers.items)
    if (p.active) {
      p.y += 1.2f;
      if (p.y > H)
        p.active = false;
    }
  if (mode == ShooterMode::Sly) {
    ufoTimer++;
    if (!ufoActive && ufoTimer >= 900) {
      ufoTimer = 0;
      ufoX = -24;
      ufoActive = true;
    }
    if (ufoActive) {
      ufoX += 1.2f;
      beam = ufoTimer % 180 > 70;
      if (ufoX > W + 24)
        ufoActive = beam = false;
    }
  }
  collisions();
  for (auto &b : shots.items)
    if (b.active && (b.y < Top - 8 || b.x < -4 || b.x > W + 4))
      b.active = false;
  for (auto &b : hostile.items)
    if (b.active && (b.y > H + 8 || b.x < -4 || b.x > W + 4))
      b.active = false;
  for (auto &p : particles.items)
    if (p.active) {
      p.x += p.vx;
      p.y += p.vy;
      p.vy += 0.06f;
      if (--p.life <= 0)
        p.active = false;
    }
  if (mode == ShooterMode::Meteor) {
    if (tick % 900 == 0) {
      level = uint16_t(std::min(999, int(level) + 1));
      add(100);
      sound(Sound::Wave);
    }
  } else if (!ended && enemies.count() == 0 &&
             (mode != ShooterMode::Sky || spawned >= 24 || bossStage)) {
    add(100 * uint32_t(level) + (mode == ShooterMode::Star && !damaged ? 200 : 0));
    clearTimer = 90;
    hostile.clear();
    sound(Sound::Wave);
  }
  input.pressed = 0;
}
void Shooter::render(Canvas &c) const {
  int sx = shake ? int(tick % 3) - 1 : 0;
  if (mode == ShooterMode::Sky) {
    for (int y = Top; y < H; y++) {
      int center = 88 + int(std::sin((y + tick) * 0.018f) * 25);
      c.rect(center - 27, y, 54, 1, 0x092b);
    }
    for (int i = 0; i < 9; i++) {
      int y = Top + int((i * 27 + tick) % 200);
      c.rect(8 + (i % 2) * 145, y, 8, 7, 0x1263);
    }
  } else
    for (int i = 0; i < 30; i++)
      c.pixel((i * 47 + 13) % W, Top + int((i * 31 + tick / (1 + i % 3)) % (H - Top)),
              i % 3 == 0 ? White : Grey);
  if (mode == ShooterMode::Sly)
    for (unsigned s = 0; s < 3; s++)
      for (int y = 0; y < 12; y++)
        for (int x = 0; x < 20; x++)
          if (barriers[s][y * 20 + x])
            c.pixel(22 + int(s) * 52 + x, y + 168, barriers[s][y * 20 + x] > 1 ? 0x041f : 0x0210);
  for (auto &e : enemies.items)
    if (e.active) {
      int x = int(e.x) + sx, y = int(e.y);
      if (e.kind == 4) {
        c.rect(x, y, 72, 26, Pink);
        c.rect(x + 8, y + 6, 12, 6, Black);
        c.rect(x + 52, y + 6, 12, 6, Black);
        c.rect(x + 20, y + 19, 32, 3, Black);
        c.rect(30, Top + 1, 116, 3, Grey);
        c.rect(30, Top + 1, clamp(e.hp * 116 / (40 + std::min(200, int(level) * 5)), 0, 116), 3,
               Red);
      } else if (mode == ShooterMode::Meteor) {
        int r = 5 + e.size * 5;
        c.circle(x, y, r, Grey, true);
        c.circle(x - 3, y - 3, std::max(1, r / 3), Black, true);
        c.line(x + r / 2, y, x, y + r, White);
      } else if (mode == ShooterMode::Sky) {
        c.rect(x + 5, y, 4, 12, Orange);
        c.line(x, y + 8, x + 13, y + 8, Yellow);
        c.line(x + 3, y + 11, x + 10, y + 11, White);
      } else
        c.bitmap(x, y, e.kind == 0 ? alienA : e.kind == 1 ? alienB : alienC, 14, 10);
    }
  if (ufoActive) {
    c.bitmap(int(ufoX), 24, ufoSprite, 24, 10);
    if (beam)
      c.line(int(ufoX) + 12, 34, int(ufoX) + 12, H, ufoTimer % 180 > 100 ? Cyan : Grey);
  }
  for (auto &b : shots.items)
    if (b.active)
      c.rect(int(b.x) + sx, int(b.y), b.damage == 2 ? 4 : 2, 6, b.damage == 2 ? Pink : Yellow);
  for (auto &b : hostile.items)
    if (b.active)
      c.rect(int(b.x) + sx, int(b.y), 3, 5, Red);
  for (auto &p : powers.items)
    if (p.active) {
      c.rect(int(p.x) - 4, int(p.y) - 4, 9, 9, Cyan);
      const char *names = "RLSDW";
      char s[2] = {names[p.kind], 0};
      c.text(int(p.x) - 2, int(p.y) - 3, s, Black);
    }
  for (auto &p : particles.items)
    if (p.active)
      c.pixel(int(p.x) + sx, int(p.y), p.color);
  if (invincible == 0 || tick % 8 < 4)
    c.bitmap(int(px) + sx, int(py), playerSprite, 16, 12);
  if (shield > 0)
    c.circle(int(px) + 8, int(py) + 6, 12, Cyan);
  char status[24];
  std::snprintf(status, sizeof(status), "%s %s",
                weapon == 0   ? "LAS"
                : weapon == 1 ? "SPR"
                              : "PLA",
                rapid > 0 ? "RAPID" : "");
  c.text(3, H - 8, status, Grey);
  if (mode == ShooterMode::Sky) {
    c.text(119, H - 8, "BOMB", Grey);
    c.number(149, H - 8, unsigned(bombs), Yellow);
  }
  if (mode == ShooterMode::Star || mode == ShooterMode::Meteor) {
    c.text(128, H - 8, "X", Grey);
    c.number(140, H - 8, unsigned(combo), Cyan);
  }
  if (clearTimer) {
    c.rect(16, 90, 144, 42, Navy);
    c.centered(98, "WAVE CLEAR", Green, 2);
    c.centered(119, "NEXT WAVE...", White);
  }
}
void Shooter::cleanup() {
  Game::cleanup();
  shots.clear();
  hostile.clear();
  enemies.clear();
  powers.clear();
  particles.clear();
  ufoActive = beam = false;
}
} // namespace arcade
