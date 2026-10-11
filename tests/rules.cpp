#include "arcade/Console.h"
#include "arcade/HttpJson.h"
#include "arcade/ReleasePolicy.h"
#include "arcade/SetupKey.h"
#include "assets/Visuals.h"
#include "games/FlapEscape.h"
#include "games/MeteorPatrol.h"
#include "games/NeonBreaker.h"
#include "games/PuzzleRules.h"
#include "games/SkyFighters.h"
#include "games/SkyInvaders.h"
#include "games/SnakeRush.h"
#include "games/TurboDodge.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
namespace arcade {
unsigned campaignAssertions = 0;
void verifyImprovements();
bool verifyVectorDrift();
bool verifyBugBlaster();
bool verifyCityShield();
bool verifyPrismRider();
bool verifyNeonClaim();
bool verifyDotChase();
bool verifyTunnelDigger();
bool verifyBarrelClimb();
bool verifySkyJoust();
bool verifyBurgerWorks();
bool verifyBubbleQuest();
bool verifySnowSiege();
bool verifyCanyonStrike();
bool verifySolarLance();
bool verifyFortressRun();
bool verifyGhoulCastle();
bool verifyIronFront();
bool verifyBladeDuel();
bool verifyElevatorHeist();
bool verifyAlpineRush();
bool verifyWildRails();
bool verifyCrystalCourier();
bool verifyNightRacer();
bool verifyPinballNova();
bool verifyRescueWing();
bool verifyCargoShift();
bool verifyPongCircuit();
bool verifyBlockCascade();
bool verifyMineGrid();
bool verifyMazeRunner();
bool verifyRiverLeap();
bool verifyOrbitGuard();
bool verifyLunarLander();
bool verifyBlastGrid();
bool verifyTankArena();
bool verifyBubbleBurst();
bool verifyGridMerge();
bool verifyMemoryMatch();
bool verifyRobotWhack();
bool verifyStackTower();
bool verifyJumpQuest();
bool verifyColorCascade();
bool verifyKeystoneKapers();
bool verifyRiverRaid();
} // namespace arcade
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
  static void difficulty() {
    int lastFire = 1000, lastGrace = 1000, lastSnake = 1000;
    float lastRock = 0, lastRoad = 0;
    for (uint8_t mode = 0; mode < DifficultyCount; ++mode) {
      GameOptions o;
      o.difficulty = mode;
      SkyInvaders g;
      g.start(o);
      CHECK(g.lives() == (mode < 2 ? 3 : mode == 2 ? 2 : 1));
      g.formationFire();
      CHECK(g.shotTimer < lastFire);
      lastFire = g.shotTimer;
      g.hurt();
      CHECK(g.invincible < lastGrace);
      lastGrace = g.invincible;
      g.enemies.clear();
      g.spawnRock(80, 40, 2, 0);
      CHECK(g.enemies.items[0].vy > lastRock);
      lastRock = g.enemies.items[0].vy;
      SnakeRush snake;
      snake.start(o);
      auto head = snake.body[0];
      int steps = 0;
      do {
        snake.update();
        ++steps;
      } while (snake.body[0] == head && steps < 60);
      CHECK(steps < lastSnake);
      lastSnake = steps;
      TurboDodge road;
      road.start(o);
      road.obstacles.clear();
      auto *obstacle = road.obstacles.acquire();
      obstacle->y = 40;
      road.spawn = 100;
      road.update();
      CHECK(obstacle->y - 40 > lastRoad);
      lastRoad = obstacle->y - 40;
    }
    SkyInvaders invalid;
    GameOptions o;
    o.difficulty = 255;
    invalid.start(o);
    CHECK(invalid.options.difficulty == 3 && invalid.lives() == 1);
  }
  static void prepareFinalBoss(Game &game) {
    auto &g = static_cast<Shooter &>(game);
    g.level = 12;
    g.wave();
  }
  static void finishFinalBoss(Game &game) {
    auto &g = static_cast<Shooter &>(game);
    g.enemies.clear();
    g.hostile.clear();
    g.clearTimer = 1;
    g.update();
    CHECK(g.campaignComplete());
  }
  static void endings() {
    for (unsigned id = 0; id < 4; ++id) {
      auto &g = static_cast<Shooter &>(gameFor(GameId(id)));
      g.start({});
      g.level = 12;
      g.wave();
      CHECK(g.bossEncounter() && g.bossHealth() > 0);
      auto &e = g.enemies.items[0];
      e.hp = 1;
      auto *b = g.shots.acquire();
      b->oldX = b->x = e.x + 10;
      b->oldY = e.y + 40;
      b->y = e.y;
      g.collisions();
      CHECK(!e.active);
      g.update();
      for (int i = 0; i < 90; ++i)
        g.update();
      CHECK(g.campaignComplete() && g.stage() == 12);
    }
    NeonBreaker b;
    b.start({});
    b.level = 12;
    b.layout();
    b.bossHP = 1;
    b.launched = true;
    b.tick = 0;
    b.ballX = 88;
    b.ballY = 73;
    b.vx = 0;
    b.vy = -3;
    b.update();
    CHECK(b.campaignComplete() && b.stage() == 12);
    SnakeRush s;
    s.start({});
    s.level = 12;
    s.length = 63;
    for (int i = 0; i < 63; ++i)
      s.body[unsigned(i)] = {int8_t(i % 20), int8_t(8 + i / 20)};
    s.body[0] = {9, 3};
    s.direction = s.nextDir = 0;
    s.food = {10, 3};
    s.step();
    CHECK(s.campaignComplete());
    TurboDodge t;
    t.start({});
    t.level = 12;
    t.tick = 599;
    t.spawn = 100;
    t.update();
    CHECK(t.campaignComplete());
    FlapEscape f;
    f.start({});
    f.level = 12;
    f.points = 59;
    f.gates.clear();
    f.spawn = 100;
    f.flying = true;
    auto *gate = f.gates.acquire();
    gate->x = 17;
    gate->gapY = 100;
    f.y = 140;
    f.vy = 0;
    f.update();
    CHECK(f.campaignComplete());
  }
  static void shooter() {
    SkyInvaders g;
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
    g.handle({0, Up});
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
  CHECK(!s.record(64, 90));
  CHECK(s.record(24, 123));
  s.favorites = uint64_t(1) << 24;
  auto b = encode(s);
  Settings out;
  CHECK(decode(b, out));
  CHECK(out.best == s.best);
  CHECK(out.favorites == s.favorites);
  CHECK(out.skyLeaders[0] == 120);
  for (uint8_t mode = 0; mode < DifficultyCount; ++mode) {
    s.difficulty = mode;
    CHECK(decodeStored(encode(s).data(), SaveSize, out));
    CHECK(out.difficulty == mode && out.best == s.best && out.favorites == s.favorites);
  }
  for (size_t k = 0; k < b.size(); k++) {
    auto bad = b;
    bad[k] ^= 0x80;
    Settings unchanged;
    unchanged.best[0] = 99;
    CHECK(!decode(bad, unchanged));
    CHECK(unchanged.best[0] == 99);
  }
  s.difficulty = 3;
  CHECK(s.valid());
  CHECK(decode(encode(s), out) && out.difficulty == 3);
  CHECK(out.best == s.best && out.favorites == s.favorites);
  s.difficulty = 4;
  CHECK(!s.valid());
  CHECK(!decode(encode(s), out));
  s = Settings{};
  s.fps = 45;
  CHECK(!decode(encode(s), out));
  s = Settings{};
  s.skyLeaders = {0, 10, 0, 0, 0};
  CHECK(!decode(encode(s), out));
}
static void newGameRules() {
  Game &cargo = gameFor(GameId::Cargo);
  cargo.start({});
  auto move = [&](Button b) {
    cargo.handle({0, b});
    cargo.update();
  };
  move(Left);
  move(Up);
  move(Special);
  move(Up);
  move(Down);
  move(Right);
  move(Right);
  move(Up);
  CHECK(cargo.stage() == 2);
  CHECK(cargo.score() == 940);
  CHECK(!cargo.gameOver());
  cargo.start({});
  move(Left);
  move(Up);
  move(Fire);
  move(Left);
  move(Up);
  move(Down);
  move(Right);
  move(Right);
  move(Up);
  CHECK(cargo.stage() == 2);
  CHECK(cargo.score() == 940);
  Game &mines = gameFor(GameId::Mines);
  for (unsigned seed = 1; seed <= 100; seed++) {
    GameOptions options;
    options.seed = seed;
    mines.start(options);
    mines.handle({0, Special});
    mines.update();
    mines.handle({0, Fire});
    mines.update();
    CHECK(mines.score() == 0);
    mines.handle({0, Special});
    mines.update();
    mines.handle({0, Fire});
    mines.update();
    CHECK(!mines.gameOver());
    CHECK(mines.score() > 0);
  }
  Game &blocks = gameFor(GameId::Blocks);
  blocks.start({});
  blocks.handle({0, Special});
  blocks.update();
  CHECK(blocks.score() > 0);
  CHECK(!blocks.gameOver());
  std::array<uint8_t, 72> legacy{};
  auto put = [&](size_t at, uint32_t value) {
    for (int i = 0; i < 4; i++)
      legacy[at + i] = uint8_t(value >> (i * 8));
  };
  put(0, 0x41435244);
  put(4, 1);
  legacy[8] = 1;
  legacy[9] = legacy[10] = legacy[14] = 1;
  legacy[15] = 30;
  put(16, 1234);
  put(48, 1234);
  put(68, checksum(legacy.data(), 68));
  Settings migrated;
  CHECK(decodeStored(legacy.data(), legacy.size(), migrated));
  CHECK(migrated.best[0] == 1234);
  CHECK(migrated.best[24] == 0);
  CHECK(migrated.skyLeaders[0] == 1234);
  legacy[18] ^= 1;
  CHECK(!decodeStored(legacy.data(), legacy.size(), migrated));
}
static void consoleRules() {
  Settings s;
  Console c(s);
  c.begin({}, 0);
  CHECK(c.consumeSuppression());
  c.update({Fire, Fire}, 21);
  CHECK(c.state() == Screen::Launcher);
  CHECK(c.consumeSuppression());
  c.update({0, Down}, 40);
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
  s.lastGame = 0;
  c.begin({}, 0);
  c.update({0, Fire}, 20);
  c.update({0, Down}, 40);
  CHECK(c.selected() == 1);
  c.update({0, Special}, 60);
  CHECK(s.favorites & (uint64_t(1) << 1));
  c.update({0, Pause}, 80);
  CHECK(c.state() == Screen::Utilities);
  c.update({0, Fire}, 100);
  CHECK(c.state() == Screen::Settings);
  c.update({0, Fire}, 120);
  CHECK(s.sound == 0);
  CHECK(c.consumeDirty());
  c.update({0, Down}, 140);
  uint32_t changedAt = 160;
  for (unsigned expected : {2u, 3u, 0u, 1u}) {
    c.update({0, Fire}, changedAt);
    changedAt += 10;
    CHECK(s.difficulty == expected && s.valid());
    CHECK(c.consumeDirty());
  }
  s.difficulty = 3;
  c.update({0, Special}, 200);
  c.update({0, Special}, 220);
  c.update({0, Fire}, 240);
  c.update({0, Fire}, 260);
  CHECK(c.state() == Screen::Playing && c.active()->lives() == 1);
  // Each classic owns its entire HUD and survives the regular pause/controls flow.
  for (auto id : {GameId::Keystone, GameId::Raid}) {
    Settings native;
    native.lastGame = uint8_t(id);
    Console console(native);
    console.begin({}, 0);
    console.update({0, Fire}, 25);
    console.update({0, Fire}, 50);
    console.update({0, Fire}, 75);
    CHECK(console.state() == Screen::Playing && console.active()->stageLimit() == 0);
    std::array<uint16_t, W * H> direct{}, integrated{};
    Canvas a, b;
    a.attach(direct.data());
    b.attach(integrated.data());
    console.active()->render(a);
    console.render(b);
    CHECK(direct == integrated);
    console.update({0, Pause}, 100);
    CHECK(console.state() == Screen::Paused);
    console.update({0, Special}, 125);
    CHECK(console.state() == Screen::Playing);
    CHECK(native.record(unsigned(id), 12345));
    native.favorites |= uint64_t(1) << unsigned(id);
    Settings restored;
    CHECK(decode(encode(native), restored));
    CHECK(restored.best[unsigned(id)] == 12345 && restored.favorites == native.favorites);
  }
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
  for (unsigned id = 0; id < GameCount; id++) {
    CHECK(registry()[id].title[0]);
    CHECK(std::strlen(registry()[id].title) <= 29);
    CHECK(std::strlen(registry()[id].description) <= 29);
    for (const auto *line : registry()[id].instructions)
      CHECK(line && std::strlen(line) <= 27);
    for (unsigned difficulty : {0u, 1u, 2u, 3u}) {
      Game &g = gameFor(GameId(id));
      GameOptions o;
      o.difficulty = uint8_t(difficulty);
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
      uint16_t previous = 0;
      for (unsigned frame = 0; frame < 10000; frame++) {
        uint16_t raw = uint16_t(random.next() & 255);
        g.handle({raw, uint16_t(raw & ~previous), 0, uint16_t(frame % 8 == 0 ? raw : 0), 0});
        previous = raw;
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
  std::printf("Lifecycle soak: %u simulation ticks, %u game-over/restart cycles\n",
              unsigned(GameCount * DifficultyCount * 10000), overs);
}
static void visualChecks() {
  // Reduced effects must freeze decorative motion; live effects must actually animate.
  std::array<uint16_t, W * H> first{}, second{};
  Canvas a, b;
  a.attach(first.data());
  b.attach(second.data());
  Settings settings;
  settings.effects = 0;
  Console console(settings);
  console.begin({}, 0);
  console.update({}, 1200);
  console.render(a);
  console.update({}, 2200);
  console.render(b);
  CHECK(first == second);
  settings.effects = 1;
  console.render(a);
  console.update({}, 3200);
  console.render(b);
  CHECK(first != second);
  CHECK(console.selected() == 0 && console.state() == Screen::Launcher);
  // The original pixel assets must stay inside the framebuffer in both storage formats.
  std::array<uint8_t, W * H + 2> guarded{};
  guarded.front() = 17;
  guarded.back() = 19;
  a.attach(guarded.data() + 1, false);
  visual::backdrop(a, 400, true);
  for (int id = 0; id < int(GameCount); ++id)
    visual::thumbnail(a, id, id % 2 ? -10 : W - 6, H - 5, Cyan, 40);
  visual::alien(a, -8, -8, Green, 1, 2);
  visual::tile(a, W - 2, H - 2, 9, 8, Pink);
  CHECK(guarded.front() == 17 && guarded.back() == 19);
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
  for (unsigned i = 0; i < GameCount; i++) {
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
    if (!g.ownsHud()) {
      c.rect(0, 0, W, Top, Navy);
      c.centered(5, registry()[i].title, registry()[i].accent);
    }
    write(int(i) + 1);
    g.cleanup();
  }
  console.update({0, Fire}, 30);
  console.update({0, Fire}, 50);
  console.update({0, Back}, 70);
  c.clear();
  console.render(c);
  write(int(GameCount) + 1);
  console.update({0, Right}, 90);
  console.update({0, Right}, 110);
  console.update({0, Right}, 130);
  console.update({0, Fire}, 150);
  console.update({0, Special}, 170);
  console.update({0, Fire}, 190);
  c.clear();
  console.render(c);
  write(int(GameCount) + 2);
  console.update({0, Fire}, 210);
  CHECK(console.state() == Screen::Playing);
  RuleTests::prepareFinalBoss(*console.active());
  c.clear();
  console.render(c);
  write(int(GameCount) + 3);
  RuleTests::finishFinalBoss(*console.active());
  console.update({}, 250);
  CHECK(console.state() == Screen::Over);
  c.clear();
  console.render(c);
  write(int(GameCount) + 4);
  // Additional real menu captures make the redesign reviewable alongside all 50 games.
  Console menus(settings);
  menus.begin({}, 0);
  menus.update({}, 600);
  menus.render(c);
  write(int(GameCount) + 5);
  menus.update({}, 1200);
  menus.update({0, Back}, 1300);
  menus.render(c);
  write(int(GameCount) + 6);
  menus.update({0, Fire}, 1400);
  menus.render(c);
  write(int(GameCount) + 7);
  menus.update({0, Special}, 1500);
  menus.update({0, Down}, 1600);
  menus.update({0, Fire}, 1700);
  menus.render(c);
  write(int(GameCount) + 8);
  menus.update({0, Special}, 1750);
  CHECK(menus.state() == Screen::Scores);
  menus.render(c);
  write(int(GameCount) + 12);
  menus.update({0, Back}, 1800);
  menus.update({0, Fire}, 1900);
  menus.update({0, Down}, 2000);
  settings.difficulty = 3;
  menus.render(c);
  write(int(GameCount) + 9);
  Console soundMenus(settings);
  soundMenus.begin({}, 0);
  soundMenus.update({}, 1200);
  soundMenus.update({0, Back}, 1300);
  soundMenus.update({0, Fire}, 1400);
  CHECK(soundMenus.state() == Screen::Settings);
  for (unsigned row = 0; row < 9; ++row)
    soundMenus.update({0, Down}, 1500 + row * 100);
  auto oldMusic = settings.music;
  soundMenus.update({0, Fire}, 2500);
  CHECK(settings.music != oldMusic && soundMenus.consumeDirty());
  soundMenus.render(c);
  write(int(GameCount) + 10);
  for (unsigned row = 0; row < 3; ++row)
    soundMenus.update({0, Down}, 2600 + row * 100);
  soundMenus.update({0, Fire}, 3000);
  CHECK(soundMenus.state() == Screen::SoundPreview);
  soundMenus.render(c);
  write(int(GameCount) + 11);
  soundMenus.update({0, Left}, 3100);
  CHECK(soundMenus.previewTrack() == int(GameCount));
  soundMenus.update({0, Fire}, 3200);
  CHECK(soundMenus.takeSound() == Sound::Power);
  soundMenus.update({0, Back}, 3300);
  CHECK(soundMenus.state() == Screen::Settings);
}
static void puzzleRules() {
  std::array<uint8_t, 4> line{{1, 1, 1, 1}};
  CHECK(mergeLine(line) == 8);
  CHECK((line == std::array<uint8_t, 4>{{2, 2, 0, 0}}));
  line = {{1, 1, 2, 0}};
  CHECK(mergeLine(line) == 4);
  CHECK((line == std::array<uint8_t, 4>{{2, 2, 0, 0}}));
  line = {{0, 3, 0, 3}};
  CHECK(mergeLine(line) == 16);
  CHECK(line[0] == 4 && line[1] == 0);
  line = {{15, 15, 0, 0}};
  CHECK(mergeLine(line) == 0);
  CHECK(line[0] == 15 && line[1] == 15);
  std::array<uint8_t, 81> mines{};
  mines[0] = mines[1] = mines[9] = 1;
  CHECK(adjacentMines(mines, 0) == 2);
  CHECK(adjacentMines(mines, 10) == 3);
  CHECK(adjacentMines(mines, 80) == 0);
  std::array<uint8_t, 64> colors{};
  colors[7] = colors[8] = colors[9] = 1;
  CHECK(matchRuns(colors) == 0);
  colors.fill(0);
  colors[8] = colors[9] = colors[10] = colors[11] = 2;
  CHECK(matchRuns(colors) == (uint64_t(15) << 8));
  colors.fill(0);
  colors[5] = colors[13] = colors[21] = 4;
  CHECK(matchRuns(colors) == ((uint64_t(1) << 5) | (uint64_t(1) << 13) | (uint64_t(1) << 21)));
  CHECK(GameCount >= 50);
  CHECK(registry().size() == GameCount);
}
static void releasePolicy() {
  Version v;
  CHECK(parseVersion("v2.10.65535", v));
  CHECK(v.minor == 10);
  CHECK(!parseVersion("2.1", v));
  CHECK(!parseVersion("2.1.65536", v));
  CHECK(!parseVersion("2.1.3-rc1", v));
  CHECK(!parseVersion("2.1.3x", v));
  CHECK(!parseVersion("2..3", v));
  CHECK(newerVersion("v2.1.0", "2.0.9"));
  CHECK(!newerVersion("2.0.9", "2.1.0"));
  CHECK(!newerVersion("2.0.0", "2.0.0"));
  CHECK(validRepository("ashkan89/Arcade"));
  CHECK(!validRepository("ashkan89"));
  CHECK(!validRepository("owner/repo/more"));
  CHECK(!validRepository("owner/repo?x=1"));
  CHECK(!validRepository("/repo"));
  uint8_t digest[32]{};
  CHECK(parseSha256("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", digest));
  CHECK(digest[0] == 1 && digest[31] == 239);
  CHECK(!parseSha256("abc", digest));
  CHECK(!parseSha256("g123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", digest));
}
static void setupKeys() {
  char key[13];
  formatSetupKey(key, 0xabcdef01, 0x12345678);
  CHECK(std::strcmp(key, "ABCDEF015678") == 0);
  formatSetupKey(key, 0, 0);
  CHECK(std::strcmp(key, "000000000000") == 0);
  formatSetupKey(key, 0xffffffff, 0xffffffff);
  CHECK(std::strcmp(key, "FFFFFFFFFFFF") == 0);
  // The displayed glyph must represent the exact case-sensitive password byte.
  const char *alphabet = "0123456789ABCDEF";
  for (int i = 0; i < 16; ++i) {
    formatSetupKey(key, uint32_t(i) * 0x11111111u, uint32_t(i) * 0x1111u);
    for (int j = 0; j < 12; ++j)
      CHECK(key[j] == alphabet[i]);
  }
}
static void httpJson() {
  struct Transport {
    std::string body;
    size_t position = 0;
    bool truncated = false;
    int read(void *out, int count) {
      size_t n = std::min(size_t(count), std::min(size_t(7), body.size() - position));
      std::memcpy(out, body.data() + position, n);
      position += n;
      return int(n);
    }
    bool complete() const {
      return !truncated && position == body.size();
    }
  };
  auto clock = []() -> uint32_t { return 0; };
  std::string body = "{\"tag_name\":\"v2.0.1\",\"draft\":false,\"prerelease\":false,\"body\":\"";
  body += std::string(90000, 'x');
  body += "\",\"assets\":[{\"name\":\"arcade-esp32-wroom-n4.bin\",\"size\":123456,\"browser_"
          "download_url\":\"https://github.com/a/b/releases/download/v2.0.1/a.bin\"}]}\n";
  DynamicJsonDocument doc(16384);
  Transport input{body};
  HttpJsonReader<Transport> reader(input, 131072, clock);
  CHECK(readReleaseJson(doc, reader));
  CHECK(reader.finish(int64_t(body.size())));
  CHECK(doc["body"].isNull());
  CHECK(doc.memoryUsage() < 2048);
  // Destroy/reuse input storage before consuming strings, as the updater does.
  input.body.assign(input.body.size(), '!');
  CHECK(std::strcmp(doc["tag_name"], "v2.0.1") == 0);
  CHECK(std::strcmp(doc["assets"][0]["name"], "arcade-esp32-wroom-n4.bin") == 0);
  for (const char *bad : {"{\"tag_name\":", "{} garbage", "{}{}"}) {
    Transport t{bad};
    HttpJsonReader<Transport> r(t, 4096, clock);
    CHECK(!(readReleaseJson(doc, r) && r.finish(int64_t(t.body.size()))));
  }
  Transport truncated{"{}", 0, true};
  HttpJsonReader<Transport> r1(truncated, 4096, clock);
  CHECK(readReleaseJson(doc, r1));
  CHECK(!r1.finish(2));
  Transport wrongSize{"{}"};
  HttpJsonReader<Transport> r2(wrongSize, 4096, clock);
  CHECK(readReleaseJson(doc, r2));
  CHECK(!r2.finish(3));
  Transport oversized{body};
  HttpJsonReader<Transport> r3(oversized, 4096, clock);
  CHECK(!readReleaseJson(doc, r3));
  Transport small{body};
  DynamicJsonDocument tiny(32);
  HttpJsonReader<Transport> r4(small, 131072, clock);
  CHECK(!readReleaseJson(tiny, r4));
  Transport expired{"{}"};
  HttpJsonReader<Transport> r5(expired, 4096, []() -> uint32_t {
    static uint32_t now = 0;
    now += 45001;
    return now;
  });
  CHECK(!readReleaseJson(doc, r5));
  Transport manifest{"{\"board\":\"esp32-wroom-n4-ili9225\",\"version\":\"2.0.1\"}"};
  HttpJsonReader<Transport> r6(manifest, 4096, clock);
  CHECK(!deserializeJson(doc, r6));
  CHECK(r6.finish(int64_t(manifest.body.size())));
  manifest.body.clear();
  CHECK(std::strcmp(doc["board"], "esp32-wroom-n4-ili9225") == 0);
}
int main() {
  httpJson();
  setupKeys();
  core();
  inputs();
  saves();
  consoleRules();
  puzzleRules();
  releasePolicy();
  newGameRules();
  RuleTests::shooter();
  RuleTests::difficulty();
  RuleTests::breaker();
  RuleTests::snake();
  RuleTests::turbo();
  RuleTests::flap();
  RuleTests::endings();
  CHECK(verifyVectorDrift());
  CHECK(verifyBugBlaster());
  CHECK(verifyCityShield());
  CHECK(verifyPrismRider());
  CHECK(verifyNeonClaim());
  CHECK(verifyDotChase());
  CHECK(verifyTunnelDigger());
  CHECK(verifyBarrelClimb());
  CHECK(verifySkyJoust());
  CHECK(verifyBurgerWorks());
  CHECK(verifyBubbleQuest());
  CHECK(verifySnowSiege());
  CHECK(verifyCanyonStrike());
  CHECK(verifySolarLance());
  CHECK(verifyFortressRun());
  CHECK(verifyGhoulCastle());
  CHECK(verifyIronFront());
  CHECK(verifyBladeDuel());
  CHECK(verifyElevatorHeist());
  CHECK(verifyAlpineRush());
  CHECK(verifyWildRails());
  CHECK(verifyCrystalCourier());
  CHECK(verifyNightRacer());
  CHECK(verifyPinballNova());
  CHECK(verifyRescueWing());
  CHECK(verifyCargoShift());
  CHECK(verifyPongCircuit());
  CHECK(verifyBlockCascade());
  CHECK(verifyMineGrid());
  CHECK(verifyMazeRunner());
  CHECK(verifyRiverLeap());
  CHECK(verifyOrbitGuard());
  CHECK(verifyLunarLander());
  CHECK(verifyBlastGrid());
  CHECK(verifyTankArena());
  CHECK(verifyBubbleBurst());
  CHECK(verifyGridMerge());
  CHECK(verifyMemoryMatch());
  CHECK(verifyRobotWhack());
  CHECK(verifyStackTower());
  CHECK(verifyJumpQuest());
  CHECK(verifyColorCascade());
  CHECK(verifyKeystoneKapers());
  CHECK(verifyRiverRaid());
  std::printf(
      "Focused campaign checks: %u assertions plus legacy endings and solvable warehouses\n",
      campaignAssertions);
  lifecycle();
  verifyImprovements();
  visualChecks();
  preview();
  std::printf(
      "PASS %d assertions across core, inputs, saves, menus, game rules and lifecycle soak\n",
      assertions);
}
