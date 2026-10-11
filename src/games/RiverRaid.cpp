#include "assets/AtariPixels.h"
#include "games/Registration.h"
namespace arcade {
class RiverRaid final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyRiverRaid();
#endif
  enum Kind : uint8_t { House, Fuel, Tanker, Helicopter, EnemyJet, Bridge };
  struct Object {
    float x = 80;
    Kind kind = House;
    int8_t direction = 1;
    bool alive = true;
  };
  struct Section {
    uint16_t seed = 0xa814, after = 0;
    uint8_t difficulty = 1;
    std::array<uint8_t, 17> bank{}, island{};
    std::array<Object, 16> objects{};
  };
  Section current, next;
  struct Burst {
    float x = 0, y = 0;
    int timer = 0;
  };
  std::array<Burst, 4> bursts{};
  void explode(float xx, float yy) {
    for (auto &b : bursts)
      if (!b.timer) {
        b = {xx, yy, 16};
        return;
      }
    bursts[0] = {xx, yy, 16};
  }
  struct Player {
    uint16_t seed = 0xa814;
    uint8_t difficulty = 1;
    uint32_t score = 0, section = 1;
    int lives = 4;
  };
  std::array<Player, 2> players{};
  uint8_t player = 0;
  bool twoPlayers = false, started = false, awaitingTakeoff = true;
  float x = 80, travel = 0, speed = 1.5f;
  float missileX = 0, missileY = 0;
  bool missile = false, bridgeCleared = false;
  uint32_t fuel = 65535, sectionNumber = 1;
  int crashTimer = 0, bank = 0;
  static uint16_t random16(uint16_t &state) {
    // Original 16-bit feedback operation (high-byte bits 7 XOR 4).
    unsigned hi = state >> 8;
    state = uint16_t((state << 1) | (((hi >> 7) ^ (hi >> 4)) & 1));
    return state;
  }
  static uint8_t followingDifficulty(uint8_t d) {
    return d == 48 ? 47 : d + 1;
  }
  static void generate(Section &s, uint16_t seed, uint8_t difficulty) {
    s = Section{};
    s.seed = seed;
    s.difficulty = difficulty;
    uint16_t state = seed;
    uint8_t phase = 0;
    s.bank[0] = 40;
    for (int i = 0; i < 16; ++i) {
      uint16_t r = random16(state);
      unsigned lo = r & 255, hi = r >> 8;
      if (!(difficulty & 1) && i < 14) {
        if (phase == 1)
          phase = 3;
        else if (phase == 2)
          phase = 0;
        else if (!(lo & 0x30))
          phase = phase == 3 ? 2 : 1;
        unsigned id = lo & 15;
        if (id < 2)
          id += 2;
        id = std::min(id, difficulty < 5 ? 8u : (phase & 2 ? 13u : 14u));
        // Four-pixel mirrored playfield cells, interpolated over each block.
        s.bank[i + 1] = phase & 2 ? 16 : uint8_t(8 + id * 4);
        s.island[i + 1] = phase & 2 ? uint8_t(std::min(24u, id * 2)) : 0;
      } else {
        s.bank[i + 1] = 40;
        s.island[i + 1] = 0;
      }
      Object &o = s.objects[i];
      if (i == 15) {
        o.kind = Bridge;
        o.x = 80;
      } else if (i == 0)
        o.kind = House;
      else if (hi > unsigned((63 - difficulty) * 2)) {
        static constexpr Kind enemies[] = {Tanker,   Helicopter, Tanker,     Helicopter,
                                           EnemyJet, Tanker,     Helicopter, Helicopter};
        o.kind = enemies[hi & (difficulty < 3 ? 1 : 7)];
      } else
        o.kind = lo & 0x40 ? House : Fuel;
      float edge = std::max(s.bank[i], s.bank[i + 1]);
      float isle = std::max(s.island[i], s.island[i + 1]);
      if (o.kind == House)
        o.x = lo & 0x80 ? 148 : 12;
      else if (o.kind != Bridge) {
        float margin = halfWidth(o.kind) + 2;
        float left = edge + margin, right = 160 - edge - margin;
        if (isle) {
          if (lo & 0x80)
            left = 80 + isle + margin;
          else
            right = 80 - isle - margin;
        }
        if (right < left) {
          o.kind = Helicopter;
          right = left;
        }
        o.x = left + (right - left) * (lo & 63) / 63.f;
      }
      o.direction = lo & 0x80 ? -1 : 1;
      random16(state); // second half-block advances the same feedback register
    }
    s.after = state;
  }
  static void profile(const Section &s, float position, float &edge, float &isle) {
    position = clamp(position, 0.f, 511.99f);
    int block = int(position) / 32;
    float t = (position - block * 32) / 32.f;
    edge = std::floor((s.bank[block] * (1 - t) + s.bank[block + 1] * t) / 4) * 4;
    isle = std::floor((s.island[block] * (1 - t) + s.island[block + 1] * t) / 4) * 4;
  }
  static bool water(const Section &s, float position, float px, float radius) {
    if (std::abs(position - 496.f) < 7)
      return !s.objects[15].alive && px - radius >= 64 && px + radius <= 96;
    float edge, isle;
    profile(s, position, edge, isle);
    return px - radius >= edge && px + radius <= 160 - edge &&
           (!isle || px + radius < 80 - isle || px - radius > 80 + isle);
  }
  void reward(unsigned n) {
    uint32_t before = points / 10000;
    add(n);
    health = std::min(10, health + int(points / 10000 - before));
    if (points >= 1000000) {
      points = 1000000;
      ended = true;
      sound(Sound::Over);
    }
  }
  void restartSection() {
    generate(current, current.seed, current.difficulty);
    generate(next, current.after, followingDifficulty(current.difficulty));
    travel = 0;
    x = 80;
    speed = 1.5f;
    fuel = 65535;
    missile = bridgeCleared = false;
    awaitingTakeoff = true;
  }
  void enterNext() {
    current = next;
    generate(next, current.after, followingDifficulty(current.difficulty));
    ++sectionNumber;
    level = uint16_t(std::min(sectionNumber, 65535u));
    bridgeCleared = false;
  }
  void crash() {
    if (crashTimer || ended)
      return;
    missile = false;
    explode(x, 163);
    sound(Sound::Explosion);
    --health;
    if (bridgeCleared)
      enterNext();
    if (twoPlayers) {
      players[player] = {current.seed, current.difficulty, points, sectionNumber, health};
      uint8_t other = player ^ 1;
      if (players[other].lives > 0) {
        player = other;
        const auto &p = players[player];
        points = p.score;
        health = p.lives;
        sectionNumber = p.section;
        level = uint16_t(std::min(sectionNumber, 65535u));
        generate(current, p.seed, p.difficulty);
      }
    }
    if (health <= 0) {
      ended = true;
      if (twoPlayers)
        points = std::max(players[0].score, players[1].score);
      sound(Sound::Over);
      return;
    }
    restartSection();
    crashTimer = 90;
  }
  static float objectY(int index) {
    return index * 32.f + 16;
  }
  static float halfWidth(Kind kind) {
    return kind == Tanker ? 9.f : kind == Bridge ? 16.f : kind == EnemyJet ? 8.f : 5.f;
  }
  void objects(Section &s, float offset, float previousMissileY) {
    for (int i = 0; i < 16; ++i) {
      auto &o = s.objects[i];
      if (!o.alive)
        continue;
      float yy = 163 - (offset + objectY(i) - travel);
      if (yy < 12 || yy > 180)
        continue;
      if (o.kind == EnemyJet || ((o.kind == Tanker || o.kind == Helicopter) && tick % 120 < 70)) {
        float step = o.kind == EnemyJet ? 1.25f : o.kind == Helicopter ? .36f : .20f;
        float nx = o.x + o.direction * step;
        if (!water(s, objectY(i), nx, halfWidth(o.kind)))
          o.direction *= -1;
        else
          o.x = nx;
      }
      if (missile && o.kind != House &&
          swept(missileX, previousMissileY, missileX, missileY,
                {o.x - halfWidth(o.kind), yy - 5, halfWidth(o.kind) * 2, 10}, 1)) {
        o.alive = false;
        missile = false;
        explode(o.x, yy);
        static constexpr unsigned scores[] = {0, 80, 30, 60, 100, 500};
        reward(scores[o.kind]);
        sound(Sound::Explosion);
        if (o.kind == Bridge && offset == 0)
          bridgeCleared = true;
        continue;
      }
      if (std::abs(yy - 163) < (o.kind == Fuel ? 12 : 10) &&
          std::abs(x - o.x) < halfWidth(o.kind) + 3) {
        if (o.kind == Fuel) {
          fuel = std::min(65535u, fuel + 512);
          if (tick % 12 == 0)
            sound(Sound::Power);
        } else if (o.kind != House) {
          crash();
          return;
        }
      }
    }
  }
  void simulate() override {
    for (auto &b : bursts)
      if (b.timer) {
        --b.timer;
        b.y += crashTimer ? 0 : speed;
      }
    if (crashTimer) {
      --crashTimer;
      return;
    }
    // B replaces the console's game-select switch before the first takeoff.
    if (!started && input.hit(Special)) {
      twoPlayers = !twoPlayers;
      sound(Sound::Select);
      return;
    }
    if (!started) {
      if (!(input.held & (Left | Right | Up | Down | Fire)))
        return;
      started = true;
    }
    if (awaitingTakeoff) {
      if (!(input.held & (Left | Right | Up | Down | Fire)))
        return;
      awaitingTakeoff = false;
    }
    bank = int(input.down(Right)) - int(input.down(Left));
    x = clamp(x + bank * 1.1f, 4.f, 156.f);
    float target = input.down(Up) ? 3.f : input.down(Down) ? .75f : 1.5f;
    speed += clamp(target - speed, -.0235f, .0235f);
    travel += speed;
    if (travel >= 512) {
      travel -= 512;
      enterNext();
    }
    if (fuel <= 32) {
      crash();
      return;
    }
    fuel -= 32;
    if (fuel < 16384 && tick % 30 == 0)
      sound(Sound::Hit);
    if (!water(current, travel, x, 3) || !water(current, travel + 6, x, 2)) {
      crash();
      return;
    }
    if (!missile && input.down(Fire)) {
      missile = true;
      missileX = x;
      missileY = 154;
      sound(Sound::Shot);
    }
    float oldY = missileY;
    if (missile) {
      missileY -= 4;
      if (options.difficulty < 2)
        missileX = clamp(missileX + bank * 1.1f, 0.f, 160.f);
      if (missileY < 20)
        missile = false;
    }
    objects(current, 0, oldY);
    if (crashTimer || ended)
      return;
    objects(next, 512, oldY);
  }
  static void drawObject(Canvas &c, const Object &o, int yy, unsigned frame) {
    int xx = int(o.x) + 8;
    bool flip = o.direction < 0;
    switch (o.kind) {
    case House:
      atari::mask(c, xx - 8, yy - 8,
                  std::array<uint8_t, 13>{{0x18, 0x3c, 0x7e, 0xff, 0x7e, 0x7e, 0x7e, 0x7e, 0x66,
                                           0x66, 0x66, 0x66, 0x7e}},
                  White, false, 2);
      c.rect(xx - 8, yy - 4, 16, 2, Red);
      break;
    case Fuel:
      c.rect(xx - 5, yy - 12, 10, 24, White);
      c.rect(xx - 5, yy - 12, 10, 3, Red);
      c.rect(xx - 5, yy + 9, 10, 3, Red);
      atari::mask(c, xx - 4, yy - 7,
                  std::array<uint8_t, 15>{{0x7c, 0x40, 0x70, 0x40, 0x40, 0, 0x44, 0x44, 0x44, 0x44,
                                           0x7c, 0, 0x7c, 0x40, 0x70}},
                  Red);
      break;
    case Tanker:
      atari::mask(c, xx - 8, yy - 4,
                  std::array<uint8_t, 9>{{0x04, 0x04, 0x34, 0x3c, 0x7f, 0xff, 0xff, 0x7e, 0x3c}},
                  0xce6f, flip, 2);
      c.rect(xx - 4, yy - 3, 4, 2, Red);
      break;
    case Helicopter:
      c.rect(xx - 7, yy - 7, 14, 1, atari::Ink);
      atari::mask(
          c, xx - 4, yy - 6,
          std::array<uint8_t, 10>{{0x10, 0x38, 0x7c, 0xfe, 0xf7, 0x7f, 0x38, 0x28, 0x7c, 0x00}},
          atari::Ink, flip);
      c.rect(xx + (flip ? -3 : 1), yy - 3, 2, 2, White);
      if (frame & 1)
        c.rect(xx - 5, yy - 8, 10, 1, atari::Ink);
      break;
    case EnemyJet:
      atari::mask(c, xx - 8, yy - 4,
                  std::array<uint8_t, 9>{{0x10, 0x30, 0x70, 0xff, 0xff, 0x70, 0x30, 0x10, 0}},
                  White, flip, 2);
      break;
    case Bridge:
      c.rect(8, yy - 6, 160, 12, atari::Concrete);
      c.rect(8, yy - 7, 160, 1, atari::Ink);
      c.rect(8, yy + 6, 160, 1, atari::Ink);
      for (int p = 8; p < 168; p += 12)
        c.rect(p, yy, 6, 1, Yellow);
      c.rect(xx - 16, yy - 7, 32, 14, o.alive ? 0xad45 : atari::RiverWater);
      break;
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    health = 4;
    sectionNumber = 1;
    crashTimer = bank = 0;
    players = {};
    bursts = {};
    player = 0;
    twoPlayers = started = false;
    generate(current, 0xa814, 1);
    restartSection();
  }
  uint16_t stageLimit() const override {
    return 0;
  }
  bool ownsHud() const override {
    return true;
  }
  size_t activeEntities() const override {
    size_t n = 33 + unsigned(missile);
    for (const auto &b : bursts)
      n += b.timer != 0;
    return n;
  }
  void render(Canvas &c) const override {
    c.clear(Black);
    for (int yy = 20; yy < 180; ++yy) {
      float pos = travel + 163 - yy;
      const Section &s = pos >= 512 ? next : current;
      if (pos >= 512)
        pos -= 512;
      float edge, isle;
      profile(s, pos, edge, isle);
      uint16_t land = s.difficulty & 1 ? atari::RiverGrass : atari::RiverDarkGrass;
      c.rect(8, yy, 160, 1, land);
      c.rect(8 + int(edge), yy, 160 - int(edge) * 2, 1, atari::RiverWater);
      if (isle)
        c.rect(88 - int(isle), yy, int(isle) * 2, 1, land);
    }
    for (int pass = 0; pass < 2; ++pass) {
      const Section &s = pass ? next : current;
      for (int i = 0; i < 16; ++i) {
        int yy = int(163 - (pass * 512 + objectY(i) - travel));
        if ((s.objects[i].alive || s.objects[i].kind == Bridge) && yy >= 32 && yy <= 166)
          drawObject(c, s.objects[i], yy, tick / 5);
      }
    }
    if (!crashTimer || tick % 8 < 4)
      atari::jet(c, int(x) + 8, 163, bank);
    if (missile)
      c.rect(int(missileX) + 8, int(missileY), 1, 5, Yellow);
    for (const auto &b : bursts)
      if (b.timer) {
        int radius = 2 + (16 - b.timer) / 3;
        int xx = int(b.x) + 8, yy = int(b.y);
        for (int dy = -radius; dy <= radius; dy += 2)
          for (int dx = -radius; dx <= radius; dx += 2)
            if ((dx * dx + dy * dy <= radius * radius) && ((dx + dy + b.timer) % 3))
              c.pixel(xx + dx, yy + dy, b.timer > 8 ? Yellow : Orange);
      }
    c.rect(0, 0, W, 20, Black);
    c.rect(0, 180, W, 40, atari::Concrete);
    if (points >= 1000000)
      c.centered(183, "!!!!!!", Yellow, 2);
    else
      atari::digits(c, 52, 183, points, 6, Yellow);
    c.text(16, 201, "E", White);
    c.text(146, 201, "F", White);
    c.rect(28, 200, 111, 9, atari::Concrete, false);
    c.rect(30, 202, int(fuel * 107 / 65535), 5, fuel < 16384 && tick % 30 < 15 ? Red : Yellow);
    for (int i = 0; i < health - 1; ++i)
      c.rect(8 + i * 4, 185, 2, 9, Yellow);
    atari::credit(c, 213, Yellow);
    c.text(8, 7, "SECTION", Grey);
    c.number(52, 7, sectionNumber, White);
    if (twoPlayers)
      c.text(136, 7, player ? "P2" : "P1", Yellow);
    if (!started)
      c.centered(90, twoPlayers ? "B: 2 PLAYERS" : "B: 1 PLAYER", White);
    if (crashTimer)
      c.centered(88, "READY", Yellow);
  }
};
GameEntry describeRiverRaid() {
  return gameEntry<RiverRaid>(
      {"RIVER RAID",
       "ATARI 2600 RIVER PATROL",
       {"L/R BANK  A FIRES", "UP SPEEDS UP  DOWN SLOWS", "FLY OVER FUEL TO REFILL",
        "SHOOT BRIDGES: CHECKPOINT", "EASY/MED: STEER MISSILE", "B BEFORE START: 1/2 PLAYERS"},
       atari::Grass,
       51,
       Left | Right | Up | Down | Fire | Special});
}
#ifdef ARCADE_HOST_TESTS
bool verifyRiverRaid() {
  RiverRaid g;
  g.start({});
  HOST_CHECK(g.lives() == 4 && g.stageLimit() == 0 && g.ownsHud());
  uint16_t r = 0xa814;
  HOST_CHECK(g.random16(r) == 0x5029);
  r = 0xa814;
  unsigned period = 0;
  do {
    g.random16(r);
    ++period;
  } while (r != 0xa814 && period < 65536);
  HOST_CHECK(period == 57337);
  RiverRaid::Section a, b;
  for (uint8_t tier = 1; tier <= 48; ++tier) {
    g.generate(a, 0xa814, tier);
    g.generate(b, 0xa814, tier);
    HOST_CHECK(a.after == b.after && a.bank == b.bank && a.island == b.island);
    HOST_CHECK(a.objects[15].kind == RiverRaid::Bridge);
    for (int i = 0; i < 16; ++i) {
      HOST_CHECK(a.objects[i].x == b.objects[i].x);
      if (tier & 1)
        HOST_CHECK(a.island[i] == 0);
      if (tier < 3)
        HOST_CHECK(a.objects[i].kind != RiverRaid::EnemyJet);
    }
  }
  HOST_CHECK(g.followingDifficulty(48) == 47 && g.followingDifficulty(47) == 48);
  for (int i = 0; i < 60; ++i)
    g.update();
  HOST_CHECK(g.travel == 0 && g.fuel == 65535);
  g.handle({Up, Up});
  g.update();
  HOST_CHECK(g.travel > 0 && g.fuel == 65503);
  float travel = g.travel;
  auto fuel = g.fuel;
  g.pause();
  for (int i = 0; i < 60; ++i)
    g.update();
  HOST_CHECK(g.travel == travel && g.fuel == fuel);
  g.resume();
  auto seed = g.current.seed;
  g.current.objects[1].alive = false;
  g.crash();
  HOST_CHECK(g.lives() == 3 && g.current.seed == seed && g.current.objects[1].alive);
  g.crashTimer = 0;
  g.bridgeCleared = true;
  auto nextSeed = g.next.seed;
  g.crash();
  HOST_CHECK(g.sectionNumber == 2 && g.current.seed == nextSeed && g.travel == 0);
  g.start({});
  g.reward(10000);
  HOST_CHECK(g.health == 5);
  g.reward(100000);
  HOST_CHECK(g.health == 10);
  g.start({});
  // Destruction awards exactly the original bridge value and unlocks the next checkpoint.
  for (auto &o : g.current.objects)
    o.alive = false;
  g.current.objects[15].alive = true;
  g.travel = 380;
  g.missile = true;
  g.missileX = 80;
  g.missileY = 46;
  g.objects(g.current, 0, 52);
  HOST_CHECK(!g.current.objects[15].alive && g.score() == 500 && g.bridgeCleared);
  // The road remains visible after the bridge is destroyed, with a water gap.
  std::array<uint16_t, W * H> pixels{};
  Canvas canvas;
  canvas.attach(pixels.data());
  g.bursts = {};
  g.render(canvas);
  HOST_CHECK(canvas.pixel(28, 47) == atari::Concrete && canvas.pixel(88, 47) == atari::RiverWater);
  g.start({});
  for (auto &o : g.current.objects)
    o.alive = false;
  auto &depot = g.current.objects[2];
  depot.kind = RiverRaid::Fuel;
  depot.x = 80;
  depot.alive = true;
  g.travel = 80;
  g.fuel = 1000;
  g.objects(g.current, 0, 0);
  HOST_CHECK(g.fuel == 1512 && g.health == 4);
  g.crashTimer = 0;
  g.fuel = 32;
  g.handle({Fire, Fire});
  g.update();
  HOST_CHECK(g.health == 3);
  g.start({});
  for (int i = 0; i < 100; ++i) {
    g.enterNext();
    HOST_CHECK(!g.campaignComplete() && !g.gameOver());
  }
  HOST_CHECK(g.stage() == 101);
  g.health = 1;
  g.crash();
  HOST_CHECK(g.gameOver());
  auto score = g.score();
  for (int i = 0; i < 120; ++i)
    g.update();
  HOST_CHECK(g.score() == score);
  g.start({});
  HOST_CHECK(g.health == 4 && g.level == 1 && g.points == 0 && g.crashTimer == 0);
  // Terrain and intact bridges cannot be bypassed by flying around their center.
  HOST_CHECK(!g.water(g.current, 40, 12, 3));
  HOST_CHECK(g.water(g.current, 40, 80, 3));
  HOST_CHECK(!g.water(g.current, 496, 80, 3));
  g.current.objects[15].alive = false;
  HOST_CHECK(g.water(g.current, 496, 80, 3));
  HOST_CHECK(!g.water(g.current, 496, 50, 3));
  for (uint8_t difficulty : {0, 1, 2, 3}) {
    GameOptions o;
    o.difficulty = difficulty;
    g.start(o);
    g.missile = true;
    g.missileX = 80;
    g.missileY = 120;
    g.handle({Right, Right});
    g.update();
    HOST_CHECK(difficulty < 2 ? g.missileX > 80 : g.missileX == 80);
  }
  // Two-player game-select, alternation, independent checkpoints/scores and last survivor.
  g.start({});
  g.handle({Special, Special});
  g.update();
  HOST_CHECK(g.twoPlayers && !g.started && g.travel == 0);
  g.handle({Fire, Fire});
  g.update();
  g.reward(500);
  g.bridgeCleared = true;
  auto checkpoint = g.next.seed;
  g.crash();
  HOST_CHECK(g.player == 1 && g.score() == 0 && g.health == 4 && g.sectionNumber == 1);
  g.crashTimer = 0;
  g.reward(30);
  g.crash();
  HOST_CHECK(g.player == 0 && g.score() == 500 && g.health == 3 && g.current.seed == checkpoint);
  HOST_CHECK(g.sectionNumber == 2 && g.awaitingTakeoff);
  g.crashTimer = 0;
  g.health = 1;
  g.crash();
  HOST_CHECK(g.player == 1 && !g.gameOver());
  g.crashTimer = 0;
  g.health = 1;
  g.crash();
  HOST_CHECK(g.gameOver() && g.score() == 500);
  g.start({});
  HOST_CHECK(!g.twoPlayers && g.player == 0 && !g.started);
  return true;
}
#endif
} // namespace arcade
