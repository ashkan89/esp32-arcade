#include "games/Registration.h"
namespace arcade {
class StackTower final : public TickGame {
  struct Slab {
    int x = 0, width = 0;
  };
  std::array<Slab, 16> slabs{};
  int count = 1, width = 92, lastX = 42;
  float moving = 0, speed = 1.5f;
  int direction = 1;
  void simulate() override {
    moving += direction * speed;
    if (moving < 0 || moving + width > W) {
      moving = clamp(moving, 0.0f, float(W - width));
      direction = -direction;
    }
    if (input.hit(Fire)) {
      int x = int(moving), lo = std::max(x, lastX), hi = std::min(x + width, lastX + width),
          overlap = hi - lo;
      if (overlap <= 0) {
        ended = true;
        health = 0;
        sound(Sound::Over);
        return;
      }
      bool perfect = std::abs(x - lastX) <= 2;
      if (perfect) {
        lo = lastX;
        overlap = width;
        add(100);
      } else
        add(25);
      width = overlap;
      lastX = lo;
      if (count == 16) {
        for (int i = 1; i < 16; i++)
          slabs[i - 1] = slabs[i];
        count = 15;
      }
      slabs[count++] = {lastX, width};
      moving = direction > 0 ? 0 : float(W - width);
      speed = std::min(5.0f, speed + 0.09f);
      nextStage();
      sound(perfect ? Sound::Power : Sound::Hit);
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    slabs.fill({});
    width = 92;
    lastX = 42;
    count = 1;
    slabs[0] = {42, 92};
    moving = 0;
    direction = 1;
    speed = 1.4f + o.difficulty * 0.2f;
    health = 1;
  }
  size_t activeEntities() const override {
    return size_t(count + 1);
  }
  void render(Canvas &c) const override {
    for (int i = 0; i < count; i++)
      c.rect(slabs[i].x, 201 - (count - 1 - i) * 10, slabs[i].width, 8, i % 2 ? Cyan : Pink);
    c.rect(int(moving), 201 - count * 10, width, 8, Yellow);
    c.centered(210, "A: DROP THE MOVING SLAB", White);
  }
};
GameEntry describeStackTower() {
  return gameEntry<StackTower>(
      {"STACK TOWER",
       "BUILD A NEON SKYSCRAPER",
       {"A DROPS THE MOVING SLAB", "ONLY THE OVERLAP REMAINS", "PERFECT DROPS KEEP WIDTH",
        "EACH FLOOR GETS FASTER", "TOWER SCROLLS UPWARD", "MISSING ENDS THE RUN"},
       Pink,
       21,
       Fire});
}
} // namespace arcade
