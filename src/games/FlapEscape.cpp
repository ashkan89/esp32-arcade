#include "assets/Art.h"
#include "games/FlapEscape.h"
namespace arcade {
void FlapEscape::start(const GameOptions &o) {
  baseStart(o);
  gates.clear();
  y = 110;
  vy = 0;
  spawn = 35;
  lastGap = 85;
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
  float speed = 1.0f + std::min(0.6f, level * 0.04f) + challenge() * 0.08f;
  int gap = 78 - challenge() * 4 - std::min(10, int(level));
  if (--spawn <= 0) {
    spawn = int(104 / speed);
    if (auto *g = gates.acquire()) {
      g->x = W + 8;
      g->gapY = clamp(lastGap + rng.range(-26, 27), 48, 123);
      lastGap = g->gapY;
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
    art::scene(c, 7);
  int gap = 78 - challenge() * 4 - std::min(10, int(level));
  for (auto &g : gates.items)
    if (g.active) {
      art::sprite(c, art::Sprite::Wall, int(g.x), Top, 20, g.gapY - Top);
      art::sprite(c, art::Sprite::Wall, int(g.x), g.gapY + gap, 20, H - g.gapY - gap);
      c.rect(int(g.x) - 2, g.gapY - 5, 24, 5, Cyan);
      c.rect(int(g.x) - 2, g.gapY + gap, 24, 5, Cyan);
    }
  art::object(c, art::Sprite::Bird, 45, int(y), 17, 15);
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
