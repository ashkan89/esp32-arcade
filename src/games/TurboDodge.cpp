#include "games/TurboDodge.h"
#include "assets/Visuals.h"
namespace arcade {
void TurboDodge::start(const GameOptions &o) {
  baseStart(o);
  obstacles.clear();
  lane = 1;
  spawn = 0;
  invincible = coins = 0;
  carX = 81;
}
void TurboDodge::spawnRow() {
  int safe = rng.range(0, 3);
  for (int l = 0; l < 3; l++) {
    if (l == safe) {
      if (rng.range(0, 100) < 55)
        if (auto *o = obstacles.acquire()) {
          o->lane = uint8_t(l);
          o->y = -12;
          o->coin = true;
        }
      continue;
    }
    if (auto *o = obstacles.acquire()) {
      o->lane = uint8_t(l);
      o->y = -12;
    }
  }
}
void TurboDodge::update() {
  if (paused || ended)
    return;
  tick++;
  if (invincible > 0)
    invincible--;
  if (input.hit(Left))
    lane = std::max(0, lane - 1);
  if (input.hit(Right))
    lane = std::min(2, lane + 1);
  float target = 29 + lane * 52;
  carX += clamp(target - carX, -6.0f, 6.0f);
  float speed = 1.2f + std::min(2.0f, level * 0.10f) + challenge() * 0.15f;
  // Rows have at least 86 pixels of separation: enough warning and travel time for two lanes.
  if (--spawn <= 0) {
    spawn = int(96 / speed);
    spawnRow();
  }
  for (auto &o : obstacles.items) {
    if (!o.active)
      continue;
    o.y += speed;
    if (overlaps({float(29 + o.lane * 52), o.y, 14, 18}, {carX, 185, 14, 22})) {
      o.active = false;
      if (o.coin) {
        coins++;
        add(50);
        sound(Sound::Power);
      } else if (invincible == 0) {
        health--;
        invincible = hazardInterval(90);
        sound(Sound::Explosion);
        if (health <= 0) {
          ended = true;
          sound(Sound::Over);
        }
      }
    } else if (o.y > H) {
      if (!o.coin)
        add(10);
      o.active = false;
    }
  }
  if (tick % 600 == 0) {
    advanceStage();
    if (ended)
      return;
    sound(Sound::Wave);
  }
  if (tick % 60 == 0)
    add(1);
  input.pressed = 0;
}
void TurboDodge::render(Canvas &c) const {
  c.rect(17, Top, 142, H - Top, 0x1082);
  c.line(17, Top, 17, H - 1, White);
  c.line(158, Top, 158, H - 1, White);
  for (int y = Top; y < H; y += 8) {
    uint16_t curb = ((y + (options.effects ? tick / 2 : 0)) / 8) % 2 ? Red : White;
    c.rect(12, y, 4, 8, curb);
    c.rect(160, y, 4, 8, curb);
  }
  for (int l = 1; l < 3; l++)
    for (int y = Top; y < H; y++)
      if ((y + tick / 2) % 22 < 10)
        c.pixel(17 + l * 52, y, Grey);
  for (auto &o : obstacles.items)
    if (o.active) {
      int x = 29 + o.lane * 52;
      if (o.coin) {
        c.circle(x + 7, int(o.y) + 8, 5, Yellow, true);
        c.circle(x + 7, int(o.y) + 8, 3, Orange);
        c.line(x + 6, int(o.y) + 5, x + 6, int(o.y) + 9, White);
      } else
        visual::car(c, x, int(o.y), Red, 18);
    }
  if (invincible == 0 || tick % 10 < 5) {
    visual::car(c, int(carX), 185, Cyan);
  }
  c.text(4, H - 8, "COINS", Grey);
  c.number(40, H - 8, unsigned(coins), Yellow);
}
} // namespace arcade

#include "games/Registration.h"
namespace arcade {
GameEntry describeTurboDodge() {
  return gameEntry<TurboDodge>(
      {"TURBO DODGE",
       "FIND THE OPEN LANE",
       {"L/R CHANGE LANES", "WATCH THE SAFE LANE", "GOLD COINS GIVE 50", "EACH ROW HAS SAFE LANE",
        "LIVES DEPEND ON MODE", "SPEED RISES EVERY 10S"},
       Yellow,
       6,
       Fire});
}
} // namespace arcade
