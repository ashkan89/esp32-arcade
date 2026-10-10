#include "arcade/Console.h"
#include "games/ArcadeGames.h"
#include "games/Shooters.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
using namespace arcade;
static int assertions = 0;
#define CHECK(x)                                                                                   \
  do {                                                                                             \
    assertions++;                                                                                  \
    if (!(x)) {                                                                                    \
      std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                                    \
      std::exit(1);                                                                                \
    }                                                                                              \
  } while (0)
namespace arcade {
struct RuleTests {
  static void shooter() {
    SlyInvaders g;
    g.start({});
    CHECK(g.enemies.count() == 24);
    g.shots.clear();
    g.dual = 100;
    g.fire();
    CHECK(g.shots.count() == 2);
    CHECK(!g.barrier(22 + 8, 168 + 9, 1));
    CHECK(g.barrier(22, 168, 1));
    g.barriers[0].fill(0);
    CHECK(!g.barrier(22, 168, 1));
    g.enemies.clear();
    g.shots.clear();
    auto *a = g.enemies.acquire();
    a->x = 40;
    a->y = 50;
    a->hp = 1;
    auto *b = g.enemies.acquire();
    b->x = 40;
    b->y = 50;
    b->hp = 1;
    auto *bullet = g.shots.acquire();
    bullet->oldX = 45;
    bullet->x = 45;
    bullet->oldY = 80;
    bullet->y = 40;
    g.collisions();
    CHECK(g.enemies.count() == 1);
    CHECK(!bullet->active);
    CHECK(g.score() == 10);
    int lives = g.lives();
    g.hurt();
    g.hurt();
    CHECK(g.lives() == lives - 1);
    g.invincible = 0;
    g.shield = 100;
    g.hurt();
    CHECK(g.lives() == lives - 1);
    CHECK(g.shield == 0);
    g.start({});
    g.rapid = 1;
    g.dual = 1;
    g.shield = 1;
    g.update();
    CHECK(g.rapid == 0 && g.dual == 0 && g.shield == 0);
    g.level = 5;
    g.wave();
    CHECK(g.bossStage);
    CHECK(g.enemies.count() == 1);
    CHECK(g.enemies.items[0].kind == 4);
    g.enemies.clear();
    g.update();
    CHECK(g.clearTimer > 0);
    for (int i = 0; i < 90; i++)
      g.update();
    CHECK(g.stage() == 6);
    CHECK(g.enemies.count() == 24);
    MeteorPatrol m;
    m.start({});
    m.enemies.clear();
    m.spawnRock(70, 60, 2);
    CHECK(m.enemies.count() == 1);
    m.killed(m.enemies.items[0]);
    CHECK(m.enemies.count() == 2);
    for (auto &e : m.enemies.items)
      if (e.active)
        CHECK(e.size == 1);
    while (m.enemies.acquire()) {
    };
    CHECK(m.enemies.count() == 32);
    m.spawnRock(90, 50, 2);
    CHECK(m.enemies.count() == 32);
    SkyFighters sky;
    sky.start({});
    sky.handle({0, Special});
    sky.update();
    CHECK(sky.bombs == 0);
    CHECK(sky.shield > 0);
    sky.cleanup();
    CHECK(sky.activeEntities() == 0);
  }
  static void breaker() {
    NeonBreaker g;
    g.start({});
    g.launched = true;
    g.ballX = g.paddle + 16;
    g.ballY = 194;
    g.vx = 0;
    g.vy = 3;
    g.update();
    CHECK(g.vy < 0);
    CHECK(std::abs(g.vx) >= 0.35f);
    g.bricks.fill(0);
    g.bricks[0] = 1;
    g.ballX = 12;
    g.ballY = 46;
    g.vy = -3;
    g.update();
    CHECK(g.stage() == 2);
    CHECK(!g.launched);
    CHECK(g.score() >= 120);
    g.launched = true;
    g.ballY = 230;
    int lives = g.lives();
    g.update();
    CHECK(g.lives() == lives - 1);
    CHECK(!g.launched);
  }
  static void snake() {
    SnakeRush g;
    g.start({});
    CHECK(g.length == 4);
    CHECK(!g.occupied(g.food, g.length));
    g.food = {10, 3};
    g.step();
    CHECK(g.length == 5);
    CHECK(g.score() == 12);
    g.direction = g.nextDir = 0;
    g.handle({0, Left});
    CHECK(g.nextDir == 3);
    g.handle({0, Right});
    CHECK(g.nextDir == 3);
    // Entering the tail's vacated square is allowed, but growth into it is not.
    g.turnQueued = false;
    g.length = 4;
    g.body[0] = {3, 3};
    g.body[1] = {3, 4};
    g.body[2] = {2, 4};
    g.body[3] = {2, 3};
    g.direction = g.nextDir = 2;
    g.food = {8, 8};
    g.step();
    CHECK(!g.gameOver());
    g.body[0] = {0, 0};
    g.direction = g.nextDir = 2;
    g.step();
    CHECK(g.gameOver());
    CHECK(g.lives() == 0);
    for (uint8_t layout = 0; layout < 3; layout++) {
      GameOptions o;
      o.layout = layout;
      g.start(o);
      for (int i = 0; i < 100; i++) {
        CHECK(g.placeFood());
        CHECK(!g.wall(g.food));
        CHECK(!g.occupied(g.food, g.length));
      }
    }
    g.start({});
    g.length = SnakeRush::Capacity;
    for (int y = 0; y < SnakeRush::Rows; y++)
      for (int x = 0; x < SnakeRush::Cols; x++)
        g.body[unsigned(y * SnakeRush::Cols + x)] = {int8_t(x), int8_t(y)};
    CHECK(!g.placeFood());
    CHECK(g.gameOver());
  }
  static void turbo() {
    TurboDodge g;
    g.start({});
    for (int i = 0; i < 200; i++) {
      g.obstacles.clear();
      g.spawnRow();
      bool occupied[3] = {};
      for (auto &o : g.obstacles.items)
        if (o.active && !o.coin)
          occupied[o.lane] = true;
      CHECK(!occupied[0] || !occupied[1] || !occupied[2]);
    }
    g.obstacles.clear();
    auto *c = g.obstacles.acquire();
    c->lane = 1;
    c->y = 184;
    c->coin = true;
    g.spawn = 100;
    g.update();
    CHECK(g.score() == 50);
    for (int i = 0; i < 2; i++) {
      auto *o = g.obstacles.acquire();
      o->lane = 1;
      o->y = 184;
    }
    g.update();
    CHECK(g.lives() == 2);
    CHECK(!g.gameOver());
  }
  static void flap() {
    FlapEscape g;
    g.start({});
    g.handle({Fire, Fire});
    g.update();
    CHECK(g.vy < 0);
    float vy = g.vy;
    g.handle({Fire, 0});
    g.update();
    CHECK(g.vy > vy);
    CHECK(g.score() == 0);
    auto *gate = g.gates.acquire();
    gate->x = 17;
    gate->gapY = 80;
    g.y = 115;
    g.vy = 0;
    g.update();
    CHECK(g.score() == 1);
    g.update();
    CHECK(g.score() == 1);
    g.y = 10;
    g.update();
    CHECK(g.gameOver());
  }
};
} // namespace arcade
static void core() {
  CHECK(segmentCircle(0, 5, 20, 5, 10, 5, 2));
  CHECK(!segmentCircle(0, 0, 20, 0, 10, 5, 2));
  CHECK(segmentCircle(10, 5, 10, 5, 10, 5, 2));
  CHECK(!circleRect(0, 0, 5, {4, 4, 2, 2}));
  CHECK(circleRect(0, 0, 5, {3, 3, 2, 2}));
  CHECK(elapsed(8, 0xfffffff8u) == 16);
  CHECK(reached(8, 0xfffffff8u));
  CHECK(!reached(0xfffffff8u, 8));
  CHECK(overlaps({0, 0, 10, 10}, {9, 9, 3, 3}));
  CHECK(!overlaps({0, 0, 10, 10}, {10, 0, 3, 3}));
  CHECK(swept(5, 100, 5, 0, {0, 40, 10, 10}, 1));
  CHECK(!swept(20, 100, 20, 0, {0, 40, 10, 10}, 1));
  CHECK(swept(5, 5, 5, 5, {0, 0, 10, 10}));
  CHECK(!swept(20, 5, 20, 5, {0, 0, 10, 10}));
  struct E {
    bool active = false;
  };
  Pool<E, 3> p;
  CHECK(p.acquire());
  CHECK(p.acquire());
  CHECK(p.acquire());
  CHECK(!p.acquire());
  CHECK(p.count() == 3);
  p.items[1].active = false;
  CHECK(p.acquire());
  p.clear();
  CHECK(p.count() == 0);
  StepClock clock;
  clock.reset(0xffff0000u);
  CHECK(clock.advance(0xffff0000u + StepClock::StepUs) == 1);
  CHECK(clock.advance(0xffff0000u + 2 * StepClock::StepUs) == 1);
  CHECK(clock.advance(500000) == 4);
  CHECK(clock.dropped > 0);
  // 30 and 60 render samples both produce exactly sixty fixed simulation steps per second.
  for (unsigned fps : {30u, 60u}) {
    StepClock c;
    c.reset(0);
    unsigned n = 0;
    for (unsigned i = 1; i <= fps; i++)
      n += c.advance(i * 1000020 / fps);
    CHECK(n == 60);
  }
}
static void inputs() {
  Input i;
  CHECK(i.poll(Fire, 100).pressed == 0);
  CHECK(i.poll(0, 105).pressed == 0);
  CHECK(i.poll(Fire, 110).pressed == 0);
  CHECK(i.poll(Fire, 130).hit(Fire));
  CHECK(i.poll(Fire, 131).pressed == 0);
  CHECK(i.poll(Fire, 500).repeated & Fire);
  CHECK(i.poll(Fire, 830).longPress & Fire);
  CHECK(i.poll(Fire, 840).longPress == 0);
  i.suppress();
  CHECK(i.poll(Fire, 950).held == 0);
  i.poll(0, 1000);
  CHECK(i.poll(0, 1020).released == 0);
  i.poll(Fire, 1040);
  CHECK(i.poll(Fire, 1060).hit(Fire));
  Input rollover;
  rollover.poll(Left, 0xfffffff0u);
  CHECK(rollover.poll(Left, 10).hit(Left));
  rollover.poll(0, 20);
  CHECK(rollover.poll(0, 40).released & Left);
  Input pending;
  pending.poll(Fire, 1);
  pending.suppress();
  CHECK(pending.poll(Fire, 25).pressed == 0);
  pending.poll(0, 30);
  pending.poll(0, 50);
  CHECK(pending.poll(Fire, 80).pressed == 0);
  CHECK(pending.poll(Fire, 100).hit(Fire));
}
static void saves() {
  Settings s;
  CHECK(s.valid());
  CHECK(s.record(0, 120));
  CHECK(!s.record(0, 0));
  CHECK(s.record(7, 55));
  CHECK(!s.record(8, 90));
  auto b = encode(s);
  Settings out;
  CHECK(decode(b, out));
  CHECK(out.best == s.best);
  CHECK(out.slyLeaders[0] == 120);
  for (size_t k = 0; k < b.size(); k++) {
    auto bad = b;
    bad[k] ^= 0x80;
    Settings unchanged;
    unchanged.best[0] = 99;
    CHECK(!decode(bad, unchanged));
    CHECK(unchanged.best[0] == 99);
  }
  s.difficulty = 3;
  CHECK(!s.valid());
  CHECK(!decode(encode(s), out));
  s = Settings{};
  s.fps = 45;
  CHECK(!decode(encode(s), out));
  s = Settings{};
  s.slyLeaders = {0, 10, 0, 0, 0};
  CHECK(!decode(encode(s), out));
}
static void consoleRules() {
  Settings s;
  Console c(s);
  c.begin({}, 0);
  CHECK(c.consumeSuppression());
  c.update({Fire, Fire}, 21);
  CHECK(c.state() == Screen::Launcher);
  CHECK(c.consumeSuppression());
  c.update({0, Right}, 40);
  CHECK(c.selected() == 1);
  c.update({0, Fire}, 60);
  CHECK(c.state() == Screen::Instructions);
  c.update({0, Fire}, 80);
  CHECK(c.state() == Screen::Playing);
  CHECK(c.active());
  c.update({0, Back}, 100);
  CHECK(c.state() == Screen::Paused);
  auto score = c.active()->score();
  for (int i = 0; i < 100; i++)
    c.update({}, 120 + unsigned(i));
  CHECK(c.active()->score() == score);
  c.update({0, Right}, 240);
  c.update({0, Right}, 260);
  c.update({0, Fire}, 280);
  CHECK(c.state() == Screen::Instructions);
  c.update({0, Fire}, 300);
  CHECK(c.state() == Screen::Paused);
  c.update({0, Right}, 320);
  c.update({0, Right}, 340);
  c.update({0, Right}, 360);
  c.update({0, Fire}, 380);
  CHECK(c.state() == Screen::Launcher);
  CHECK(!c.active());
  GameOptions one;
  one.oneButton = true;
  s.lastGame = 0;
  c.begin(one, 0);
  c.update({Fire, Fire}, 20);
  CHECK(c.state() == Screen::Launcher);
  for (int k = 0; k < 8; k++)
    c.update({0, 0, Fire}, 100 + unsigned(k) * 40);
  CHECK(c.state() == Screen::Utilities);
  c.update({0, 0, 0, 0, Fire}, 500);
  CHECK(c.state() == Screen::Settings);
  CHECK(s.sound == 1);
  c.update({0, 0, 0, 0, Fire}, 530);
  CHECK(s.sound == 0);
  CHECK(c.consumeDirty());
}
static void lifecycle() {
  std::array<uint16_t, W * H + 2> pixels{};
  pixels.front() = 0xdead;
  pixels.back() = 0xbeef;
  Canvas canvas;
  canvas.attach(pixels.data() + 1);
  Random random;
  random.state = 123;
  unsigned overs = 0;
  for (unsigned id = 0; id < 8; id++) {
    CHECK(registry()[id].title[0]);
    for (bool one : {false, true}) {
      Game &g = gameFor(GameId(id));
      GameOptions o;
      o.oneButton = one;
      o.seed = 100 + id;
      g.initialize();
      g.start(o);
      CHECK(!g.gameOver());
      CHECK(g.score() == 0);
      uint32_t score = g.score();
      g.pause();
      for (int j = 0; j < 20; j++) {
        g.handle({Fire, Fire});
        g.update();
      }
      CHECK(g.score() == score);
      g.resume();
      for (unsigned frame = 0; frame < 10000; frame++) {
        uint16_t raw = uint16_t(random.next() & 255);
        g.handle({raw, uint16_t(raw & Fire), 0, 0, 0});
        g.update();
        CHECK(g.activeEntities() < 500);
        if (frame % 30 == 0) {
          canvas.clear();
          g.render(canvas);
          CHECK(pixels.front() == 0xdead);
          CHECK(pixels.back() == 0xbeef);
        }
        if (g.gameOver()) {
          overs++;
          g.start(o);
        }
      }
      g.cleanup();
      CHECK(g.gameOver());
      g.start(o);
      CHECK(g.score() == 0);
      CHECK(!g.gameOver());
      g.cleanup();
    }
  }
  CHECK(overs > 8);
  // RGB332 fallback clipping must not overwrite guard bytes.
  std::array<uint8_t, W * H + 2> small{};
  small.front() = 17;
  small.back() = 19;
  canvas.attach(small.data() + 1, false);
  canvas.clear(Cyan);
  canvas.rect(-10, -10, 30, 30, Red);
  canvas.circle(W - 1, H - 1, 12, White, true);
  canvas.text(160, 215, "CLIPPED", White, 2);
  CHECK(small.front() == 17 && small.back() == 19);
  std::printf("Lifecycle soak: 160000 simulation ticks, %u game-over/restart cycles\n", overs);
}
static void preview() {
  std::array<uint16_t, W * H> pixels{};
  Canvas c;
  c.attach(pixels.data());
  auto write = [&](int index) {
    char name[64];
    std::snprintf(name, sizeof(name), "test-results/preview_%02d.ppm", index);
    FILE *f = std::fopen(name, "wb");
    CHECK(f);
    std::fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (uint16_t p : pixels) {
      uint8_t bytes[3] = {uint8_t(((p >> 11) & 31) * 255 / 31), uint8_t(((p >> 5) & 63) * 255 / 63),
                          uint8_t((p & 31) * 255 / 31)};
      if (std::fwrite(bytes, 1, 3, f) != 3)
        CHECK(false);
    }
    std::fclose(f);
  };
  Settings settings;
  Console console(settings);
  console.begin({}, 0);
  console.update({0, Fire}, 25);
  c.clear();
  console.render(c);
  write(0);
  for (unsigned i = 0; i < 8; i++) {
    Game &g = gameFor(GameId(i));
    g.start({});
    for (int j = 0; j < 110; j++) {
      g.handle({Fire, uint16_t(j % (i == 7 ? 60 : 28) == 0 ? Fire : 0)});
      g.update();
      if (g.gameOver())
        g.start({});
    }
    c.clear();
    g.render(c);
    c.rect(0, 0, W, Top, Navy);
    c.centered(5, registry()[i].title, registry()[i].accent);
    write(int(i) + 1);
    g.cleanup();
  }
  console.update({0, Fire}, 30);
  console.update({0, Fire}, 50);
  console.update({0, Back}, 70);
  c.clear();
  console.render(c);
  write(9);
  console.update({0, Right}, 90);
  console.update({0, Right}, 110);
  console.update({0, Right}, 130);
  console.update({0, Fire}, 150);
  console.update({0, Special}, 170);
  console.update({0, Fire}, 190);
  c.clear();
  console.render(c);
  write(10);
}
int main() {
  core();
  inputs();
  saves();
  consoleRules();
  RuleTests::shooter();
  RuleTests::breaker();
  RuleTests::snake();
  RuleTests::turbo();
  RuleTests::flap();
  lifecycle();
  preview();
  std::printf(
      "PASS %d assertions across core, inputs, saves, menus, game rules and lifecycle soak\n",
      assertions);
}
