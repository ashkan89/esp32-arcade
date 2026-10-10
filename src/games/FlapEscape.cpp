#include "games/FlapEscape.h"
namespace arcade {
void FlapEscape::start(const GameOptions &o) {
  baseStart(o);
  gates.clear();
  y = 110;
  vy = 0;
  spawn = 35;
  flying = false;
  health = 1;
}
void FlapEscape::update() {
  if (paused || ended)
    return;
  tick++;
  if (input.hit(Fire)) {
    flying = true;
    vy = -2.65f;
    sound(Sound::Shot);
  }
  input.pressed = 0;
  if (!flying)
    return;
  vy = std::min(3.2f, vy + 0.085f);
  y += vy;
  float speed = 1.0f + std::min(0.6f, level * 0.04f) + options.difficulty * 0.08f;
  int gap = 78 - options.difficulty * 4 - std::min(10, int(level));
  if (--spawn <= 0) {
    spawn = int(104 / speed);
    if (auto *g = gates.acquire()) {
      g->x = W + 8;
      g->gapY = rng.range(48, 124);
    }
  }
  for (auto &g : gates.items) {
    if (!g.active)
      continue;
    g.x -= speed;
    if (overlaps({38, y - 5, 10, 10}, {g.x, float(Top), 20, float(g.gapY - Top)}) ||
        overlaps({38, y - 5, 10, 10}, {g.x, float(g.gapY + gap), 20, float(H - g.gapY - gap)})) {
      ended = true;
      health = 0;
      sound(Sound::Over);
    }
    if (!g.passed && g.x + 20 < 38) {
      g.passed = true;
      add(1);
      progressTo(uint16_t(std::min(13u, 1 + points / 5)));
      sound(Sound::Power);
    }
    if (g.x < -24)
      g.active = false;
  }
  if (y - 5 < Top || y + 5 > H) {
    ended = true;
    health = 0;
    sound(Sound::Over);
  }
}
void FlapEscape::render(Canvas &c) const {
  int gap = 78 - options.difficulty * 4 - std::min(10, int(level));
  for (auto &g : gates.items)
    if (g.active) {
      c.rect(int(g.x), Top, 20, g.gapY - Top, Green);
      c.rect(int(g.x), g.gapY + gap, 20, H - g.gapY - gap, Green);
      c.rect(int(g.x) - 2, g.gapY - 5, 24, 5, Cyan);
      c.rect(int(g.x) - 2, g.gapY + gap, 24, 5, Cyan);
    }
  c.circle(43, int(y), 5, Yellow, true);
  c.pixel(45, int(y) - 2, Black);
  c.rect(47, int(y), 5, 2, Orange);
  if (!flying) {
    c.centered(148, "FIRE TO FLAP", Yellow);
    c.centered(164, "TAP. RELEASE. REPEAT.", White);
  }
}
} // namespace arcade

#include "games/Registration.h"
namespace arcade {
GameEntry describeFlapEscape() {
  return gameEntry<FlapEscape>(
      {"FLAP ESCAPE",
       "THREAD THE GREEN GATES",
       {"TAP A TO FLAP", "RELEASE BEFORE NEXT TAP", "ONE POINT PER GATE", "GAPS NARROW GRADUALLY",
        "AVOID FLOOR AND CEILING", "A REPLAYS AFTER LOSS"},
       Yellow,
       7,
       Fire});
}
} // namespace arcade
