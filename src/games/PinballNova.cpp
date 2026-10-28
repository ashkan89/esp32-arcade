#include "assets/Art.h"
#include "games/Campaign.h"
namespace arcade {
class PinballNova final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyPinballNova();
#endif
  float x = 160, y = 193, vx = 0, vy = 0;
  bool served = false;
  int energy = 0, hp = 0, maximum = 0, nudge = 0;
  std::array<int, 5> cooldown{};
  static int bx(int i) {
    return 36 + (i % 3) * 52;
  }
  static int by(int i) {
    return 70 + (i / 3) * 48;
  }
  void setupStage() override {
    x = 160;
    y = 193;
    vx = vy = 0;
    served = false;
    energy = 0;
    nudge = 0;
    cooldown.fill(0);
    hp = maximum = bossRound() ? 12 + level / 2 : 0;
  }
  void play() override {
    if (nudge)
      --nudge;
    for (auto &t : cooldown)
      if (t)
        --t;
    if (!served) {
      if (input.hit(Fire)) {
        served = true;
        vx = -1.2f;
        vy = -5;
        sound(Sound::Shot);
      }
      return;
    }
    if (input.hit(Special) && !nudge) {
      vy -= 1.4f;
      vx += x < 88 ? .5f : -.5f;
      nudge = 180;
    }
    // Four substeps prevent the ball tunneling through flippers or bumpers.
    for (int step = 0; step < 4; ++step) {
      float oldY = y;
      vy = clamp(vy + hazardSpeed(.025f), -5.5f, 5.5f);
      x += vx / 4;
      y += vy / 4;
      if (x < 8) {
        x = 8;
        vx = std::abs(vx);
      }
      if (x > 168) {
        x = 168;
        vx = -std::abs(vx);
      }
      if (y < 38) {
        y = 38;
        vy = std::abs(vy);
      }
      for (int i = 0; i < 5; ++i)
        if (near(x, y, float(bx(i)), float(by(i)), 14)) {
          float dx = x - bx(i), dy = y - by(i);
          if (dx * dx + dy * dy < .01f) {
            dx = 0;
            dy = -1;
          }
          float d = std::sqrt(dx * dx + dy * dy);
          x = bx(i) + dx / d * 14;
          y = by(i) + dy / d * 14;
          vx = dx / d * 3.5f;
          vy = dy / d * 3.5f;
          if (!cooldown[i]) {
            cooldown[i] = 10;
            ++energy;
            add(50);
            sound(Sound::Hit);
            if (hp && i == 1) {
              --hp;
              burst(x, y);
            }
          }
        }
      if (vy > 0 && oldY <= 185 && y >= 185 &&
          ((x >= 24 && x <= 78 && input.down(Left)) ||
           (x >= 98 && x <= 152 && input.down(Right)))) {
        y = 184;
        vy = -4.8f;
        vx = x < 88 ? 1.4f : -1.4f;
        sound(Sound::Move);
      }
      if (y >= 211) {
        grace = 0;
        hit();
        served = false;
        x = 160;
        y = 193;
        vx = vy = 0;
        break;
      }
    }
    if ((!bossRound() && energy >= 24) || (bossRound() && hp == 0))
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return hp > 0;
  }
  int bossHealth() const override {
    return hp;
  }
  size_t activeEntities() const override {
    return sparks.count() + 6;
  }
  void render(Canvas &c) const override {
    art::scene(c, 48);
    art::scene(c, 53, 12, 40, 152, 157, 65);
    c.rect(4, 34, 168, 179, Navy, false);
    for (int i = 0; i < 5; ++i) {
      art::object(c, cooldown[i] ? art::Sprite::Amber : art::Sprite::Amethyst, bx(i), by(i), 23, 23);
      c.circle(bx(i), by(i), 5, White);
    }
    c.line(10, 152, 25, 178, Cyan);
    c.line(166, 152, 151, 178, Cyan);
    c.line(24, 190, 78, input.down(Left) ? 181 : 199, Green);
    c.line(152, 190, 98, input.down(Right) ? 181 : 199, Green);
    art::object(c, art::Sprite::Pinball, int(x), int(y), 7, 7);
    if (!served)
      c.centered(145, "A LAUNCH", Yellow);
    if (hp)
      bossBar(c, hp, maximum);
    else {
      c.text(5, 24, "BUMPER HITS", Grey);
      c.number(82, 24, energy, Yellow);
      c.text(101, 24, "/24", Grey);
    }
    if (transition) {
      c.rect(15, 100, 146, 24, Navy);
      c.centered(108, "TABLE COMPLETE", Green);
    }
    c.centered(213, "L/R FLIP  B NUDGE", Grey);
  }
};
GameEntry describePinballNova() {
  return gameEntry<PinballNova>(
      {"PINBALL NOVA",
       "FLIPPERS BUMPERS AND NOVA",
       {"A LAUNCHES A NEW BALL", "HOLD L/R FOR THE FLIPPERS", "B NUDGES EVERY THREE SEC",
        "24 BUMPER HITS CLEAR TABLE", "BOSS: HIT RED TOP BUMPER", "CLEAR ALL 12 NEON TABLES"},
       Pink,
       48,
       Left | Right | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyPinballNova() {
  PinballNova g;
  g.start({});
  // Regression checks for the gameplay audit.
  g.served = true;
  g.x = float(g.bx(0));
  g.y = float(g.by(0));
  g.vx = g.vy = 0;
  g.play();
  HOST_CHECK(std::isfinite(g.x) && std::isfinite(g.y));
  HOST_CHECK(std::abs(g.vx) + std::abs(g.vy) > 1);
  g.start({});
  for (int stage = 1; stage <= 12; ++stage) {
    HOST_CHECK(g.stage() == stage && !g.gameOver());
    HOST_CHECK(g.activeEntities() < 500);
    g.clearStage();
    g.pause();
    for (int i = 0; i < 100; ++i)
      g.update();
    HOST_CHECK(g.transition == 90);
    g.resume();
    for (int i = 0; i < 90; ++i)
      g.update();
  }
  HOST_CHECK(g.campaignComplete() && g.stage() == 12);
  auto finalScore = g.score();
  for (int i = 0; i < 100; ++i)
    g.update();
  HOST_CHECK(g.score() == finalScore);
  g.start({});
  HOST_CHECK(!g.gameOver() && !g.campaignComplete() && g.score() == 0);
  g.level = 4;
  g.setupStage();
  g.stageTick = 1;
  HOST_CHECK(g.bossHealth() > 0);
  g.hp = 1;
  g.served = true;
  g.x = 88;
  g.y = 70;
  g.vx = 0;
  g.vy = -1;
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
