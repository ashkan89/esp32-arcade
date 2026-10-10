#include "games/Registration.h"
namespace arcade {
class LunarLander final : public TickGame {
  float x = 88, y = 50, vx = 0, vy = 0, fuel = 100;
  int pad = 60, wait = 0;
  bool landed = false;
  void reset() {
    x = 88;
    y = 45;
    vx = 0.15f * (rng.range(-2, 3));
    vy = 0;
    fuel = 100;
    pad = rng.range(15, 111);
    landed = false;
  }
  void simulate() override {
    if (wait) {
      if (--wait == 0)
        reset();
      return;
    }
    vx += (int(input.down(Right)) - int(input.down(Left))) * 0.014f;
    vy += 0.013f + options.difficulty * 0.002f;
    if (input.down(Fire) && fuel > 0) {
      vy -= 0.031f;
      fuel = std::max(0.0f, fuel - 0.18f);
    }
    vx = clamp(vx, -1.5f, 1.5f);
    vy = clamp(vy, -1.7f, 2.0f);
    x += vx;
    y += vy;
    if (x < 7 || x > W - 7) {
      x = clamp(x, 7.0f, float(W - 7));
      vx = -vx * 0.5f;
    }
    if (y < 28) {
      y = 28;
      vy = std::max(0.0f, vy);
    }
    if (y + 7 >= 199) {
      if (x - 6 >= pad && x + 6 <= pad + 44 && std::abs(vx) < 0.45f && vy < 0.65f) {
        landed = true;
        add(200 + uint32_t(fuel) * 3);
        nextStage();
        sound(Sound::Power);
      } else
        loseLife();
      y = 192;
      wait = 90;
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    wait = 0;
    reset();
  }
  size_t activeEntities() const override {
    return 2;
  }
  void render(Canvas &c) const override {
    for (int i = 0; i < 24; i++)
      c.pixel(i * 47 % W, 28 + i * 29 % 140, Grey);
    c.line(0, 200, W - 1, 200, Grey);
    c.rect(pad, 199, 44, 4, Green);
    c.rect(int(x) - 4, int(y) - 5, 8, 9, White);
    c.line(int(x) - 7, int(y) + 6, int(x) - 3, int(y) + 2, White);
    c.line(int(x) + 7, int(y) + 6, int(x) + 3, int(y) + 2, White);
    if (input.down(Fire) && fuel > 0 && !wait)
      c.line(int(x), int(y) + 5, int(x), int(y) + 12, Orange);
    c.text(5, 25, "FUEL", Grey);
    c.rect(35, 25, int(fuel), 4, Cyan);
    if (wait)
      c.centered(95, landed ? "SAFE LANDING!" : "HARD LANDING", landed ? Green : Red);
    c.text(5, 211, "V SPEED", Grey);
    c.number(57, 211, uint32_t(std::abs(vy) * 100), vy < 0.65f ? Green : Red);
  }
};
GameEntry describeLunarLander() {
  return gameEntry<LunarLander>(
      {"LUNAR LANDER",
       "TOUCH DOWN WITH CARE",
       {"L/R ADJUST SIDE THRUST", "HOLD A FOR MAIN ENGINE", "LAND INSIDE GREEN PAD",
        "SIDE SPEED BELOW 45", "VERTICAL SPEED BELOW 65", "FUEL SAVED GIVES BONUS"},
       White,
       14,
       Left | Right | Fire});
}
} // namespace arcade
