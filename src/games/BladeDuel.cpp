#include "games/Campaign.h"
namespace arcade {
class BladeDuel final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyBladeDuel();
#endif
  float x = 35, enemyX = 135;
  int stamina = 100, enemyHP = 0, maximum = 0, attack = 0, enemyAttack = 0, windup = 0, cool = 0,
      roundTime = 0;
  int enemyRecover = 0, decision = 90, approach = 29;
  void setupStage() override {
    x = 35;
    enemyX = 135;
    stamina = 100;
    enemyHP = maximum = bossRound() ? 18 : 6 + level;
    attack = enemyAttack = windup = cool = 0;
    enemyRecover = 0;
    decision = 90;
    approach = 29;
    roundTime = hazardInterval(60 * 90);
  }
  void play() override {
    bool guard = input.down(Special);
    x = clamp(x + (int(input.down(Right)) - int(input.down(Left))) * (guard ? .6f : 1.5f), 8.f,
              enemyX - 13);
    if (stamina < 100 && stageTick % 3 == 0)
      ++stamina;
    if (attack)
      --attack;
    if (enemyAttack)
      --enemyAttack;
    if (enemyRecover)
      --enemyRecover;
    if (cool)
      --cool;
    if (input.hit(Fire) && stamina >= 20 && !cool) {
      stamina -= 20;
      attack = 12;
      cool = 25;
      sound(Sound::Shot);
      if (enemyX - x < 37) {
        bool parry = !windup && stageTick % 120 < 30;
        if (!parry) {
          enemyHP -= input.down(Up) ? 2 : 1;
          add(40);
          burst(enemyX, 155);
        } else
          sound(Sound::Hit);
      }
    }
    if (!windup && !enemyAttack && !enemyRecover) {
      enemyX = clamp(enemyX + clamp(x + approach - enemyX, -.6f, .6f), x + 14, 168.f);
      if (--decision <= 0) {
        if (enemyX - x < 55)
          windup = std::max(20, hazardInterval(bossRound() ? 28 : 36));
        decision = hazardInterval(85) + rng.range(0, 50);
        approach = rng.range(0, 3) == 0 ? 45 : 29;
      }
    }
    if (windup && --windup == 0) {
      enemyAttack = 12;
      enemyRecover = 45 + rng.range(0, 30);
      if (enemyX - x < 42) {
        if (guard && stamina >= 15) {
          stamina -= 15;
          add(10);
          sound(Sound::Hit);
          enemyX = std::min(168.f, enemyX + 12);
        } else {
          hit();
          x = std::max(8.f, x - 18);
        }
      }
    }
    if (--roundTime <= 0) {
      grace = 0;
      hit();
      roundTime = hazardInterval(60 * 90);
    }
    if (enemyHP <= 0)
      clearStage();
  }

public:
  bool bossEncounter() const override {
    return bossRound() && enemyHP > 0;
  }
  int bossHealth() const override {
    return bossRound() ? enemyHP : 0;
  }
  size_t activeEntities() const override {
    return sparks.count() + 2;
  }
  void render(Canvas &c) const override {
    c.circle(88, 75, 27, Orange, true);
    c.rect(0, 190, 176, 30, Navy);
    for (int i = 0; i < 4; ++i)
      c.line(i * 55, 190, i * 55 + 22, 220, Grey);
    c.rect(int(x) - 7, 151, 14, 39, Cyan);
    c.circle(int(x), 145, 7, White, true);
    c.rect(int(enemyX) - 7, 151, 14, 39, bossRound() ? Red : Pink);
    c.circle(int(enemyX), 145, 7, Orange, true);
    c.line(int(x) + 6, 160, int(x) + (attack ? 38 : 15), attack ? 158 : 132, White);
    c.line(int(enemyX) - 6, 160, int(enemyX) - (enemyAttack ? 38 : 15), enemyAttack ? 158 : 132,
           Yellow);
    if (input.down(Special))
      c.line(int(x) + 10, 145, int(x) + 10, 177, Green);
    if (windup)
      c.centered(112, "STRIKE INCOMING", Red);
    else if (enemyRecover)
      c.centered(112, "RIVAL RECOVERING", Green);
    c.text(5, 34, "RIVAL", Pink);
    c.rect(42, 34, std::max(0, enemyHP) * 126 / maximum, 5, Red);
    c.text(5, 206, "STAMINA", Grey);
    c.rect(52, 206, stamina, 5, Green);
    if (bossRound())
      bossBar(c, std::max(0, enemyHP), maximum);
    scenery(c, Cyan);
  }
};
GameEntry describeBladeDuel() {
  return gameEntry<BladeDuel>(
      {"BLADE DUEL",
       "READ THE RIVAL AND STRIKE",
       {"L/R APPROACH OR RETREAT", "A SLASH COSTS 20 STAMINA", "UP + A IS A HEAVY SLASH",
        "HOLD B TO GUARD TELEGRAPH", "RIVAL PARRIES WHILE CALM", "DEFEAT 12 RIVALS TO WIN"},
       Cyan,
       42,
       Left | Right | Up | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyBladeDuel() {
  BladeDuel g;
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
  g.enemyHP = 1;
  g.x = 100;
  g.enemyX = 125;
  g.stageTick = 50;
  g.input.pressed = Fire;
  g.play();
  HOST_CHECK(g.enemyHP <= 0 && g.transition > 0);
  g.start({});
  g.x = 100;
  g.enemyX = 125;
  g.windup = 1;
  g.grace = 0;
  g.input.held = Special;
  int lives = g.health;
  g.play();
  HOST_CHECK(g.health == lives && g.stamina == 85);
  return true;
}
#endif
} // namespace arcade
