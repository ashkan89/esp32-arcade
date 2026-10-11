#include "games/Campaign.h"
namespace arcade {
class CityShield final : public CampaignGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyCityShield();
#endif
  struct Blast {
    bool active = false;
    float x = 0, y = 0;
    int age = 0;
  };
  Pool<Projectile, 32> missiles;
  Pool<Blast, 12> blasts;
  Pool<Projectile, 12> rockets;
  std::array<bool, 6> cities{};
  float x = 88, y = 100;
  int spawned = 0, cool = 0, ammo = 0, hp = 0, maximum = 0;
  float bx = 88;
  int quota() const {
    return 12 + int(level) * 2;
  }
  void setupStage() override {
    missiles.clear();
    blasts.clear();
    rockets.clear();
    cities.fill(true);
    health = 6;
    spawned = cool = 0;
    ammo = 30 + level * 2;
    x = 88;
    y = 100;
    hp = maximum = bossRound() ? 8 + level : 0;
  }
  void play() override {
    x = clamp(x + (int(input.down(Right)) - int(input.down(Left))) * 2, 3.f, 173.f);
    y = clamp(y + (int(input.down(Down)) - int(input.down(Up))) * 2, 42.f, 185.f);
    if (cool)
      --cool;
    if (input.hit(Fire) && !cool && ammo) {
      if (auto *p = rockets.acquire()) {
        p->x = 88;
        p->y = 205;
        p->vx = x;
        p->vy = y;
        --ammo;
        cool = 8;
        sound(Sound::Shot);
      }
    }
    if (input.hit(Special) && ammo >= 3) {
      if (auto *b = blasts.acquire()) {
        b->x = x;
        b->y = y;
        ammo -= 3;
        sound(Sound::Power);
      }
    }
    for (auto &p : rockets.items)
      if (p.active) {
        float dx = p.vx - p.x, dy = p.vy - p.y, d = std::sqrt(dx * dx + dy * dy);
        if (d < 4) {
          p.active = false;
          if (auto *b = blasts.acquire()) {
            b->x = p.vx;
            b->y = p.vy;
          }
        } else {
          p.x += dx / d * 4;
          p.y += dy / d * 4;
        }
      }
    int interval = std::max(18, 65 - int(level) * 3 - challenge() * 5);
    if (stageTick % unsigned(interval) == 0 && (hp || spawned < quota())) {
      float xx = hp ? bx : float(rng.range(5, 171));
      int target = rng.range(0, 6);
      float dx = (15 + target * 29 - xx) / 180;
      projectile(missiles, xx, hp ? 52 : 33, dx * (.65f + level * .035f), .65f + level * .035f);
      ++spawned;
    }
    if (hp)
      bx = 88 + std::sin(stageTick * .02f) * 60;
    for (auto &b : blasts.items)
      if (b.active) {
        ++b.age;
        int radius = b.age < 25 ? b.age : std::max(0, 50 - b.age);
        for (auto &p : missiles.items)
          if (p.active && near(p.x, p.y, b.x, b.y, float(radius))) {
            p.active = false;
            add(50);
          }
        if (hp && b.age == 25 && near(b.x, b.y, bx, 45, float(radius + 14))) {
          if (--hp == 0) {
            missiles.clear();
            clearStage();
          }
          burst(bx, 45);
        }
        if (b.age >= 50)
          b.active = false;
      }
    for (auto &p : missiles.items)
      if (p.active) {
        p.x += p.vx;
        p.y += p.vy;
        if (p.y >= 201) {
          p.active = false;
          int city = clamp(int((p.x + 1) / 29), 0, 5);
          cities[city] = false;
          burst(p.x, 201);
        }
      }
    int alive = 0;
    for (bool b : cities)
      alive += b;
    health = alive;
    if (!alive) {
      ended = true;
      sound(Sound::Over);
      return;
    }
    if (!bossRound() && spawned >= quota() && !missiles.count()) {
      add(alive * 200);
      clearStage();
    }
    // Empty batteries regenerate one round each second, so no unwinnable boss soft lock.
    if (stageTick % 60 == 0 && ammo < 6)
      ++ammo;
  }

public:
  bool bossEncounter() const override {
    return hp > 0;
  }
  int bossHealth() const override {
    return hp;
  }
  size_t activeEntities() const override {
    return missiles.count() + rockets.count() + blasts.count() + sparks.count() + 6;
  }
  void render(Canvas &c) const override {
    for (int i = 0; i < 6; ++i)
      if (cities[i]) {
        c.rect(7 + i * 29, 198, 17, 13, Cyan);
        c.rect(10 + i * 29, 190, 7, 8, Green);
      }
    c.line(0, 212, 175, 212, Grey);
    c.line(int(x) - 5, int(y), int(x) + 5, int(y), White);
    c.line(int(x), int(y) - 5, int(x), int(y) + 5, White);
    for (const auto &p : missiles.items)
      if (p.active)
        c.line(int(p.x), int(p.y), int(p.x - p.vx * 15), int(p.y - p.vy * 15), Red);
    for (const auto &b : blasts.items)
      if (b.active)
        c.circle(int(b.x), int(b.y), b.age < 25 ? b.age : std::max(0, 50 - b.age),
                 b.age % 4 ? Orange : Yellow);
    drawProjectiles(c, rockets, Green);
    if (hp) {
      c.rect(int(bx) - 16, 37, 32, 16, Pink);
      bossBar(c, hp, maximum);
    }
    scenery(c, Cyan);
    c.number(140, 212, uint32_t(ammo), Yellow);
  }
};
GameEntry describeCityShield() {
  return gameEntry<CityShield>(
      {"CITY SHIELD",
       "SAVE SIX NEON CITIES",
       {"D-PAD MOVES TARGET CURSOR", "A FIRES AN INTERCEPTOR", "B SPENDS 3 ON INSTANT BLAST",
        "EXPLOSIONS STOP MISSILES", "BOSS: BLAST THE BOMBER", "SAVE CITIES FOR 12 WAVES"},
       Cyan,
       27,
       Left | Right | Up | Down | Fire | Special});
}

#ifdef ARCADE_HOST_TESTS
bool verifyCityShield() {
  CityShield g;
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
  g.stageTick = 1;
  g.bx = 88;
  auto *b = g.blasts.acquire();
  b->x = 88;
  b->y = 45;
  b->age = 24;
  g.play();
  HOST_CHECK(g.hp == 0 && g.transition > 0);
  g.start({});
  g.ammo = 0;
  g.stageTick = 60;
  g.play();
  HOST_CHECK(g.ammo == 1);
  return true;
}
#endif
} // namespace arcade
