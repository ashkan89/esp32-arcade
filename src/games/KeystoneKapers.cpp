#include "assets/AtariPixels.h"
#include "games/Registration.h"
namespace arcade {
class KeystoneKapers final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyKeystoneKapers();
#endif
  enum Kind : uint8_t { Ball, Radio, Cart, Biplane, Money, Suitcase };
  struct Hazard {
    float x = 0;
    uint8_t floor = 0;
    Kind kind = Ball;
    int8_t direction = 1;
    bool active = true;
  };
  std::array<Hazard, 30> hazards{};
  float x = 24, thiefX = 384, jump = 0, jumpVelocity = 0;
  uint8_t floor = 0, thiefFloor = 1;
  int facing = 1, thiefDirection = -1, seconds = 3000, grace = 0, transition = 0, ride = 0;
  uint32_t roundTick = 0;
  bool inLift = false, crouching = false;
  struct Lift {
    float floor = 0;
    bool open = false;
    int stop = 0;
  };
  static int ground(int f) {
    static constexpr int rows[] = {162, 120, 78, 52};
    return rows[f];
  }
  static float path(float xx, int f) {
    return f * 480.f + (f % 2 ? 480 - xx : xx);
  }
  Lift lift() const {
    static constexpr int stops[] = {0, 1, 2, 1, 0};
    unsigned phase = (roundTick / 192) % 4, t = roundTick % 192;
    if (t < 120)
      return {float(stops[phase]), true, stops[phase]};
    return {stops[phase] + (stops[phase + 1] - stops[phase]) * (t - 120) / 72.f, false,
            stops[phase]};
  }
  void reward(unsigned n) {
    auto old = points / 10000;
    add(n);
    health = std::min(4, health + int(points / 10000 - old));
  }
  void setupRound() {
    x = 24;
    thiefX = 384;
    floor = 0;
    thiefFloor = 1;
    jump = jumpVelocity = 0;
    facing = 1;
    thiefDirection = -1;
    seconds = 3000;
    grace = ride = 0;
    inLift = crouching = false;
    roundTick = 0;
    for (auto &h : hazards)
      h.active = false;
    int n = 0;
    unsigned tier = std::min(16u, unsigned(level));
    for (int f = 0; f < 3; ++f) {
      auto put = [&](Kind k, float xx, int dir) {
        hazards[n++] = {xx, uint8_t(f), k, int8_t(dir), true};
      };
      put(Ball, 132 + f * 80, -1);
      if (tier >= 2)
        put(Radio, 208 + f * 32, 0);
      if (tier >= 3)
        put(Cart, 320 + f * 24, -1);
      if (tier >= 4)
        put(Biplane, 440 - f * 64, 1);
      if (tier >= 7)
        put(Cart, 360 + f * 24, -1);
      if (tier >= 10)
        put(Biplane, 404 - f * 64, 1);
      if (tier >= 13)
        put(Cart, 400 + f * 24, -1);
      if (tier >= 16)
        put(Biplane, 368 - f * 64, 1);
      put(Money, 64 + f * 80, 0);
      put(Suitcase, 296 - f * 48, 0);
    }
  }
  void lose() {
    if (ended || transition)
      return;
    sound(Sound::Hit);
    if (--health <= 0) {
      ended = true;
      sound(Sound::Over);
      return;
    }
    setupRound();
    transition = 60;
  }
  void arrest() {
    unsigned multiplier = level <= 8 ? 100 : level <= 16 ? 200 : 300;
    reward(unsigned(std::max(0, seconds) / 60) * multiplier);
    if (level < 65535)
      ++level;
    sound(Sound::Wave);
    setupRound();
    transition = 90;
  }
  void thief() {
    // Intercepting ahead of Harry sends him back toward the lower floors.
    thiefDirection = path(x, floor) > path(thiefX, thiefFloor) ? (thiefFloor % 2 ? 1 : -1)
                                                               : (thiefFloor % 2 ? -1 : 1);
    float speed = .53f + std::min(15, int(level) - 1) * .012f;
    thiefX += thiefDirection * speed;
    if (thiefX < 12 || thiefX > 468) {
      bool ascending = (thiefFloor % 2 ? thiefDirection < 0 : thiefDirection > 0);
      if (ascending) {
        if (thiefFloor == 3) {
          lose();
          return;
        }
        ++thiefFloor;
      } else if (thiefFloor)
        --thiefFloor;
      thiefX = clamp(thiefX, 12.f, 468.f);
    }
  }
  void simulate() override {
    if (transition) {
      --transition;
      return;
    }
    ++roundTick;
    if (--seconds <= 0) {
      lose();
      return;
    }
    if (grace)
      --grace;
    Lift elevator = lift();
    if (inLift) {
      if (elevator.open && input.down(Down)) {
        floor = uint8_t(elevator.stop);
        inLift = false;
        x = 240;
        crouching = false;
      }
    } else if (ride) {
      --ride;
      x += floor % 2 ? .55f : -.55f;
      if (!ride) {
        ++floor;
        jump = 0;
      }
    } else {
      crouching = input.down(Down) && jump == 0;
      int dx = int(input.down(Right)) - int(input.down(Left));
      if (dx)
        facing = dx;
      if (!crouching)
        x = clamp(x + dx * 1.45f, 12.f, 468.f);
      if (input.hit(Fire) && !jump && !crouching) {
        jumpVelocity = 2.9f;
        jump = .1f;
        sound(Sound::Move);
      }
      if (jump > 0) {
        jump += jumpVelocity;
        jumpVelocity -= .19f;
        if (jump <= 0)
          jump = jumpVelocity = 0;
      }
      if (floor < 3 && std::abs(x - 240) < 8 && elevator.open && elevator.stop == floor &&
          input.down(Up) && jump == 0) {
        inLift = true;
        x = 240;
      }
      if (floor < 3 && !inLift && (floor % 2 ? x <= 32 : x >= 448)) {
        ride = 60;
        jump = 0;
        crouching = false;
      }
    }
    thief();
    if (transition || ended)
      return;
    if (!inLift && !ride && floor == thiefFloor && std::abs(x - thiefX) < 8 && jump < 10) {
      arrest();
      return;
    }
    float tier = float(std::min(16, int(level)));
    for (auto &h : hazards)
      if (h.active) {
        if (h.kind == Ball || h.kind == Cart || h.kind == Biplane) {
          float speed = h.kind == Ball ? .60f : h.kind == Cart ? .7f : 1.15f;
          h.x += h.direction * (speed + tier * .07f);
          if (h.x < 8)
            h.x = 472;
          if (h.x > 472)
            h.x = 8;
        }
        if (inLift || ride || h.floor != floor)
          continue;
        float objectBottom = float(ground(floor));
        int objectHeight = 9, width = 8;
        if (h.kind == Ball) {
          objectBottom -=
              std::abs(std::sin((roundTick + h.floor * 40) * .055f)) * (level >= 6 ? 24 : 9);
          objectHeight = 7;
        }
        if (h.kind == Biplane) {
          objectBottom -= 13;
          objectHeight = 6;
          width = 13;
        }
        if (h.kind == Cart)
          width = 12;
        float feet = ground(floor) - jump;
        int bodyHeight = crouching ? 9 : 20;
        if (overlaps({x - 3, feet - bodyHeight, 6, float(bodyHeight)},
                     {h.x - width / 2.f, objectBottom - objectHeight, float(width),
                      float(objectHeight)})) {
          if (h.kind == Money || h.kind == Suitcase) {
            h.active = false;
            reward(50);
            sound(Sound::Power);
          } else if (!grace) {
            if (h.kind == Biplane) {
              lose();
              return;
            }
            seconds = std::max(0, seconds - 540);
            grace = 60;
            sound(Sound::Hit);
            if (!seconds) {
              lose();
              return;
            }
          }
        }
      }
  }
  void drawHazard(Canvas &c, const Hazard &h, int camera) const {
    int xx = int(h.x) - camera + 8, yy = ground(h.floor);
    if (xx < 8 || xx > 168)
      return;
    switch (h.kind) {
    case Ball:
      yy -= int(std::abs(std::sin((roundTick + h.floor * 40) * .055f)) * (level >= 6 ? 24 : 9));
      atari::mask(c, xx - 4, yy - 7,
                  std::array<uint8_t, 7>{{0x3c, 0x7e, 0xff, 0xff, 0xff, 0x7e, 0x3c}}, 0xcb24);
      c.rect(xx - 3, yy - 5, 6, 1, 0xec6a);
      break;
    case Radio:
      atari::mask(c, xx - 4, yy - 12,
                  std::array<uint8_t, 12>{
                      {0x18, 0x3c, 0x7e, 0x7e, 0xff, 0xff, 0xff, 0xdb, 0xdb, 0xff, 0xff, 0xff}},
                  0xa325);
      c.rect(xx - 2, yy - 5, 4, 3, Black);
      break;
    case Cart:
      atari::mask(c, xx - 4, yy - 12,
                  std::array<uint8_t, 12>{
                      {0xc0, 0x40, 0x7f, 0x55, 0x55, 0x55, 0x7f, 0x02, 0x7e, 0x00, 0x42, 0x42}},
                  0xd6da, h.direction < 0);
      break;
    case Biplane:
      atari::mask(c, xx - 8, yy - 19,
                  std::array<uint8_t, 7>{{0x3c, 0x18, 0xff, 0x7e, 0xff, 0x18, 0x3c}}, White,
                  h.direction < 0, 2);
      break;
    case Money:
      atari::mask(c, xx - 4, yy - 9,
                  std::array<uint8_t, 9>{{0x18, 0x3c, 0x18, 0x3c, 0x7e, 0xff, 0xff, 0x7e, 0x3c}},
                  atari::Gold);
      break;
    case Suitcase:
      atari::mask(
          c, xx - 4, yy - 10,
          std::array<uint8_t, 10>{{0x3c, 0x24, 0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff, 0xff}},
          atari::Gold);
      break;
    }
  }

public:
  void start(const GameOptions &o) override {
    baseStart(o);
    health = 4;
    transition = 0;
    setupRound();
  }
  uint16_t stageLimit() const override {
    return 0;
  }
  bool ownsHud() const override {
    return true;
  }
  size_t activeEntities() const override {
    size_t n = 3;
    for (const auto &h : hazards)
      n += h.active;
    return n;
  }
  void render(Canvas &c) const override {
    c.clear(Black);
    int camera = inLift ? 160 : clamp(int(x) / 160, 0, 2) * 160;
    c.rect(8, 8, 160, 25, atari::Blue);
    atari::digits(c, 92, 11, points % 1000000, 6, White);
    if (seconds > 600 || roundTick % 30 < 15)
      atari::digits(c, 60, 17, unsigned(std::max(0, seconds)) / 60, 2, Black);
    for (int i = 0; i < health - 1; ++i)
      atari::mask(c, 16 + i * 12, 23, std::array<uint8_t, 5>{{0x18, 0x3c, 0x3c, 0x7e, 0x7e}},
                  Black);
    c.rect(8, 33, 160, 20, 0x7370);
    for (int i = 0; i < 10; ++i) {
      c.rect(8 + i * 16, 33, 8, 2, 0xb349);
      c.rect(12 + i * 16, 35, 8, 2, atari::Gold);
    }
    for (int f = 0; f < 3; ++f) {
      int yy = ground(f);
      c.rect(8, yy - 38, 160, 38, atari::Grass);
      for (int col = 0; col < 4; ++col)
        c.rect(28 + col * 36, yy - 38, 9, 38, 0xce59);
      c.rect(8, yy, 160, 4, 0xb525);
    }
    // Escalators only lead upwards. The roof is deliberately outside the lift route.
    for (int f = 0; f < 3; ++f) {
      int start = f % 2 ? 32 : 448, finish = start + (f % 2 ? 32 : -32);
      int xx = start - camera + 8, ex = finish - camera + 8;
      if (xx >= 8 && xx <= 168) {
        c.line(xx, ground(f), ex, ground(f + 1), atari::Gold);
        c.line(xx + 3, ground(f), ex + 3, ground(f + 1), Black);
        for (int j = 0; j < 8; ++j) {
          int sx = xx + (ex - xx) * j / 8;
          int sy = ground(f) + (ground(f + 1) - ground(f)) * j / 8;
          c.line(sx, sy, sx + 5, sy, atari::Gold);
        }
      }
    }
    Lift elevator = lift();
    if (camera == 160) {
      c.rect(79, 58, 18, 104, 0xce59);
      int yy = int(ground(0) + (ground(2) - ground(0)) * elevator.floor / 2);
      c.rect(79, yy - 30, 18, 30, elevator.open ? atari::DarkGrass : 0xad55);
      if (!elevator.open)
        c.line(88, yy - 30, 88, yy - 1, 0x5acb);
      if (inLift)
        atari::person(c, 88, yy, true, false, facing < 0, roundTick / 8);
    }
    for (const auto &h : hazards)
      if (h.active)
        drawHazard(c, h, camera);
    if (int(thiefX) / 160 == camera / 160)
      atari::person(c, int(thiefX) - camera + 8, ground(thiefFloor), false, false,
                    thiefDirection < 0, roundTick / 8);
    if (!inLift && (!grace || tick % 8 < 4)) {
      float feet = ground(floor) - jump;
      if (ride)
        feet = ground(floor) + (ground(floor + 1) - ground(floor)) * (60 - ride) / 60.f;
      atari::person(c, int(x) - camera + 8, int(feet), true, crouching, facing < 0, roundTick / 8);
    }
    c.rect(8, 169, 160, 32, 0xad55);
    for (int f = 0; f < 4; ++f) {
      int yy = 197 - f * 7;
      c.rect(40, yy - 4, 96, 4, atari::Grass);
      c.line(40, yy, 136, yy, atari::Gold);
      if (f < 3) {
        int ex = f % 2 ? 40 : 132;
        c.line(ex, yy, ex + (f % 2 ? 4 : -4), yy - 7, Black);
      }
    }
    c.rect(86, int(197 - elevator.floor * 7) - 4, 5, 4, 0xce59);
    c.rect(40 + int(x / 5), 197 - floor * 7 - 3, 2, 3, Black);
    c.rect(40 + int(thiefX / 5), 197 - thiefFloor * 7 - 3, 2, 3, White);
    atari::credit(c, 207);
    if (transition)
      c.centered(99, transition > 60 ? "ARREST!" : "READY", White);
  }
};
GameEntry describeKeystoneKapers() {
  return gameEntry<KeystoneKapers>(
      {"KEYSTONE KAPERS",
       "CATCH HARRY HOOLIGAN",
       {"L/R RUN  A JUMPS", "DOWN DUCKS UNDER PLANES", "UP ENTERS OPEN LIFT",
        "DOWN EXITS LIFT AT STOP", "ESCALATORS ONLY GO UP", "CATCH KROOK BEFORE 50 SEC"},
       atari::Blue,
       50,
       Left | Right | Up | Down | Fire});
}
#ifdef ARCADE_HOST_TESTS
bool verifyKeystoneKapers() {
  KeystoneKapers g;
  g.start({});
  HOST_CHECK(g.health == 4 && g.seconds == 3000 && g.stageLimit() == 0 && g.ownsHud());
  auto time = g.seconds;
  g.pause();
  for (int i = 0; i < 120; ++i)
    g.update();
  HOST_CHECK(g.seconds == time && g.roundTick == 0);
  g.resume();
  g.handle({Fire, Fire});
  g.update();
  HOST_CHECK(g.jump > 0);
  for (int i = 0; i < 40; ++i)
    g.update();
  HOST_CHECK(g.jump == 0);
  g.start({});
  g.x = 240;
  g.handle({Up, Up});
  g.update();
  HOST_CHECK(g.inLift);
  g.roundTick = 130;
  g.handle({Down, Down});
  g.update();
  HOST_CHECK(g.inLift);
  g.roundTick = 192;
  g.handle({Down, Down});
  g.update();
  HOST_CHECK(!g.inLift && g.floor == 1);
  g.start({});
  g.x = 448;
  g.update();
  HOST_CHECK(g.ride == 60);
  for (int i = 0; i < 60; ++i)
    g.update();
  HOST_CHECK(g.floor == 1 && !g.ride);
  g.start({});
  g.floor = 3;
  g.x = 240;
  g.handle({Up, Up});
  g.update();
  HOST_CHECK(!g.inLift);
  // Catch awards 100/200/300 times whole seconds, with transitions beyond stage twelve.
  for (unsigned stage : {1u, 8u, 9u, 16u, 17u, 100u}) {
    g.start({});
    g.level = uint16_t(stage);
    g.seconds = 1200;
    g.floor = g.thiefFloor = 1;
    g.x = g.thiefX = 200;
    g.update();
    HOST_CHECK(g.score() == 19 * (stage <= 8 ? 100u : stage <= 16 ? 200u : 300u));
    HOST_CHECK(g.stage() == stage + 1 && !g.gameOver() && g.transition == 90);
  }
  g.start({});
  for (auto &h : g.hazards)
    h.active = false;
  g.hazards[0] = {g.x, 0, KeystoneKapers::Radio, 0, true};
  g.update();
  HOST_CHECK(g.seconds == 2459 && g.health == 4 && g.grace == 60);
  g.update();
  HOST_CHECK(g.seconds == 2458);
  g.start({});
  for (auto &h : g.hazards)
    h.active = false;
  g.hazards[0] = {g.x, 0, KeystoneKapers::Money, 0, true};
  g.update();
  HOST_CHECK(g.score() == 50 && !g.hazards[0].active);
  g.start({});
  for (auto &h : g.hazards)
    h.active = false;
  g.hazards[0] = {g.x, 0, KeystoneKapers::Biplane, 1, true};
  g.handle({Down, Down});
  g.update();
  HOST_CHECK(g.health == 4);
  g.handle({});
  g.update();
  HOST_CHECK(g.health == 3);
  g.start({});
  g.seconds = 1;
  g.update();
  HOST_CHECK(g.health == 3 && g.seconds == 3000);
  g.start({});
  g.health = 2;
  g.reward(10000);
  HOST_CHECK(g.health == 3);
  g.reward(20000);
  HOST_CHECK(g.health == 4);
  g.start({});
  g.floor = 3;
  g.x = 100;
  g.thiefFloor = 3;
  g.thiefX = 12.1f;
  g.update();
  HOST_CHECK(g.health == 3); // Harry exits the roof before Kelly can arrest him.
  for (unsigned tier = 1; tier <= 32; ++tier) {
    g.start({});
    g.level = uint16_t(tier);
    g.setupRound();
    HOST_CHECK(g.activeEntities() <= 33 && !g.campaignComplete());
  }
  GameOptions o;
  o.difficulty = 3;
  g.start(o);
  HOST_CHECK(g.health == 4);
  g.health = 1;
  g.seconds = 1;
  g.update();
  HOST_CHECK(g.gameOver());
  g.start({});
  HOST_CHECK(g.points == 0 && g.level == 1 && !g.inLift && !g.transition);
  return true;
}
#endif
} // namespace arcade
