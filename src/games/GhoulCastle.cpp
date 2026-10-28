#include "assets/Art.h"
#include "games/Campaign.h"
namespace arcade {
class GhoulCastle final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyGhoulCastle();
#endif
  struct Ghoul {
    bool active = false;
    float x = 0, y = 0;
    int hp = 1;
  };
  Pool<Ghoul, 10> ghouls;
  Pool<Projectile, 24> spears, hostile;
  float x = 15, y = 190, vy = 0;
  int facing = 1, cool = 0, armor = 1, hp = 0, maximum = 0;
  float bx = 140, by = 80;
  int jumpBuffer = 0, coyote = 0;
  bool ground(float xx) const {
    return xx < 58 || xx > 88;
  }
  void setupStage() override {
    ghouls.clear();
    spears.clear();
    hostile.clear();
    x = 15;
    y = 190;
    vy = 0;
    jumpBuffer = coyote = 0;
    facing = 1;
    cool = 0;
    armor = 1;
    hp = maximum = bossRound() ? 25 + level : 0;
    for (int i = 0; i < std::min(10, 3 + int(level) / 2); ++i)
      if (auto *e = ghouls.acquire()) {
        e->x = 100 + i % 4 * 18;
        e->y = 190;
        e->hp = 1 + level / 6;
      }
  }
  void play() override {
    int dx = int(input.down(Right)) - int(input.down(Left));
    if (dx)
      facing = dx;
    if (ground(x) && y == 190 && vy == 0)
      coyote = 6;
    else if (coyote)
      --coyote;
    if (input.hit(Special))
      jumpBuffer = 6;
    else if (jumpBuffer)
      --jumpBuffer;
    x = clamp(x + dx * 1.6f, 6.f, 170.f);
    if (jumpBuffer && coyote) {
      jumpBuffer = coyote = 0;
      vy = -4.4f;
      sound(Sound::Move);
    }
    float old = y;
    y += vy;
    vy = std::min(4.5f, vy + .14f);
    if (ground(x) && old <= 190 && y >= 190) {
      y = 190;
      vy = 0;
    }
    if (y > 225) {
      grace = 0;
      hit();
      x = 15;
      y = 190;
      vy = 0;
    }
    if (cool)
      --cool;
    if (input.down(Fire) && !cool) {
      projectile(spears, x + facing * 9, y, facing * 3.4f, 0);
      cool = 16;
      sound(Sound::Shot);
    }
    for (auto &e : ghouls.items)
      if (e.active) {
        float nx = e.x + clamp(x - e.x, -.35f, .35f);
        if (ground(nx))
          e.x = nx;
        if (near(x, y, e.x, e.y, 11) && !grace) {
          if (armor) {
            armor = 0;
            grace = 90;
            sound(Sound::Hit);
          } else
            hit();
        }
      }
    if (hp) {
      bx = 140 + std::sin(stageTick * .02f) * 18;
      by = 90 + std::sin(stageTick * .012f) * 40;
      if (stageTick % unsigned(hazardInterval(hp < maximum / 2 ? 45 : 80)) == 0) {
        float a = std::atan2(y - by, x - bx);
        for (int k = -1; k <= 1; ++k)
          projectile(hostile, bx, by, std::cos(a + k * .22f) * 1.8f, std::sin(a + k * .22f) * 1.8f);
      }
    }
    moveProjectiles(spears);
    moveProjectiles(hostile);
    for (auto &p : spears.items)
      if (p.active) {
        if (hp && projectileNear(p, bx, by, 20)) {
          p.active = false;
          --hp;
        } else
          for (auto &e : ghouls.items)
            if (e.active && projectileNear(p, e.x, e.y, 9)) {
              p.active = false;
              if (--e.hp == 0) {
                e.active = false;
                add(100);
                burst(e.x, e.y);
              }
              break;
            }
      }
    for (auto &p : hostile.items)
      if (p.active && near(x, y, p.x, p.y, 8)) {
        p.active = false;
        if (armor && !grace) {
          armor = 0;
          grace = 90;
        } else
          hit();
      }
    if (!ghouls.count() && !hp && x > 160 && y == 190)
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
    return ghouls.count() + spears.count() + hostile.count() + sparks.count() + 1;
  }
  void render(Canvas &c) const override {
    art::scene(c, 40);
    // Moonlit castle artwork supplies the distant scenery.
    for (int i = 0; i < 5; ++i) {
      c.rect(i * 40 + 5, 143, 18, 57, Navy);
      c.rect(i * 40 + 3, 139, 22, 5, Grey);
    }
    art::platform(c, 0, 200, 58, 20);
    art::platform(c, 89, 200, 87, 20);
    c.rect(158, 170, 16, 30, ghouls.count() || hp ? Red : Green, false);
    for (const auto &e : ghouls.items)
      if (e.active) {
        art::object(c, art::Sprite::Zombie, int(e.x), int(e.y), 15, 22);
      }
    if (!grace || tick % 10 < 5) {
      art::object(c, tick / 8 % 2 ? art::Sprite::Hero : art::Sprite::HeroStep, int(x), int(y), 15, 22);
    }
    if (hp) {
      art::object(c, art::Sprite::Demon, int(bx), int(by), 38, 38);
      c.line(int(bx) - 26, int(by) - 15, int(bx), int(by) + 4, Pink);
      c.line(int(bx) + 26, int(by) - 15, int(bx), int(by) + 4, Pink);
      bossBar(c, hp, maximum);
    }
    drawProjectiles(c, spears, White);
    drawProjectiles(c, hostile, Orange);
    scenery(c, Pink);
    c.centered(211, "A SPEAR  B JUMP", Grey);
  }
};
GameEntry describeGhoulCastle() {
  return gameEntry<GhoulCastle>(
      {"GHOUL CASTLE",
       "SPEARS ARMOR AND UNDEAD",
       {"L/R WALK  B JUMPS THE GAP", "HOLD A TO THROW SPEARS", "ARMOR BLOCKS ONE HIT",
        "BOSS: JUMP AND HIT DEMON", "CLEAR FOES THEN RIGHT DOOR", "ESCAPE 12 CASTLE ROOMS"},
       Pink,
       40,
       Left | Right | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyGhoulCastle() {
  GhoulCastle g;
  g.start({});
  // Regression checks for the gameplay audit.
  g.x = 57;
  g.y = 190;
  g.vy = 0;
  g.input = {Right | Special, Special};
  g.play();
  HOST_CHECK(g.x > 58 && g.vy < 0);
  g.input = {};
  g.x = 30;
  g.y = 190;
  g.vy = 0;
  g.jumpBuffer = 4;
  g.play();
  HOST_CHECK(g.vy < 0 && g.jumpBuffer == 0);
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
  g.ghouls.clear();
  g.hp = 1;
  g.x = 165;
  g.y = 190;
  g.stageTick = 1;
  projectile(g.spears, 140, 90, 0, 0);
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  return true;
}
#endif
} // namespace arcade
