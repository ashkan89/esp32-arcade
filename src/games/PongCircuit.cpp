#include "games/Registration.h"
namespace arcade {
class PongCircuit final : public TickGame {
  float x = 88, y = 110, vx = 2, vy = 1.2f, player = 90, ai = 90;
  int serves = 0;
  void serve() {
    x = 88;
    y = 110;
    vx = (serves++ % 2 ? 1 : -1) * (2.0f + std::min(1.5f, level * 0.1f));
    vy = rng.range(-15, 16) * 0.1f;
  }
  void simulate() override {
    player = clamp(player + (int(input.down(Down)) - int(input.down(Up))) * 3.2f, 24.0f, 178.0f);
    ai = clamp(ai + clamp(y - (ai + 18), -1.3f - options.difficulty * 0.3f,
                          1.3f + options.difficulty * 0.3f),
               24.0f, 178.0f);
    float old = x;
    x += vx;
    y += vy;
    if (y < 24 || y > 216) {
      y = clamp(y, 24.0f, 216.0f);
      vy = -vy;
    }
    if (vx < 0 && old >= 15 && x <= 15 && y + 3 >= player && y - 3 <= player + 36) {
      x = 15;
      vx = std::abs(vx);
      vy = (y - player - 18) * 0.13f;
      sound(Sound::Hit);
    }
    if (vx > 0 && old <= 161 && x >= 161 && y + 3 >= ai && y - 3 <= ai + 36) {
      x = 161;
      vx = -std::abs(vx);
      vy = (y - ai - 18) * 0.13f;
    }
    if (x > 180) {
      add(100);
      if (points % 500 == 0)
        nextStage();
      serve();
    } else if (x < -4) {
      loseLife();
      serve();
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    player = ai = 90;
    serves = 0;
    serve();
  }
  size_t activeEntities() const override {
    return 3;
  }
  void render(Canvas &c) const override {
    for (int yy = 24; yy < H; yy += 12)
      c.rect(87, yy, 2, 6, Grey);
    c.rect(8, int(player), 4, 36, Cyan);
    c.rect(164, int(ai), 4, 36, Pink);
    c.circle(int(x), int(y), 3, White, true);
  }
};
GameEntry describePongCircuit() {
  return gameEntry<PongCircuit>(
      {"PONG CIRCUIT",
       "OUTPLAY THE RIVAL",
       {"UP/DOWN MOVE PADDLE", "EDGE HITS CHANGE ANGLE", "SCORE 100 PER GOAL",
        "FIVE GOALS RAISE SPEED", "THREE MISSES END RUN", "MENU PAUSES THE MATCH"},
       Cyan,
       8,
       Up | Down});
}
} // namespace arcade
