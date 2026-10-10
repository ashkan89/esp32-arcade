#include "games/Registration.h"
namespace arcade {
class JumpQuest final : public TickGame {
  struct Platform {
    bool active = false;
    float x = 0, y = 0;
    int width = 38;
  };
  Pool<Platform, 12> platforms;
  float x = 88, y = 180, vx = 0, vy = -3;
  void reset() {
    platforms.clear();
    for (int i = 0; i < 8; i++)
      if (auto *p = platforms.acquire()) {
        p->x = float(i == 0 ? 65 : rng.range(4, 134));
        p->y = float(205 - i * 25);
        p->width = 42;
      }
    x = 85;
    y = 190;
    vy = -3.5f;
    vx = 0;
  }
  void simulate() override {
    vx = (int(input.down(Right)) - int(input.down(Left))) * 2.0f;
    x += vx;
    if (x < -5)
      x = W + 5;
    if (x > W + 5)
      x = -5;
    float old = y;
    vy = std::min(4.0f, vy + 0.09f);
    y += vy;
    if (vy > 0)
      for (const auto &p : platforms.items)
        if (p.active && old + 5 <= p.y && y + 5 >= p.y && x + 5 >= p.x && x - 5 <= p.x + p.width) {
          y = p.y - 5;
          vy = -3.5f;
          sound(Sound::Shot);
          break;
        }
    if (y < 105) {
      float shift = 105 - y;
      y = 105;
      add(uint32_t(std::max(1, int(shift))));
      for (auto &p : platforms.items)
        if (p.active)
          p.y += shift;
    }
    for (auto &p : platforms.items)
      if (p.active && p.y > H + 10)
        p.active = false;
    float top = H;
    for (const auto &p : platforms.items)
      if (p.active)
        top = std::min(top, p.y);
    while (top > 25) {
      if (auto *p = platforms.acquire()) {
        p->y = top - 25;
        top = p->y;
        p->x = float(rng.range(3, 135));
        p->width = 38;
      } else
        break;
    }
    level = uint16_t(std::min(999u, 1 + points / 500));
    if (y > H + 10) {
      loseLife();
      if (!ended)
        reset();
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    reset();
  }
  size_t activeEntities() const override {
    return platforms.count() + 1;
  }
  void cleanup() override {
    Game::cleanup();
    platforms.clear();
  }
  void render(Canvas &c) const override {
    for (const auto &p : platforms.items)
      if (p.active)
        c.rect(int(p.x), int(p.y), p.width, 4, Green);
    c.rect(int(x) - 5, int(y) - 5, 10, 10, Yellow);
    c.pixel(int(x) + 2, int(y) - 2, Black);
    c.centered(211, "L/R: STEER THE JUMPER", Grey);
  }
};
GameEntry describeJumpQuest() {
  return gameEntry<JumpQuest>(
      {"JUMP QUEST",
       "CLIMB ABOVE THE CLOUDS",
       {"L/R STEERS IN THE AIR", "LANDING AUTO JUMPS", "SCREEN SIDES WRAP AROUND",
        "CLIMB TO SCROLL THE WORLD", "GAIN POINTS FOR HEIGHT", "THREE FALLS END THE RUN"},
       Yellow,
       22,
       Left | Right});
}
} // namespace arcade
