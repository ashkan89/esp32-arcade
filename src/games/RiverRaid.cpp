#include "assets/RiverSource.h"
#include "games/Registration.h"
namespace arcade {
// Native fixed-step port of athaless/RiverRaid, commit d6dcebe7.
// Keep source-space geometry (800x480); scale only when drawing to the cabinet.
class RiverRaid final : public TickGame {
#ifdef ARCADE_HOST_TESTS
  friend bool verifyRiverRaid();
#endif
  struct Object { float x = 0, y = 0; int kind = 4, direction = 1, burst = 0; bool alive = true; };
  struct Course {
    float base = 238;
    std::array<int,3> bank{{3,3,3}}, island{{13,13,13}};
    std::array<Object,5> objects{};
    bool bridge = false;
    uint32_t random = 1;
  } course, checkpoint;
  float x = 370, missileX = 0, missileY = -20, fuel = 166;
  int scrollPhase = 0, fuelPhase = 0, retry = 0, bank = 0;
  bool launched = false;
  static int width(int kind) { return kind < 7 ? 42 : kind < 9 ? 81 : kind < 11 ? 48 : 37; }
  static int height(int kind) { return kind < 7 ? 30 : kind < 9 ? 24 : kind < 11 ? 18 : 72; }
  void profile(float yy, int &edge, int &island) const {
    using namespace river_source;
    edge = 21; island = 0;
    for (int i = 0; i < 3; ++i) {
      int row = int(std::floor((yy - (course.base - 330 - i * 300)) / 6));
      int shape = course.bank[i];
      if (row >= 0 && row < banksLength[shape]) {
        edge = std::max(edge, 21 + banks[shape][row] * 21);
      }
      row = int(std::floor((yy - (course.base - 2000 - i * 246)) / 10));
      shape = course.island[i];
      if (row >= 0 && row < islandsLength[shape])
        island = std::max(island, islands[shape][row] * 21);
    }
  }
  bool land(float xx, float yy) const {
    int edge, island; profile(yy,edge,island);
    return xx < edge || xx >= 800-edge || std::fabs(xx-400) < island;
  }
  bool wall(Rect r) const {
    for (int iy = 0; iy <= 4; ++iy)
      for (int ix = 0; ix <= 2; ++ix)
        if (land(r.x + r.w * ix / 2, r.y + r.h * iy / 4)) return true;
    return false;
  }
  void spawn(Object &o, int index) {
    static constexpr int kinds[] = {4,6,7,8,9,10,11};
    o.kind = kinds[rng.range(0,7)]; o.y = -32.f - index * 96;
    o.direction = o.kind == 6 || o.kind == 8 || o.kind == 9 ? -1 : 1;
    o.alive = true; o.burst = 0;
    // Bounded search replaces the source's potentially endless placement loop.
    for (int attempt = 0; attempt < 16; ++attempt) {
      o.x = float(rng.range(0,9) * 84 + 23);
      if (!wall({o.x,o.y,float(width(o.kind)),float(height(o.kind))})) return;
    }
    o.alive = false;
  }
  void saveCheckpoint() { course.random = rng.state; checkpoint = course; }
  void crash() {
    sound(Sound::Explosion);
    if (--health <= 0) { ended = true; sound(Sound::Over); return; }
    course = checkpoint; rng.state = checkpoint.random;
    x = 370; fuel = 166; missileY = -20; fuelPhase = scrollPhase = 0;
    retry = 80; launched = false;
  }
  void reward(unsigned amount) {
    unsigned before = points / 10000;
    add(amount);
    health = std::min(6, health + int(points / 10000 - before));
  }
  void simulate() override {
    if (retry) { --retry; return; }
    if (!launched) {
      if (!(input.held & (Fire|Left|Right|Up|Down))) return;
      launched = true;
    }
    bank = input.down(Left) == input.down(Right) ? 0 : input.down(Left) ? -1 : 1;
    x = clamp(x + bank * 3.f, 10.f, 734.f);
    int delay = input.down(Up) ? 0 : input.down(Down) ? 2 : 1;
    bool scroll = ++scrollPhase > delay;
    if (scroll) {
      scrollPhase = 0; course.base += 2;
      if (++fuelPhase > 100) { fuelPhase = 0; fuel = std::max(0.f, fuel - 5); }
      if (course.base > 480) course.bridge = true;
      if (course.base - 2492 > 480) {
        course.base = -400;
        for (int i = 0; i < 3; ++i) { course.bank[i] = rng.range(0,8); course.island[i] = rng.range(0,14); }
      }
    }
    if (input.down(Fire) && missileY <= 0) { missileX = x + 24; missileY = 420; sound(Sound::Shot); }
    float previousY = missileY;
    if (missileY > -15) missileY -= 6;
    if (missileY > 0 && wall({missileX,missileY,5,15})) missileY = -20;
    Rect plane{x,420,49,42};
    if (wall(plane) || fuel <= 0) { crash(); return; }
    for (auto &o : course.objects) {
      if (o.burst) --o.burst;
      if (scroll) o.y += 2;
      if (o.alive && o.kind < 11 && (o.kind >= 9 || o.y > 200)) {
        float nx = o.x + o.direction;
        if (o.kind < 9 && wall({nx,o.y,float(width(o.kind)),float(height(o.kind))})) o.direction = -o.direction;
        else o.x = nx;
        if (o.kind >= 9) {
          if (o.x > 800) o.x = -float(width(o.kind));
          if (o.x + width(o.kind) < 0) o.x = 800;
        }
      }
      if (o.y > 480) { spawn(o,0); continue; }
      if (!o.alive) continue;
      Rect target{o.x,o.y,float(width(o.kind)),float(height(o.kind))};
      if (missileY > 0 && swept(missileX+2,previousY,missileX+2,missileY,target,2)) {
        o.alive = false; o.burst = 40; missileY = -20;
        reward(o.kind < 7 ? 80 : o.kind < 9 ? 40 : o.kind < 11 ? 120 : 30);
        sound(Sound::Explosion); continue;
      }
      if (overlaps(plane,target)) {
        if (o.kind == 11) { fuel = std::min(166.f,fuel + .3f); if (tick % 12 == 0) sound(Sound::Power); }
        else { crash(); return; }
      }
    }
    Rect bridge{312,course.base+169,175,68};
    if (course.bridge && missileY > 0 && swept(missileX+2,previousY,missileX+2,missileY,bridge,2)) {
      course.bridge = false; missileY = -20; reward(250); saveCheckpoint();
      if (level < 65535) ++level;
      sound(Sound::Wave);
    }
    if (course.bridge && overlaps(plane,bridge)) { crash(); return; }
    if (fuel <= 70 && tick % 60 == 0) sound(Sound::Warning);
  }
  static void sprite(Canvas &c, int kind, float xx, float yy, float ww, float hh) {
    using namespace river_source;
    const auto &s = sprites[kind];
    int left = 8 + int(xx / 5), top = 10 + int(yy * .375f);
    int w = std::max(1,int(ww/5)), h = std::max(1,int(hh*.375f));
    for (int row = 0; row < h; ++row) {
      int y = top + row; if (y < 10 || y >= 190) continue;
      for (int col = 0; col < w; ++col) {
        int x = left + col; if (x < 8 || x >= 168) continue;
        uint8_t color = s.data[(row*s.h/h)*s.w + col*s.w/w];
        if (color) c.pixel(x,y,palette[color]);
      }
    }
  }
public:
  void start(const GameOptions &o) override {
    baseStart(o); health = 3;
    course = Course{};
    for (int i = 0; i < 5; ++i) spawn(course.objects[i],i);
    // Source restart's guaranteed first targets and refuelling station.
    course.objects[4] = {350,-96,8,-1,0,true};
    course.objects[3] = {450,-192,6,-1,0,true};
    course.objects[1] = {500,-384,11,1,0,true};
    x = 370; fuel = 166; missileY = -20; bank = retry = scrollPhase = fuelPhase = 0;
    launched = false; saveCheckpoint();
  }
  void render(Canvas &c) const override {
    using namespace river_source;
    c.clear();
    // One shared geometry query drives both collisions and visible terrain.
    c.rect(8,10,160,180,palette[3]);
    for (int yy = 0; yy < 180; ++yy) {
      int edge, island; profile(yy/.375f,edge,island);
      // Match source-space pixel-center sampling used by land().
      int left = clamp(int(std::ceil((edge-2.5f)/5)),0,160);
      int right = clamp(int(std::ceil((800-edge-2.5f)/5)),0,160);
      c.rect(8,10+yy,left,1,palette[2]);
      c.rect(8+right,10+yy,160-right,1,palette[2]);
      if (island) {
        int begin = clamp(int(std::floor((400-island-2.5f)/5))+1,0,160);
        int end = clamp(int(std::ceil((400+island-2.5f)/5)),0,160);
        c.rect(8+begin,10+yy,end-begin,1,palette[2]);
      }
    }
    int by = 10 + int((course.base+169)*.375f);
    if (by < 190 && by + 25 >= 10) {
      for (int row = 0; row < 25; ++row) if (by+row >= 10 && by+row < 190) {
        c.rect(8,by+row,62,1,palette[row % 8 == 0 ? 1 : 11]);
        c.rect(106,by+row,62,1,palette[row % 8 == 0 ? 1 : 11]);
        if (course.bridge) c.rect(70,by+row,36,1,palette[row%5 == 0 ? 17 : row%2 ? 18 : 19]);
      }
    }
    for (const auto &o : course.objects) {
      if (o.alive) sprite(c,o.kind < 7 ? (o.direction < 0 ? 5 : 3) + int((tick/8)%2) : o.kind,o.x,o.y,float(width(o.kind)),float(height(o.kind)));
      else if (o.burst) sprite(c,o.burst > 20 ? 12 : 13,o.x,o.y,50,36);
    }
    if (missileY > 0) c.rect(8+int(missileX/5),10+int(missileY*.375f),1,5,palette[13]);
    if (!retry || tick % 8 < 4) sprite(c,bank < 0 ? 1 : bank > 0 ? 2 : 0,x,420,49,42);
    c.rect(8,194,160,26,palette[14]);
    c.number(12,198,points,Yellow); c.text(119,198,"L",Black); c.number(130,198,health,Black);
    c.rect(51,210,75,7,Black); c.rect(53,212,int(71*fuel/166),3,fuel <= 70 ? Red : Yellow);
    c.text(13,211,"FUEL",Black);
    if (!launched && !retry) c.centered(82,"A TO TAKE OFF",Yellow);
    if (retry) c.centered(82,"CHECKPOINT RETRY",Yellow);
  }
  size_t activeEntities() const override { return 7; }
  bool ownsHud() const override { return true; }
  uint16_t stageLimit() const override { return 0; }
};
GameEntry describeRiverRaid() {
  return gameEntry<RiverRaid>({"RIVER RAID", "ATHALESS SOURCE PORT",
    {"L/R BANK  A FIRES", "UP FAST  DOWN SLOW", "FLY OVER FUEL TO REFILL",
     "SHOOT BRIDGES: CHECKPOINT", "SHIPS 40  HELICOPTERS 80", "JETS 120  BRIDGES 250"},
     Green,51,Left|Right|Up|Down|Fire});
}
#ifdef ARCADE_HOST_TESTS
bool verifyRiverRaid() {
  RiverRaid g; g.start({});
  HOST_CHECK(g.health == 3 && g.fuel == 166 && !g.launched);
  auto course = g.course;
  for (int t = 0; t < 60; ++t) g.update();
  HOST_CHECK(g.course.base == course.base);
  g.handle({Up,Up}); g.update();
  HOST_CHECK(g.course.base == course.base + 2);
  g.start({}); g.handle({Down,Down});
  for (int t = 0; t < 3; ++t) g.update();
  HOST_CHECK(g.course.base == course.base + 2);
  // Cartridge-inspired score values retained from the supplied Python source.
  g.course.base = -1000; g.course.bank.fill(7); g.course.island.fill(0);
  for (int kind : {4,8,9,11}) {
    for (auto &o : g.course.objects) o.alive = false;
    uint32_t before = g.points;
    g.course.objects[0] = {300,100,kind,1,0,true};
    g.missileX = 315; g.missileY = 110; g.handle({}); g.update();
    HOST_CHECK(g.points - before == unsigned(kind < 7 ? 80 : kind < 9 ? 40 : kind < 11 ? 120 : 30));
  }
  // Refuelling is safe, shooting a depot removes it, crash restores checkpoint.
  for (auto &o : g.course.objects) o.alive = false;
  g.course.objects[0] = {g.x,420,11,1,0,true}; g.fuel = 50;
  g.update(); HOST_CHECK(g.fuel > 50 && g.health == 3);
  g.course.base = 180; g.course.bridge = true; g.missileX = 400; g.missileY = 355;
  g.course.objects[0].alive = false;
  uint32_t before = g.points; g.update();
  HOST_CHECK(!g.course.bridge && g.points == before+250 && g.level == 2);
  auto saved = g.checkpoint; g.course.base += 100; g.crash();
  HOST_CHECK(g.course.base == saved.base && g.fuel == 166 && g.retry == 80 && g.health == 2);
  std::array<uint16_t,W*H> pixels{}; Canvas c; c.attach(pixels.data());
  g.start({}); g.render(c); HOST_CHECK(captureAtari(c,"river_source_00_start"));
  g.course.base = 800; g.render(c); HOST_CHECK(captureAtari(c,"river_source_01_banks"));
  g.course.base = 2200; g.render(c); HOST_CHECK(captureAtari(c,"river_source_02_islands"));
  g.launched = true;
  for (auto &o : g.course.objects) o.alive = false;
  g.render(c);
  for (int row = 0; row < 140; ++row)
    for (int col = 0; col < 160; ++col)
      HOST_CHECK(c.pixel(8+col,10+row) == river_source::palette[g.land(col*5+2.5f,row/.375f) ? 2 : 3]);
  g.course.base = 100; g.course.bridge = true; g.render(c); HOST_CHECK(captureAtari(c,"river_source_03_bridge"));
  return true;
}
#endif
} // namespace arcade
