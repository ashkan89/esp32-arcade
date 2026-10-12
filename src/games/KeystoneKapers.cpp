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
  float rideOriginX = 0, rideHeight = 0;
  float walkDistance = 0, thiefWalkDistance = 0;
  static constexpr float RunSpeed = 1.125f;
  int rideDuration = 60, jumpBuffer = 0;
  bool inLift = false, crouching = false, running = false, caught = false;
  int thiefCommit = 0;
  int thiefRide = 0;
  uint8_t thiefTargetFloor = 0;
  bool thiefInLift = false, thiefSeekLift = false;
  struct Lift {
    float floor = 0;
    bool open = false;
    int stop = 0;
  };
  static int ground(int f) {
    static constexpr int rows[] = {162, 126, 90, 54};
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
    grace = ride = jumpBuffer = 0;
    rideOriginX = rideHeight = 0;
    walkDistance = thiefWalkDistance = 0;
    rideDuration = 60;
    inLift = crouching = running = false;
    roundTick = 0;
    thiefCommit = 0;
    thiefRide = 0;
    thiefTargetFloor = 0;
    thiefInLift = thiefSeekLift = false;
    for (auto &h : hazards)
      h.active = false;
    int n = 0;
    unsigned tier = std::min(16u, unsigned(level));
    for (int f = 0; f < 3; ++f) {
      auto put = [&](Kind k, float xx, int dir) {
        hazards[n++] = {xx, uint8_t(f), k, int8_t(dir), true};
      };
      int approach = options.enhanced && f % 2 ? 1 : -1;
      int planeDirection = options.enhanced ? approach : 1;
      put(Ball, 132 + f * 80, approach);
      if (tier >= 2)
        put(Radio, 208 + f * 32, 0);
      if (tier >= 3)
        put(Cart, 320 + f * 24, approach);
      if (tier >= 4)
        put(Biplane, 440 - f * 64, planeDirection);
      if (tier >= 7)
        put(Cart, (options.enhanced ? 248 : 360) + f * 24, approach);
      if (tier >= 10)
        put(Biplane, (options.enhanced ? 300 : 404) - f * 64, planeDirection);
      if (tier >= 13)
        put(Cart, (options.enhanced ? 176 : 400) + f * 24, approach);
      if (tier >= 16)
        put(Biplane, (options.enhanced ? 160 : 368) - f * 64, planeDirection);
      put(Money, 64 + f * 80, 0);
      put(Suitcase, 296 - f * 48, 0);
    }
  }
  void lose() {
    if (ended || transition)
      return;
    sound(Sound::Hit);
    caught = false;
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
    caught = true;
  }
  float thiefVisualFloor() const {
    if (options.enhanced && thiefInLift)
      return lift().floor;
    if (options.enhanced && thiefRide)
      return thiefFloor + (thiefTargetFloor - thiefFloor) * (60 - thiefRide) / 60.f;
    return float(thiefFloor);
  }
  int liftWait(int f) const {
    Lift elevator = lift();
    if (elevator.open && elevator.stop == f)
      return 0;
    static constexpr int stops[] = {0, 1, 2, 1};
    int wait = 768;
    for (int i = 0; i < 4; ++i)
      if (stops[i] == f)
        wait = std::min(wait, (i * 192 + 768 - int(roundTick % 768)) % 768);
    return wait;
  }
  int viewCamera() const {
    return inLift             ? 160
           : options.enhanced ? clamp(int(x) - 80, 0, 320)
                              : clamp(int(x) / 160, 0, 2) * 160;
  }
  float ridingFeet() const {
    float progress = float(rideDuration - ride) / rideDuration;
    return ground(floor) - rideHeight + (rideHeight - 36) * progress;
  }
  void enhancedThief() {
    Lift elevator = lift();
    if (thiefInLift) {
      if (elevator.open && elevator.stop == thiefTargetFloor) {
        thiefFloor = thiefTargetFloor;
        thiefInLift = false;
        thiefCommit = 0;
      }
      return;
    }
    if (thiefRide) {
      --thiefRide;
      int start = thiefFloor % 2 ? 32 : 448;
      thiefX = start + (thiefFloor % 2 ? 32.f : -32.f) * (60 - thiefRide) / 60.f;
      if (!thiefRide) {
        thiefFloor = thiefTargetFloor;
        thiefCommit = 0;
      }
      return;
    }
    if (--thiefCommit <= 0) {
      float officerPath = inLift ? 240 + elevator.floor * 480 : path(x, floor);
      bool intercepted = officerPath > path(thiefX, thiefFloor);
      thiefSeekLift =
          intercepted && thiefFloor > 0 && thiefFloor < 3 && liftWait(thiefFloor) <= 120;
      thiefDirection = thiefSeekLift                    ? (thiefX > 240 ? -1 : 1)
                       : intercepted && thiefFloor == 3 ? 1
                                                        : (thiefFloor % 2 ? -1 : 1);
      thiefCommit = 36 + rng.range(0, 25);
    }
    float speed = .38f + std::min(15, int(level) - 1) * .008f;
    if (thiefSeekLift) {
      thiefX += clamp(240 - thiefX, -speed, speed);
      if (thiefX == 240 && elevator.open && elevator.stop == thiefFloor) {
        thiefX = 240;
        thiefInLift = true;
        thiefTargetFloor = thiefFloor - 1;
        thiefSeekLift = false;
      }
      return;
    }
    thiefX += thiefDirection * speed;
    if (thiefFloor < 3 && (thiefFloor % 2 ? thiefX <= 32 : thiefX >= 448)) {
      thiefX = thiefFloor % 2 ? 32.f : 448.f;
      thiefTargetFloor = thiefFloor + 1;
      thiefRide = 60;
    } else if (thiefFloor == 3 && thiefX < 12) {
      lose();
    } else if (thiefX > 468 || thiefX < 12) {
      thiefX = clamp(thiefX, 12.f, 468.f);
      thiefDirection = -thiefDirection;
      thiefCommit = 36;
    }
  }
  void thief() {
    if (options.enhanced) {
      enhancedThief();
      return;
    }
    // Intercepting ahead of Harry sends him back toward the lower floors.
    float officerPath = inLift ? 240 + lift().floor * 480 : path(x, floor);
    if (!options.enhanced || --thiefCommit <= 0) {
      thiefDirection = officerPath > path(thiefX, thiefFloor) ? (thiefFloor % 2 ? 1 : -1)
                                                              : (thiefFloor % 2 ? -1 : 1);
      thiefCommit = options.enhanced ? 36 + rng.range(0, 25) : 0;
    }
    float speed = .38f + std::min(15, int(level) - 1) * .008f;
    thiefX += thiefDirection * speed;
    if (thiefX < 12 || thiefX > 468) {
      thiefCommit = 0;
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
    float oldX = x;
    bool wasGrounded = !inLift && !ride && jump == 0;
    running = false;
    if (options.enhanced && input.hit(Fire))
      jumpBuffer = 6;
    else if (jumpBuffer)
      --jumpBuffer;
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
      int finish = floor % 2 ? 64 : 416;
      x = rideOriginX + (finish - rideOriginX) * (rideDuration - ride) / rideDuration;
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
        x = clamp(x + dx * RunSpeed, 12.f, 468.f);
      if ((input.hit(Fire) || jumpBuffer) && !jump && !crouching) {
        jumpBuffer = 0;
        jumpVelocity = floor == 3 ? 1.9f : 2.9f;
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
        int rise = ground(floor) - ground(floor + 1);
        ride = clamp(60 - int(jump * 60 / rise), 1, 60);
        rideDuration = ride;
        rideOriginX = x;
        rideHeight = jump;
        jumpBuffer = 0;
        jump = 0;
        jumpVelocity = 0;
        crouching = false;
      }
    }
    running = wasGrounded && !inLift && !ride && jump == 0 && !crouching && x != oldX;
    if (running) {
      int oldContact = int((walkDistance + 8) / 16);
      walkDistance += std::abs(x - oldX);
      if (int((walkDistance + 8) / 16) != oldContact)
        sound(Sound::Footstep);
      if (walkDistance >= 32)
        walkDistance -= 32;
    } else
      walkDistance = 0;
    float oldThiefX = thiefX;
    int oldThiefFloor = thiefFloor;
    bool thiefWasWalking = !thiefRide && !thiefInLift;
    thief();
    if (transition || ended)
      return;
    if (thiefWasWalking && !thiefRide && !thiefInLift && thiefFloor == oldThiefFloor &&
        thiefX != oldThiefX) {
      thiefWalkDistance += std::abs(thiefX - oldThiefX);
      if (thiefWalkDistance >= 32)
        thiefWalkDistance -= 32;
    } else
      thiefWalkDistance = 0;
    bool sameFloor = options.enhanced && thiefInLift
                         ? inLift
                         : !inLift && !ride && !thiefRide && floor == thiefFloor;
    if (sameFloor && std::abs(x - thiefX) < 8 && jump < 10) {
      arrest();
      return;
    }
    float tier = float(std::min(16, int(level)));
    for (auto &h : hazards)
      if (h.active) {
        if (h.kind == Ball || h.kind == Cart || h.kind == Biplane) {
          float speed = h.kind == Ball ? .60f : h.kind == Cart ? .7f : 1.15f;
          h.x += h.direction * (speed + tier * .07f);
          // Leave the complete sprite offscreen before returning at the other edge.
          if (h.x < -12)
            h.x += 504;
          if (h.x > 492)
            h.x -= 504;
          if (h.floor == floor && h.kind == Biplane && std::abs(h.x - x) < 70 &&
              (h.x - x) * h.direction < 0 && !inLift && !ride && tick % 45 == 0)
            sound(Sound::Warning);
        }
        if (inLift || ride || h.floor != floor)
          continue;
        // The roof has less headroom beneath the scoreboard than the shop floors.
        int ceiling = floor < 3 ? ground(floor + 1) + 4 : 33;
        float feet = ground(floor) - std::min(jump, float(ground(floor) - ceiling - 8));
        int bodyHeight = crouching ? 9 : 20;
        if (overlaps({x - 3, feet - bodyHeight, 6, float(bodyHeight)}, hazardBounds(h))) {
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
  Rect hazardBounds(const Hazard &h) const {
    float bottom = float(ground(h.floor));
    if (h.kind == Ball)
      bottom -= int(std::abs(std::sin((roundTick + h.floor * 40) * .055f)) * (level >= 6 ? 24 : 9));
    if (h.kind == Biplane)
      return {h.x - 8, bottom - 19, 16, 7};
    int height = h.kind == Ball ? 7 : h.kind == Money ? 9 : h.kind == Suitcase ? 10 : 12;
    return {h.x - 4, bottom - height, 8, float(height)};
  }
  void drawHazard(Canvas &c, const Hazard &h, int camera) const {
    int xx = int(h.x) - camera + 8, yy = ground(h.floor);
    int halfWidth = h.kind == Biplane ? 8 : 4;
    if (xx + halfWidth <= 8 || xx - halfWidth >= 168)
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
      c.pixel(xx + 2, yy - 7, atari::Gold);
      break;
    case Cart:
      atari::mask(c, xx - 4, yy - 12,
                  std::array<uint8_t, 12>{
                      {0xc0, 0x40, 0x7f, 0x55, 0x55, 0x55, 0x7f, 0x02, 0x7e, 0x00, 0x42, 0x42}},
                  0xd6da, h.direction < 0);
      break;
    case Biplane:
      atari::mask(c, xx - 8, yy - 19,
                  std::array<uint8_t, 7>{{0x3c, 0x18, 0xff, 0x7e, 0xff, 0x18, 0x3c}}, atari::Gold,
                  h.direction < 0, 2);
      c.line(xx + (h.direction < 0 ? -8 : 7), yy - 19 + int(roundTick % 3),
             xx + (h.direction < 0 ? -8 : 7), yy - 13, White);
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
    caught = false;
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
    int camera = viewCamera();
    c.rect(8, 8, 160, 25, atari::Blue);
    atari::digits(c, 92, 11, points % 1000000, 6, White);
    if (seconds > 600 || roundTick % 30 < 15)
      atari::digits(c, 60, 17, unsigned(std::max(0, seconds)) / 60, 2, Black);
    for (int i = 0; i < health - 1; ++i)
      atari::mask(c, 16 + i * 12, 23, std::array<uint8_t, 5>{{0x18, 0x3c, 0x3c, 0x7e, 0x7e}},
                  Black);
    c.rect(8, 33, 160, ground(3) - 33, 0x7370);
    for (int i = 0; i < 11; ++i) {
      c.rect(8 + i * 16 - camera % 16, 33, 8, 2, 0xb349);
      c.rect(12 + i * 16 - camera % 16, 35, 8, 2, atari::Gold);
    }
    c.rect(8, ground(3), 160, 4, atari::Gold);
    for (int f = 0; f < 3; ++f) {
      int yy = ground(f);
      int ceiling = ground(f + 1) + 4;
      c.rect(8, ceiling, 160, yy - ceiling, atari::Grass);
      if (f == 1) {
        // Blue display windows distinguish the middle storey in the original.
        for (int col = 0; col < 3; ++col) {
          c.rect(24 + col * 80 - camera % 80, yy - 17, 42, 9, atari::Blue);
          c.rect(24 + col * 80 - camera % 80, yy - 18, 42, 1, atari::DarkGrass);
          c.rect(24 + col * 80 - camera % 80, yy - 8, 42, 1, 0xce59);
        }
      } else
        for (int col = 0; col < 5; ++col) {
          c.rect(28 + col * 36 - camera % 36, ceiling, 9, yy - ceiling, 0xce59);
          c.rect(37 + col * 36 - camera % 36, ceiling, 1, yy - ceiling, atari::DarkGrass);
        }
      c.rect(8, yy, 160, 4, 0xb525);
      c.rect(8, yy + 3, 160, 1, atari::DarkGrass);
    }
    // Escalators only lead upwards. The roof is deliberately outside the lift route.
    for (int f = 0; f < 3; ++f) {
      int start = f % 2 ? 32 : 448, finish = start + (f % 2 ? 32 : -32);
      int xx = start - camera + 8, ex = finish - camera + 8;
      if (std::min(xx, ex) <= 168 && std::max(xx, ex) >= 8) {
        c.line(xx, ground(f), ex, ground(f + 1), atari::Gold);
        c.line(xx + 3, ground(f), ex + 3, ground(f + 1), Black);
        for (int j = 0; j < 8; ++j) {
          int phase = j * 8 + int(roundTick / 3 % 8);
          int sx = xx + (ex - xx) * phase / 64;
          int sy = ground(f) + (ground(f + 1) - ground(f)) * phase / 64;
          c.line(sx, sy, sx + 5, sy, atari::Gold);
        }
      }
    }
    Lift elevator = lift();
    int liftOffset = 160 - camera;
    if (camera > 60 && camera < 268) {
      c.rect(79 + liftOffset, ground(3) + 4, 18, ground(0) - ground(3) - 4, 0xce59);
      for (int f = 0; f < 3; ++f) {
        int yy = ground(f);
        c.rect(78 + liftOffset, yy - 24, 20, 24, atari::DarkGrass, false);
        c.rect(99 + liftOffset, yy - 17, 2, 3,
               elevator.open && elevator.stop == f ? Yellow : atari::Gold);
      }
      int yy = int(ground(0) + (ground(2) - ground(0)) * elevator.floor / 2);
      c.rect(79 + liftOffset, yy - 24, 18, 24, elevator.open ? atari::DarkGrass : 0xad55);
      c.rect(79 + liftOffset, yy - 24, 18, 1, atari::Gold);
      c.rect(79 + liftOffset, yy - 1, 18, 1, atari::Gold);
      if (!elevator.open) {
        c.line(88 + liftOffset, yy - 24, 88 + liftOffset, yy - 1, 0x5acb);
        c.rect(81 + liftOffset, yy - 21, 5, 16, 0xce59);
        c.rect(90 + liftOffset, yy - 21, 5, 16, 0xce59);
      }
      if (inLift)
        atari::person(c, 88 + liftOffset, yy, true, false, facing < 0, 0);
    }
    for (const auto &h : hazards)
      if (h.active) {
        drawHazard(c, h, camera);
        if (options.enhanced && h.kind == Biplane && !inLift && !ride && h.floor == floor &&
            (h.x - x) * h.direction < 0 && std::abs(h.x - x) < 110 &&
            (h.x < camera || h.x > camera + 160)) {
          int edge = h.direction > 0 ? 10 : 165;
          int yy = ground(h.floor) - 16;
          c.line(edge, yy - 3, edge + h.direction * 3, yy, Yellow);
          c.line(edge, yy + 3, edge + h.direction * 3, yy, Yellow);
        }
      }
    if (thiefX - camera >= -4 && thiefX - camera <= 164)
      atari::person(c, int(thiefX) - camera + 8, int(ground(0) - 36 * thiefVisualFloor()), false,
                    false, thiefDirection < 0,
                    thiefWalkDistance > 0 ? 1 + unsigned(thiefWalkDistance / 4) : 0, false, 0,
                    true);
    if (!inLift && (!grace || tick % 8 < 4)) {
      // The roof has less headroom beneath the scoreboard than the shop floors.
      int ceiling = floor < 3 ? ground(floor + 1) + 4 : 33;
      float feet = ground(floor) - std::min(jump, float(ground(floor) - ceiling - 8));
      if (ride)
        feet = ridingFeet();
      atari::person(c, int(x) - camera + 8, int(feet), true, crouching, facing < 0,
                    running ? 1 + unsigned(walkDistance / 4) : 0, jump > 0,
                    ride        ? 0
                    : floor < 3 ? ground(floor + 1) + 4
                                : 33,
                    true);
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
    float mapFloor =
        inLift ? elevator.floor : floor + (ride ? (ground(floor) - ridingFeet()) / 36.f : 0);
    c.rect(40 + int(x / 5), int(197 - mapFloor * 7) - 3, 2, 3, Black);
    c.rect(40 + int(thiefX / 5), int(197 - thiefVisualFloor() * 7) - 3, 2, 3, White);
    atari::credit(c, 207);
    if (transition)
      c.centered(99, caught ? "ARREST!" : "READY", White);
    // Sprites entering a paged view stay inside the Atari playfield gutters.
    c.rect(0, 0, 8, H, Black);
    c.rect(168, 0, 8, H, Black);
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
  GameOptions physical;
  physical.enhanced = true;
  physical.seed = 9;
  g.start(physical);
  g.thiefFloor = 2;
  g.thiefX = 448;
  g.thiefCommit = 0;
  g.floor = 0;
  g.x = 24;
  g.thief();
  HOST_CHECK(g.thiefRide == 60 && g.thiefFloor == 2);
  for (int t = 0; t < 30; ++t)
    g.thief();
  HOST_CHECK(g.thiefFloor == 2 && g.thiefVisualFloor() == 2.5f && g.thiefX == 432);
  for (int t = 0; t < 30; ++t)
    g.thief();
  HOST_CHECK(g.thiefFloor == 3 && g.thiefX == 416 && !g.thiefRide);
  g.thiefFloor = 1;
  g.thiefX = 240;
  g.floor = 2;
  g.x = 240;
  g.roundTick = 192;
  g.thiefCommit = 0;
  g.thief();
  HOST_CHECK(g.thiefInLift && g.thiefTargetFloor == 0);
  g.roundTick = 348;
  g.thief();
  HOST_CHECK(g.thiefFloor == 1 && g.thiefVisualFloor() > 1);
  g.roundTick = 768;
  g.thief();
  HOST_CHECK(!g.thiefInLift && g.thiefFloor == 0);
  g.start(physical);
  g.thiefInLift = true;
  g.thiefTargetFloor = 0;
  g.thiefX = 240;
  g.inLift = true;
  g.x = 240;
  g.roundTick = 193;
  g.update();
  HOST_CHECK(g.caught && g.transition == 90 && g.level == 2);
  g.start({});
  // Regression checks for the gameplay audit.
  GameOptions enhanced;
  enhanced.enhanced = true;
  enhanced.seed = 42;
  g.start(enhanced);
  g.thief();
  int committed = g.thiefDirection;
  g.floor = 3;
  g.x = 460;
  for (int t = 0; t < 20; ++t) {
    g.thief();
    HOST_CHECK(g.thiefDirection == committed);
  }
  for (int t = 0; t < 2400; ++t) {
    float before = g.thiefX;
    int beforeFloor = g.thiefFloor;
    int previousRide = g.thiefRide;
    bool previousLift = g.thiefInLift;
    ++g.roundTick;
    g.thief();
    HOST_CHECK(g.thiefFloor < 4 && g.thiefX >= 12 && g.thiefX <= 468);
    if (g.thiefFloor != beforeFloor)
      HOST_CHECK(previousRide == 1 || previousLift || g.transition);
    else
      HOST_CHECK(std::abs(before - g.thiefX) < 1 || g.transition);
    if (g.transition || g.ended)
      break;
  }
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
  // Jumping onto an escalator preserves height and shortens the ascent.
  g.x = 448;
  g.jump = 18;
  g.jumpVelocity = 0;
  g.update();
  HOST_CHECK(g.ride > 0 && g.ride < 60 && g.jump == 0 && g.jumpVelocity == 0);
  HOST_CHECK(g.x == 448 && std::abs(g.ridingFeet() - 144) < .01f);
  while (g.ride)
    g.update();
  HOST_CHECK(g.floor == 1 && std::abs(g.x - 416) < .01f);
  g.start({});
  for (auto &h : g.hazards)
    h.active = false;
  // Contact with the drawn radio's upper rows used to be missed.
  g.jump = 10;
  g.jumpVelocity = 0;
  g.hazards[0] = {g.x, 0, KeystoneKapers::Radio, 0, true};
  g.update();
  HOST_CHECK(g.grace == 60);
  g.start({});
  g.inLift = true;
  g.x = 240;
  g.roundTick = 384;
  g.thiefFloor = 1;
  g.thiefX = 240;
  g.thief();
  HOST_CHECK(g.thiefDirection == 1); // Harry reacts to Kelly's actual lift floor
  // Grounded stride contacts follow distance, never the global animation clock.
  for (bool enhancedMode : {false, true}) {
    GameOptions walking;
    walking.enhanced = enhancedMode;
    g.start(walking);
    for (auto &h : g.hazards)
      h.active = false;
    g.x = 100;
    g.thiefFloor = 3;
    g.thiefX = 400;
    g.handle({Right, Right});
    unsigned contacts = 0;
    for (int t = 1; t <= 60; ++t) {
      g.update();
      Sound effect = g.takeSound();
      bool contact = t == 8 || t == 22 || t == 36 || t == 50;
      HOST_CHECK((effect == Sound::Footstep) == contact);
      contacts += effect == Sound::Footstep;
      HOST_CHECK(g.running && g.x == 100 + t * 1.125f);
    }
    HOST_CHECK(contacts == 4 && g.walkDistance == 3.5f);
    g.handle({});
    float stoppedX = g.x;
    for (int t = 0; t < 20; ++t) {
      g.update();
      HOST_CHECK(g.x == stoppedX && !g.running && g.walkDistance == 0);
      HOST_CHECK(g.takeSound() != Sound::Footstep);
    }
    g.handle({Left, Left});
    contacts = 0;
    for (int t = 0; t < 32; ++t) {
      g.update();
      contacts += g.takeSound() == Sound::Footstep;
      HOST_CHECK(g.running && g.facing == -1);
    }
    HOST_CHECK(g.x == stoppedX - 36 && contacts == 2);
    g.handle({});
    g.update();
    g.takeSound();
    g.handle({Right | Fire, Fire});
    g.update();
    HOST_CHECK(g.jump > 0 && !g.running && g.walkDistance == 0);
    HOST_CHECK(g.takeSound() != Sound::Footstep);
    while (g.jump > 0) {
      g.handle({Right, 0});
      g.update();
      HOST_CHECK(!g.running && g.takeSound() != Sound::Footstep);
    }
    g.handle({Down, Down});
    g.update();
    HOST_CHECK(g.crouching && g.takeSound() != Sound::Footstep);
    g.floor = 3;
    g.x = 468;
    g.thiefFloor = 2;
    g.handle({Right, Right});
    for (int t = 0; t < 20; ++t) {
      g.update();
      HOST_CHECK(g.x == 468 && !g.running && g.takeSound() != Sound::Footstep);
    }
    g.start(walking);
    g.x = 240;
    g.handle({Up, Up});
    g.update();
    HOST_CHECK(g.inLift && g.takeSound() != Sound::Footstep);
    g.start(walking);
    g.x = 448;
    g.handle({Right, Right});
    g.update();
    HOST_CHECK(g.ride && !g.running && g.takeSound() != Sound::Footstep);
  }
  // Reduced speed still permits a normal chase before the timer expires.
  for (bool enhancedMode : {false, true})
    for (unsigned stage : {1u, 8u, 16u, 32u}) {
      GameOptions walking;
      walking.enhanced = enhancedMode;
      g.start(walking);
      g.level = uint16_t(stage);
      g.setupRound();
      for (auto &h : g.hazards)
        h.active = false;
      for (int t = 0; t < 3000 && !g.transition; ++t) {
        uint16_t direction = g.floor % 2 ? Left : Right;
        if (g.floor == g.thiefFloor)
          direction = g.x < g.thiefX ? Right : Left;
        g.handle({direction, 0});
        g.update();
      }
      if (!g.caught)
        std::fprintf(
            stderr, "CHASE mode=%d stage=%u officer=%u/%.1f thief=%u/%.1f time=%d health=%d\n",
            enhancedMode, stage, g.floor, g.x, g.thiefFloor, g.thiefX, g.seconds, g.health);
      HOST_CHECK(g.caught && g.level == stage + 1 && g.health == 4);
    }
  // Enhanced hazards approach along each floor's route without homing or instant reversals.
  g.start(enhanced);
  g.level = 16;
  g.setupRound();
  for (const auto &h : g.hazards)
    if (h.active && (h.kind == KeystoneKapers::Ball || h.kind == KeystoneKapers::Cart ||
                     h.kind == KeystoneKapers::Biplane))
      HOST_CHECK(h.direction == (h.floor % 2 ? 1 : -1));
  // Wrap only after the complete aircraft has left the mall, preserving overshoot.
  for (int direction : {-1, 1}) {
    g.start(enhanced);
    for (auto &h : g.hazards)
      h.active = false;
    g.hazards[0] = {direction < 0 ? -10.f : 490.f, 1, KeystoneKapers::Biplane, int8_t(direction),
                    true};
    g.update();
    HOST_CHECK(direction < 0 ? g.hazards[0].x < -10 : g.hazards[0].x > 490);
    g.update();
    HOST_CHECK(direction < 0 ? g.hazards[0].x > 490 : g.hazards[0].x < -10);
  }
  // A press just before landing survives until the next grounded frame.
  g.start(enhanced);
  g.jump = 1;
  g.jumpVelocity = -2;
  g.handle({Fire, Fire});
  g.update();
  HOST_CHECK(g.jump == 0 && g.jumpBuffer > 0);
  g.handle({});
  g.update();
  HOST_CHECK(g.jump > 0 && g.jumpBuffer == 0);
  // An unavailable lift should not strand Harry when the officer intercepts him.
  g.start(enhanced);
  g.floor = 2;
  g.x = 240;
  g.thiefFloor = 1;
  g.thiefX = 240;
  g.roundTick = 504;
  HOST_CHECK(g.liftWait(1) == 72);
  g.roundTick = 696;
  HOST_CHECK(g.liftWait(1) == 264);
  g.thief();
  HOST_CHECK(!g.thiefSeekLift && g.thiefX < 240);
  // Continuous camera motion crosses the old page seam without a 160-pixel jump.
  g.x = 159;
  int beforeCamera = g.viewCamera();
  g.x = 160;
  HOST_CHECK(g.viewCamera() == beforeCamera + 1);
  g.inLift = true;
  HOST_CHECK(g.viewCamera() == 160);
  std::array<uint16_t, W * H> pixels{};
  Canvas canvas;
  canvas.attach(pixels.data());
  g.start({});
  g.level = 16;
  g.setupRound();
  g.x = 150;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "keystone_01_hazards"));
  g.x = 240;
  g.inLift = true;
  g.roundTick = 156;
  g.render(canvas);
  HOST_CHECK(canvas.pixel(88, 196) != Black); // no stale ground-floor officer marker
  HOST_CHECK(captureAtari(canvas, "keystone_02_lift"));
  g.inLift = false;
  g.x = 448;
  g.floor = 0;
  g.ride = 30;
  g.rideOriginX = 448;
  g.rideDuration = 60;
  g.rideHeight = 0;
  g.x = 432;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "keystone_03_escalator"));
  g.ride = 0;
  g.floor = 3;
  g.x = 400;
  g.thiefFloor = 3;
  g.thiefX = 448;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "keystone_04_roof"));
  HOST_CHECK(canvas.pixel(20, 45) == 0x7370); // top storey must not overwrite the roof
  g.start({});
  g.level = 16;
  g.setupRound();
  g.floor = 1;
  g.x = 100;
  g.crouching = true;
  g.hazards[0] = {100, 1, KeystoneKapers::Biplane, 1, true};
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "keystone_05_duck"));
  g.floor = 0;
  g.x = 170;
  g.jump = 20;
  g.crouching = false;
  g.hazards[0] = {160, 0, KeystoneKapers::Biplane, 1, true};
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "keystone_06_jump_page_edge"));
  for (int yy = 0; yy < H; ++yy)
    for (int xx : {0, 7, 168, 175})
      HOST_CHECK(canvas.pixel(xx, yy) == Black);
  // Jumping must never repaint the upper storey, its floor stripe, or the HUD.
  for (int f = 0; f < 4; ++f) {
    g.start(enhanced);
    g.floor = uint8_t(f);
    g.x = 160;
    g.render(canvas);
    auto standing = pixels;
    g.jump = 23;
    g.render(canvas);
    int ceiling = f < 3 ? KeystoneKapers::ground(f + 1) + 4 : 33;
    bool visibleFace = false;
    for (int yy = ceiling; yy < KeystoneKapers::ground(f); ++yy)
      for (int xx = 84; xx < 92; ++xx)
        visibleFace |= canvas.pixel(xx, yy) == atari::Skin;
    HOST_CHECK(visibleFace);
    for (int yy = 0; yy < ceiling; ++yy)
      for (int xx = 0; xx < W; ++xx)
        HOST_CHECK(pixels[yy * W + xx] == standing[yy * W + xx]);
  }
  g.start(enhanced);
  g.level = 16;
  g.setupRound();
  g.floor = 1;
  g.x = 160;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "keystone_07_enhanced_middle"));
  g.jump = 23;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "keystone_08_jump_ceiling"));
  g.jump = 0;
  g.x = 160;
  g.hazards[0] = {60, 1, KeystoneKapers::Biplane, 1, true};
  g.render(canvas);
  HOST_CHECK(canvas.pixel(10, KeystoneKapers::ground(1) - 19) == Yellow);
  HOST_CHECK(captureAtari(canvas, "keystone_09_incoming_plane"));
  g.start(enhanced);
  for (auto &h : g.hazards)
    h.active = false;
  g.floor = 0;
  g.x = 160;
  g.running = true;
  for (unsigned pose = 0; pose < 8; ++pose) {
    g.walkDistance = pose * 4;
    g.render(canvas);
    char scene[40];
    std::snprintf(scene, sizeof(scene), "keystone_stride_%02u", pose);
    HOST_CHECK(captureAtari(canvas, scene));
  }
  return true;
}
#endif
} // namespace arcade
