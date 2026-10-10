#pragma once
#include "Canvas.h"
namespace arcade {
struct GameOptions {
  uint8_t difficulty = 1, layout = 0;
  bool effects = true;
  uint32_t seed = 1;
};
class Game {
protected:
  GameOptions options;
  Random rng;
  InputFrame input;
  bool paused = false, ended = false;
  uint32_t points = 0, tick = 0;
  uint16_t level = 1;
  int health = 3;
  Sound pending = Sound::None;
  void add(uint32_t n) {
    points = UINT32_MAX - points < n ? UINT32_MAX : points + n;
  }
  void sound(Sound s) {
    if (uint8_t(s) > uint8_t(pending))
      pending = s;
  }
  void baseStart(const GameOptions &o) {
    options = o;
    rng.state = o.seed ? o.seed : 1;
    input = {};
    paused = ended = false;
    points = tick = 0;
    level = 1;
    health = 3;
    pending = Sound::None;
  }

public:
  virtual ~Game() = default;
  virtual void initialize() {};
  virtual void start(const GameOptions &) = 0;
  virtual void handle(const InputFrame &f) {
    input = f;
  }
  virtual void update() = 0; // fixed 1/60 second, independent of display frame rate
  virtual void render(Canvas &) const = 0;
  virtual void pause() {
    paused = true;
    input = {};
    pending = Sound::None;
  }
  virtual void resume() {
    paused = false;
    input = {};
  }
  virtual void cleanup() {
    input = {};
    pending = Sound::None;
    ended = true;
  }
  virtual size_t activeEntities() const = 0;
  uint32_t score() const {
    return points;
  }
  uint16_t stage() const {
    return level;
  }
  int lives() const {
    return health;
  }
  bool gameOver() const {
    return ended;
  }
  Sound takeSound() {
    Sound s = pending;
    pending = Sound::None;
    return s;
  }
};
enum class GameId : uint8_t {
#define ARCADE_GAME(id, type, number) id = number,
#include "games/Catalog.def"
#undef ARCADE_GAME
  Count
};
constexpr size_t GameCount = size_t(GameId::Count);
static_assert(GameCount <= 64, "Expand persistent score slots before shipping more than 64 games");
struct GameInfo {
  const char *title;
  const char *description;
  std::array<const char *, 6> instructions;
  uint16_t accent;
  uint8_t icon;
  uint16_t required;
};
struct GameEntry {
  GameInfo info;
  size_t bytes;
  Game *(*construct)(void *);
  void (*destroy)(Game *);
};
const std::array<GameInfo, GameCount> &registry();
Game &gameFor(GameId id);
} // namespace arcade
