#include "assets/Art.h"
#include "arcade/DirtySpan.h"
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
#ifdef ARCADE_HOST_TESTS
    std::array<float, 129> route{};
#endif
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
  // Two reachable intervals retain both island branches. Include the nose footprint,
  // depot contact and bridge approach at maximum forward speed.
  static bool feasible(const Section &s, std::array<float, 129> *route = nullptr) {
    int previousDepot = 0, depots = 0;
    for (int i = 0; i < 15; ++i)
      if (s.objects[i].kind == Fuel) {
        int position = i * 32 + 16;
        if (position - previousDepot > 160)
          return false;
        previousDepot = position;
        ++depots;
      }
    if (depots < 3 || 512 - previousDepot > 160)
      return false;
    struct Interval {
      float lo = 1, hi = 0;
    };
    using Pair = std::array<Interval, 2>;
    std::array<Pair, 129> reachable{};
    Pair previous{{{80, 80}, {1, 0}}};
    for (int index = 0; index <= 128; ++index) {
      int pos = index * 4;
      float edge, isle, frontEdge, frontIsle;
      profile(s, float(pos), edge, isle);
      profile(s, float(pos + 6), frontEdge, frontIsle);
      float left = std::max(edge + 5, frontEdge + 3);
      float right = 160 - std::max(edge + 5, frontEdge + 3);
      float island = std::max(isle ? isle + 5.01f : 0.f, frontIsle ? frontIsle + 3.01f : 0.f);
      Pair allowed{
          {{left, island ? 80 - island : right}, {island ? 80 + island : 1, island ? right : 0}}};
      float contactLow = 0, contactHigh = 160;
      if (pos >= 484 && pos <= 504) {
        contactLow = 68;
        contactHigh = 92;
      }
      for (int i = 0; i < 15; ++i)
        if (s.objects[i].kind == Fuel && pos == i * 32 + 16) {
          contactLow = s.objects[i].x - 7;
          contactHigh = s.objects[i].x + 7;
        }
      std::array<Interval, 4> parts{};
      unsigned count = 0;
      for (const auto &a : allowed)
        for (const auto &v : previous) {
          if (v.lo > v.hi || a.lo > a.hi)
            continue;
          float expansion = index ? 1.46f : 0.f;
          Interval part{std::max({a.lo, contactLow, v.lo - expansion}),
                        std::min({a.hi, contactHigh, v.hi + expansion})};
          if (part.lo <= part.hi)
            parts[count++] = part;
        }
      if (!count)
        return false;
      std::sort(parts.begin(), parts.begin() + count,
                [](Interval a, Interval b) { return a.lo < b.lo; });
      Pair result{};
      unsigned merged = 0;
      for (unsigned i = 0; i < count; ++i) {
        if (merged && parts[i].lo <= result[merged - 1].hi)
          result[merged - 1].hi = std::max(result[merged - 1].hi, parts[i].hi);
        else {
          if (merged == 2)
            return false;
          result[merged++] = parts[i];
        }
      }
      previous = result;
      if (route)
        reachable[index] = result;
    }
    if (route) {
      float chosen = 80;
      for (int index = 128; index >= 0; --index) {
        float best = 1000, selected = 80;
        for (const auto &v : reachable[index])
          if (v.lo <= v.hi) {
            float candidate = clamp(chosen, v.lo, v.hi);
            if (std::abs(candidate - chosen) < best) {
              best = std::abs(candidate - chosen);
              selected = candidate;
            }
          }
        (*route)[unsigned(index)] = chosen = selected;
      }
    }
    return true;
  }
  static bool validateLayout(Section &s) {
#ifdef ARCADE_HOST_TESTS
    return feasible(s, &s.route);
#else
    return feasible(s);
#endif
  }
  static void placeObject(Section &s, int index, Random &layout, int side = -1) {
    auto &o = s.objects[index];
    float edge, isle;
    footprint(s, index, edge, isle);
    if (o.kind == House) {
      float side = std::max(8.f, edge - 10);
      o.x = layout.range(0, 2) ? 160 - side : side;
      return;
    }
    if (o.kind == Bridge)
      return;
    float radius = halfWidth(o.kind) + 2;
    float left = edge + radius, right = 160 - edge - radius;
    bool chooseRight = side < 0 ? layout.range(0, 2) != 0 : side != 0;
    if (isle) {
      if (chooseRight)
        left = 80 + isle + radius;
      else
        right = 80 - isle - radius;
    }
    if (!isle && side >= 0) {
      if (chooseRight)
        left = 80 + radius;
      else
        right = 80 - radius;
    }
    o.x = left + (right - left) * layout.range(0, 101) / 100.f;
    o.direction = chooseRight ? -1 : 1;
  }
  void buildSection(Section &s, uint16_t seed, uint8_t difficulty) {
    generate(s, seed, difficulty);
    Random layout{derivedSeed(options.seed, uint32_t(seed) | (uint32_t(difficulty) << 16))};
    if (!options.enhanced) {
      // Keep the historical sequence; anchor houses wholly on their shoreline.
      for (int i = 0; i < 15; ++i)
        if (s.objects[i].kind == House) {
          float edge, isle;
          footprint(s, i, edge, isle);
          float side = std::max(8.f, edge - 10);
          s.objects[i].x = s.objects[i].x < 80 ? side : 160 - side;
        } else {
          auto &o = s.objects[i];
          float left, right;
          patrolRange(s, i, o, left, right);
          o.x = clamp(o.x, left, right);
        }
      return;
    }
    bool firstRight = layout.range(0, 2) != 0;
    for (unsigned attempt = 0; attempt < 4; ++attempt) {
      for (int i = 0; i <= 16; ++i) {
        s.bank[i] = uint8_t(16 + layout.range(0, 5) * 4);
        // Short islands leave open water for changes of refuelling side.
        s.island[i] = (i == 3 || i == 8) ? uint8_t(4 + layout.range(0, 3) * 4) : 0;
      }
      s.bank[0] = s.bank[16] = 32;
      bool patrolRight = firstRight;
      for (int i = 0; i < 15; ++i) {
        auto &o = s.objects[i];
        if (i == 2 || i == 6 || i == 10 || i == 13) {
          bool right = (i == 2 || i == 13) ? firstRight : !firstRight;
          o = {(right ? 100.f : 60.f) + layout.range(-2, 3), Fuel, 1, true};
        } else {
          // Mixed patrols on both banks; later tiers introduce faster aircraft.
          o.kind = i == 0 || layout.range(0, 5) == 0            ? House
                   : difficulty >= 3 && layout.range(0, 4) == 0 ? EnemyJet
                   : layout.range(0, 2)                         ? Tanker
                                                                : Helicopter;
          o.alive = true;
          placeObject(s, i, layout, patrolRight);
          if (o.kind != House)
            patrolRight = !patrolRight;
        }
      }
      if (validateLayout(s))
        return;
    }
    // Bounded fallback retains side variety without islands or narrow bends.
    s.bank.fill(24);
    s.bank[0] = s.bank[16] = 32;
    s.island.fill(0);
    bool patrolRight = firstRight;
    for (int i = 0; i < 15; ++i)
      if (s.objects[i].kind != Fuel) {
        placeObject(s, i, layout, patrolRight);
        if (s.objects[i].kind != House)
          patrolRight = !patrolRight;
      }
    validateLayout(s);
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
    buildSection(current, current.seed, current.difficulty);
    buildSection(next, current.after, followingDifficulty(current.difficulty));
    travel = 0;
    x = 80;
    speed = 1.5f;
    bank = 0;
    fuel = 65535;
    missile = bridgeCleared = false;
    awaitingTakeoff = true;
  }
  void enterNext() {
    current = next;
    buildSection(next, current.after, followingDifficulty(current.difficulty));
    ++sectionNumber;
    level = uint16_t(std::min(sectionNumber, 65535u));
    bridgeCleared = !current.objects[15].alive;
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
        buildSection(current, p.seed, p.difficulty);
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
    return kind == Tanker                      ? 9.f
           : kind == Bridge                    ? 16.f
           : kind == EnemyJet || kind == House ? 8.f
                                               : 5.f;
  }
  static int halfHeight(Kind kind) {
    return kind == Fuel ? 12 : kind == Bridge ? 7 : kind == EnemyJet ? 5 : 8;
  }
  static void footprint(const Section &s, int index, float &edge, float &isle) {
    bool house = s.objects[index].kind == House;
    edge = house ? 160 : 0;
    isle = 0;
    // Include the entire sprite height; center-only checks let wings/hulls enter land.
    int radius = halfHeight(s.objects[index].kind);
    for (int dy : {-radius, radius}) {
      float e, island;
      profile(s, objectY(index) + dy, e, island);
      edge = house ? std::min(edge, e) : std::max(edge, e);
      isle = std::max(isle, island);
    }
  }
  static void patrolRange(const Section &s, int index, Object &o, float &left, float &right) {
    float edge, isle;
    footprint(s, index, edge, isle);
    float radius = halfWidth(o.kind);
    left = edge + radius;
    right = 160 - edge - radius;
    if (isle) {
      if (o.x < 80)
        right = 80 - isle - radius - .01f;
      else
        left = 80 + isle + radius + .01f;
    }
    if (left > right) {
      o.kind = Helicopter;
      radius = halfWidth(o.kind);
      left = edge + radius;
      right = 160 - edge - radius;
      if (isle) {
        if (o.x < 80)
          right = 80 - isle - radius - .01f;
        else
          left = 80 + isle + radius + .01f;
      }
    }
  }
  void objects(Section &s, float offset, float previousMissileY) {
    for (int i = 0; i < 16; ++i) {
      auto &o = s.objects[i];
      if (!o.alive)
        continue;
      float yy = 163 - (offset + objectY(i) - travel);
      if (yy < 8 || yy > 192)
        continue;
      unsigned patrolPhase = (tick + (s.seed ^ unsigned(i * 37))) % 120;
      if (o.kind == EnemyJet || ((o.kind == Tanker || o.kind == Helicopter) && patrolPhase < 70)) {
        float left, right;
        patrolRange(s, i, o, left, right);
        o.x = clamp(o.x, left, right);
        float step = o.kind == EnemyJet ? 1.25f : o.kind == Helicopter ? .36f : .20f;
        if (options.enhanced)
          step += std::min(.25f, s.difficulty * .006f);
        float nx = o.x + o.direction * step;
        if (nx < left || nx > right)
          o.direction *= -1;
        else
          o.x = nx;
      }
      if (missile && o.kind != House &&
          swept(missileX, previousMissileY, missileX, missileY,
                {o.x - halfWidth(o.kind), yy - halfHeight(o.kind), halfWidth(o.kind) * 2,
                 float(halfHeight(o.kind) * 2)},
                1)) {
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
      if (std::abs(yy - 163) < (o.kind == Fuel ? 12 : halfHeight(o.kind) + 6) &&
          std::abs(x - o.x) < halfWidth(o.kind) + 4) {
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
      sound(Sound::Warning);
    if (!water(current, travel, x, 4) || !water(current, travel + 6, x, 2)) {
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
    }
    // Clip the shot at the first shoreline contact before resolving targets.
    // This prevents a target behind a bank being hit during the same swept step.
    bool blocked = false;
    if (missile)
      for (int yy = int(oldY); yy >= int(missileY); --yy) {
        float pos = travel + 163 - yy;
        const Section &s = pos >= 512 ? next : current;
        if (pos >= 512)
          pos -= 512;
        float edge, isle;
        profile(s, pos, edge, isle);
        if (yy < 20 || missileX < edge || missileX >= 160 - edge ||
            (isle && missileX >= 80 - isle && missileX < 80 + isle)) {
          missileY = float(yy + 1);
          blocked = true;
          break;
        }
      }
    if (blocked && missileY > oldY)
      missile = false;
    objects(current, 0, oldY);
    if (crashTimer || ended)
      return;
    objects(next, 512, oldY);
    if (blocked)
      missile = false;
  }

  static void drawObject(Canvas &c, const Object &o, int yy, unsigned frame, unsigned scroll = 0) {
    int xx = int(o.x) + 8;
    bool flip = o.direction < 0;
    (void)frame;
    switch (o.kind) {
    case House: art::object(c, art::Sprite::Fuel, xx, yy, 16, 16); break;
    case Fuel:
      art::object(c, art::Sprite::Fuel, xx, yy, 10, 24);
      c.text(xx - 3, yy - 3, "F", White);
      break;
    case Tanker: art::object(c, art::Sprite::Tanker, xx, yy, 16, 14, flip); break;
    case Helicopter: art::object(c, art::Sprite::Helicopter, xx, yy, 16, 14, flip); break;
    case EnemyJet: art::object(c, art::Sprite::EnemyJet, xx, yy, 16, 12, flip); break;
    case Bridge:
      art::platform(c, 8, yy - 6, 160, 12, art::Sprite::SteelPlatform);
      c.rect(8, yy - 7, 160, 1, atari::Ink);
      c.rect(8, yy + 6, 160, 1, atari::Ink);
      for (int p = 8; p < 168; p += 12)
        c.rect(p, yy, 6, 1, Yellow);
      if (o.alive)
        art::sprite(c, art::Sprite::WoodSlab, xx - 16, yy - 7, 32, 14);
      else
        art::water(c, xx - 16, yy - 7, 32, 14, scroll);
      if (o.alive) {
        for (int row = -5; row <= 5; row += 3)
          c.rect(xx - 16, yy + row, 32, 1, atari::Gold);
        c.rect(xx - 16, yy - 7, 32, 1, atari::Ink);
        c.rect(xx - 16, yy + 6, 32, 1, atari::Ink);
      }
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
    buildSection(current, options.enhanced ? uint16_t((options.seed & 65535) | 1) : 0xa814, 1);
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
      art::river(c, yy, int(edge), int(isle), unsigned(travel));
    }
    for (int pass = 0; pass < 2; ++pass) {
      const Section &s = pass ? next : current;
      for (int i = 0; i < 16; ++i) {
        int yy = int(163 - (pass * 512 + objectY(i) - travel));
        if ((s.objects[i].alive || s.objects[i].kind == Bridge) && yy >= 8 && yy <= 192)
          drawObject(c, s.objects[i], yy, tick / 5, unsigned(travel));
      }
    }
    if (!crashTimer || tick % 8 < 4)
      art::object(c, art::Sprite::Jet, int(x) + 8, 163, 14, 14);
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
    art::scene(c, 53, 0, 180, W, 40, 70);
    if (points >= 1000000)
      c.centered(183, "!!!!!!", Yellow, 2);
    else
      atari::digits(c, 52, 183, points, 6, Yellow);
    c.text(16, 201, "E", White);
    c.text(146, 201, "F", White);
    c.rect(28, 200, 111, 9, Black, false);
    c.rect(30, 202, 107, 5, 0x632c);
    c.rect(30, 202, int(fuel * 107 / 65535), 5, fuel < 16384 && tick % 30 < 15 ? Red : Yellow);
    for (int i = 1; i < 4; ++i)
      c.rect(30 + i * 107 / 4, 200, 1, 2, Black);
    for (int i = 0; i < health - 1; ++i)
      art::sprite(c, art::Sprite::Jet, 8 + i % 5 * 8, 184 + i / 5 * 8, 7, 7);
    atari::credit(c, 213, Yellow);
    c.text(8, 7, "SECTION", Grey);
    c.number(52, 7, sectionNumber, White);
    if (twoPlayers)
      c.text(136, 7, player ? "P2" : "P1", Yellow);
    if (!started)
      c.centered(90, twoPlayers ? "B: 2 PLAYERS" : "B: 1 PLAYER", White);
    if (crashTimer)
      c.centered(88, "READY", Yellow);
    c.rect(0, 20, 8, 160, Black);
    c.rect(168, 20, 8, 160, Black);
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
  for (unsigned seed = 1; seed <= 16; ++seed)
    for (unsigned difficulty = 0; difficulty < 4; ++difficulty)
      for (bool fast : {false, true}) {
        GameOptions o;
        o.seed = seed;
        o.difficulty = uint8_t(difficulty);
        o.enhanced = true;
        g.start(o);
        g.started = true;
        g.awaitingTakeoff = false;
        g.speed = fast ? 3.f : .75f;
        unsigned ticks = 0;
        while (g.sectionNumber <= 10 && ticks++ < 8000) {
          unsigned part = std::min(127u, unsigned(g.travel / 4));
          float t = (g.travel - part * 4) / 4;
          float target = g.current.route[part] * (1 - t) + g.current.route[part + 1] * t;
          for (auto *section : {&g.current, &g.next})
            for (auto &enemy : section->objects)
              if (enemy.kind == RiverRaid::Tanker || enemy.kind == RiverRaid::Helicopter ||
                  enemy.kind == RiverRaid::EnemyJet)
                enemy.alive = false;
          InputFrame f;
          f.held = fast ? Up : Down;
          if (g.x < target - .6f)
            f.held |= Right;
          if (g.x > target + .6f)
            f.held |= Left;
          if (g.travel > 450 && g.x >= 65 && !g.bridgeCleared)
            f.held |= Fire;
          float priorX = g.x, priorTravel = g.travel;
          g.handle(f);
          g.update();
          if (g.health != 4 || g.crashTimer)
            std::fprintf(stderr, "River pilot seed=%u fast=%u tick=%u travel=%.2f x=%.2f fuel=%u\n",
                         seed, unsigned(fast), ticks, priorTravel, priorX, g.fuel);
          if (g.health != 4 || g.crashTimer)
            std::fprintf(stderr, "target=%.2f route[0]=%.2f part=%u route=%.2f/%.2f\n", target,
                         g.current.route[0], part, g.current.route[part],
                         g.current.route[part + 1]);
          HOST_CHECK(g.health == 4 && !g.crashTimer && g.fuel > 32);
        }
        HOST_CHECK(g.sectionNumber == 11);
      }
  g.start({});
  // Regression checks for the gameplay audit.
  for (unsigned seed = 1; seed <= 128; ++seed) {
    GameOptions o;
    o.seed = seed;
    o.enhanced = true;
    g.start(o);
    for (unsigned d = 1; d <= 48; ++d) {
      RiverRaid::Section section;
      g.buildSection(section, uint16_t(seed * 317u), uint8_t(d));
      HOST_CHECK(g.feasible(section));
      HOST_CHECK(section.bank.front() == 32 && section.bank.back() == 32 &&
                 !section.island.front() && !section.island.back());
      unsigned fuelCount = 0, leftEnemies = 0, rightEnemies = 0;
      for (int i = 0; i < 15; ++i) {
        const auto &o = section.objects[i];
        if (o.kind == RiverRaid::Fuel) {
          ++fuelCount;
          HOST_CHECK(o.x >= 40 && o.x <= 120);
        } else if (o.kind != RiverRaid::House) {
          HOST_CHECK(g.water(section, g.objectY(i), o.x, g.halfWidth(o.kind)));
          if (o.x < 80)
            ++leftEnemies;
          else
            ++rightEnemies;
        }
      }
      HOST_CHECK(fuelCount >= 3);
      HOST_CHECK(leftEnemies && rightEnemies);
      HOST_CHECK(section.objects[2].x < 80 ? section.objects[6].x > 80 : section.objects[6].x < 80);
      for (unsigned part = 1; part < section.route.size(); ++part)
        HOST_CHECK(std::abs(section.route[part] - section.route[part - 1]) <= 1.461f);
      section.objects[15].alive = false;
      HOST_CHECK(g.water(section, 496, 80, 3));
    }
    auto saved = g.current.bank;
    auto objects = g.current.objects;
    g.restartSection();
    HOST_CHECK(saved == g.current.bank);
    for (unsigned i = 0; i < objects.size(); ++i)
      HOST_CHECK(objects[i].kind == g.current.objects[i].kind &&
                 objects[i].x == g.current.objects[i].x);
  }
  g.start({});
  for (unsigned seed = 1; seed <= 128; ++seed)
    for (unsigned tier = 1; tier <= 48; ++tier) {
      RiverRaid::Section section;
      g.buildSection(section, uint16_t(seed * 317), uint8_t(tier));
      for (int i = 0; i < 15; ++i) {
        const auto &o = section.objects[i];
        for (int dy : {-g.halfHeight(o.kind), g.halfHeight(o.kind)}) {
          if (o.kind == RiverRaid::House) {
            float edge, isle;
            g.profile(section, g.objectY(i) + dy, edge, isle);
            HOST_CHECK(o.x < 80 ? o.x + 8 <= edge : o.x - 8 >= 160 - edge);
          } else
            HOST_CHECK(g.water(section, g.objectY(i) + dy, o.x, g.halfWidth(o.kind)));
        }
      }
    }
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
  auto roadImage = pixels;
  auto bridgeObject = g.current.objects[15];
  g.current.objects[15].kind = RiverRaid::House;
  g.render(canvas);
  unsigned roadPixels = 0;
  for (int yy = 47; yy < 59; ++yy)
    for (int xx = 8; xx < 60; ++xx)
      roadPixels += pixels[yy * W + xx] != roadImage[yy * W + xx];
  HOST_CHECK(roadPixels > 50 && roadImage[47 * W + 88] == pixels[47 * W + 88]);
  g.current.objects[15] = bridgeObject;
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
  // Patrols cross open water in both directions, and remain on water around islands.
  GameOptions enhanced;
  enhanced.enhanced = true;
  g.start(enhanced);
  g.current.bank.fill(24);
  g.current.island.fill(0);
  for (auto &o : g.current.objects)
    o.alive = false;
  g.current.objects[3] = {120, RiverRaid::EnemyJet, -1, true};
  g.travel = 20;
  for (unsigned i = 0; i < 50; ++i) {
    ++g.tick;
    g.objects(g.current, 0, 0);
  }
  HOST_CHECK(g.current.objects[3].x < 80);
  g.current.objects[3] = {40, RiverRaid::EnemyJet, 1, true};
  for (unsigned i = 0; i < 50; ++i) {
    ++g.tick;
    g.objects(g.current, 0, 0);
  }
  HOST_CHECK(g.current.objects[3].x > 80);
  g.current.island.fill(12);
  g.current.objects[3] = {44, RiverRaid::EnemyJet, 1, true};
  for (unsigned i = 0; i < 200; ++i) {
    ++g.tick;
    g.objects(g.current, 0, 0);
    HOST_CHECK(g.water(g.current, g.objectY(3), g.current.objects[3].x, 8));
  }
  // An enemy behind a bank cannot be destroyed by a shot starting on land.
  g.start({});
  g.started = true;
  g.awaitingTakeoff = false;
  for (auto &o : g.current.objects)
    o.alive = false;
  g.current.objects[2] = {30, RiverRaid::Fuel, 1, true};
  g.travel = 10;
  g.missile = true;
  g.missileX = 30;
  g.missileY = 96;
  g.handle({});
  g.update();
  HOST_CHECK(g.current.objects[2].alive && g.score() == 0 && !g.missile);
  g.start({});
  // Targets remain visible through the player's collision/refuelling band.
  for (auto &o : g.current.objects)
    o.alive = false;
  auto &visibleDepot = g.current.objects[2];
  visibleDepot = {80, RiverRaid::Fuel, 1, true};
  g.travel = 88; // depot center is 171: previously culled while still refuelling
  g.render(canvas);
  auto depotImage = pixels;
  visibleDepot.alive = false;
  g.render(canvas);
  bool visibleDepotBand = false;
  for (int yy = 172; yy < 180; ++yy)
    for (int xx = 83; xx < 93; ++xx)
      visibleDepotBand |= pixels[yy * W + xx] != depotImage[yy * W + xx];
  HOST_CHECK(visibleDepotBand);
  visibleDepot.alive = true;
  g.travel = 0;
  g.current.bank.fill(64);
  g.current.island.fill(0);
  g.x = 80;
  g.missile = true;
  g.missileX = 50;
  g.missileY = 100;
  g.handle({Fire, Fire});
  g.update();
  HOST_CHECK(!g.missile && g.health == 4);
  g.start({});
  for (auto &o : g.current.objects)
    o.alive = false;
  g.missile = true;
  g.missileX = 80;
  g.missileY = 100;
  g.handle({Fire, Fire});
  g.update();
  HOST_CHECK(g.missile); // shots in open water continue
  g.next.objects[15].alive = false;
  g.enterNext();
  HOST_CHECK(g.bridgeCleared); // pre-cleared upcoming section retains its checkpoint
  g.start({});
  g.started = true;
  g.travel = 380;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "river_01_bridge"));
  g.current.objects[15].alive = false;
  g.fuel = 8000;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "river_02_gap_low_fuel"));
  g.start({});
  g.started = true;
  g.generate(g.current, 0xa814, 8);
  g.generate(g.next, g.current.after, 9);
  g.travel = 260;
  g.x = 48;
  g.bank = -1;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "river_03_islands"));
  g.travel = 500;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "river_04_section_seam"));
  g.start({});
  g.started = true;
  g.travel = 50;
  g.current.objects[1] = {60, RiverRaid::Tanker, -1, true};
  g.current.objects[2] = {100, RiverRaid::Fuel, 1, true};
  g.current.objects[3] = {64, RiverRaid::EnemyJet, -1, true};
  g.current.objects[4] = {98, RiverRaid::Helicopter, 1, true};
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "river_05_sprites"));
  // Mirrored fleet details must stay attached to the silhouette, without stray pixels.
  std::array<uint16_t, W * H> mirrored{};
  Canvas other;
  other.attach(mirrored.data());
  for (auto kind : {RiverRaid::Tanker, RiverRaid::EnemyJet, RiverRaid::Helicopter}) {
    canvas.clear(atari::RiverWater);
    other.clear(atari::RiverWater);
    g.drawObject(canvas, {80, kind, 1, true}, 90, 0);
    g.drawObject(other, {80, kind, -1, true}, 90, 0);
    for (int yy = 78; yy <= 102; ++yy)
      for (int xx = 76; xx < 100; ++xx)
        HOST_CHECK(canvas.pixel(xx, yy) == other.pixel(175 - xx, yy));
  }
  // Replay exact dirty transfers through moving objects, bridge damage, retry and seams.
  std::array<uint16_t, W * H> snapshot{}, presented{};
  std::array<uint16_t, W * 8> packed{};
  for (unsigned frame = 0; frame < 180; ++frame) {
    if (frame % 60 == 0) {
      enhanced.seed = 1 + frame;
      g.start(enhanced);
      g.started = true;
    }
    g.travel = float((frame * 13) % 512) + .5f;
    g.tick = frame * 5;
    g.bank = int(frame % 3) - 1;
    g.x = 55 + float(frame % 50);
    g.current.objects[15].alive = frame % 7 != 0;
    g.missile = frame % 3 == 0;
    g.missileX = g.x;
    g.missileY = 70 + float(frame % 80);
    g.render(canvas);
    for (int y = 0; y < H; y += 8) {
      int rows = std::min(8, H - y);
      auto plan = dirtyPlan(pixels.data() + y * W, snapshot.data() + y * W, rows, 20, 20000000);
      for (unsigned i = 0; i < plan.count; ++i) {
        const auto &span = plan.spans[i];
        int width = span.right - span.left;
        canvas.copyRect(span.left, y, width, rows, packed.data());
        for (int row = 0; row < rows; ++row)
          std::memcpy(presented.data() + (y + row) * W + span.left, packed.data() + row * width,
                      size_t(width) * 2);
      }
      std::memcpy(snapshot.data() + y * W, pixels.data() + y * W, size_t(rows * W) * 2);
    }
    HOST_CHECK(presented == pixels);
    for (int y = 20; y < 180; ++y)
      HOST_CHECK(canvas.pixel(0, y) == Black && canvas.pixel(175, y) == Black);
  }
  g.start(enhanced);
  g.started = true;
  g.travel = 250;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "river_07_enhanced_patrol"));
  g.travel = 500;
  g.current.objects[15].alive = false;
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "river_08_enhanced_seam"));
  g.crash();
  g.render(canvas);
  HOST_CHECK(captureAtari(canvas, "river_06_retry"));
  return true;
}
#endif
} // namespace arcade
