#pragma once
#include "Canvas.h"
namespace arcade {
struct GameOptions {
  uint8_t difficulty = 1, layout = 0;
  bool effects = true;
  uint32_t seed = 1;
  bool enhanced = false;
};
class Game {
protected:
  GameOptions options;
  Random rng;
  Random cosmeticRng;
  InputFrame input;
  bool paused = false, ended = false, victorious = false;
  uint32_t points = 0, tick = 0;
  uint16_t level = 1;
  int health = 3;
  Sound pending = Sound::None;
  // Tune hazards and decision windows, never the simulation clock or input sampling.
  int challenge() const {
    static constexpr int pressure[] = {1, 2, 3, 5};
    return pressure[options.difficulty];
  }
  int pacePercent() const {
    static constexpr int pace[] = {106, 112, 122, 142};
    return pace[options.difficulty];
  }
  int hazardInterval(int ticks) const {
    return std::max(1, ticks * 100 / pacePercent());
  }
  float hazardSpeed(float speed) const {
    return speed * pacePercent() / 100.f;
  }
  void completeCampaign() {
    if (ended)
      return;
    victorious = ended = true;
    add(5000 + uint32_t(std::max(0, health)) * 1000);
    sound(Sound::Wave);
  }
  void advanceStage() {
    if (ended)
      return;
    if (level >= 12)
      completeCampaign();
    else {
      ++level;
      sound(Sound::Wave);
    }
  }
  void progressTo(uint16_t target) {
    while (!ended && level < target)
      advanceStage();
  }
  void add(uint32_t n) {
    points = UINT32_MAX - points < n ? UINT32_MAX : points + n;
  }
  void sound(Sound s) {
    if (soundPriority(s) > soundPriority(pending))
      pending = s;
  }
  void baseStart(const GameOptions &o) {
    options = o;
    options.difficulty = std::min(uint8_t(DifficultyCount - 1), o.difficulty);
    rng.state = o.seed ? o.seed : 1;
    options.seed = rng.state;
    cosmeticRng.state = derivedSeed(options.seed, 2);
    input = {};
    paused = ended = victorious = false;
    points = tick = 0;
    level = 1;
    health = options.difficulty < 2 ? 3 : options.difficulty == 2 ? 2 : 1;
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
  // Zero denotes an ongoing score attack rather than a finite campaign.
  virtual uint16_t stageLimit() const {
    return 12;
  }
  virtual bool ownsHud() const {
    return false;
  }
  uint32_t score() const {
    return points;
  }
  uint32_t runSeed() const {
    return options.seed;
  }
  uint32_t gameplayRandomState() const {
    return rng.state;
  }
  unsigned runDifficulty() const {
    return options.difficulty;
  }
  bool enhancedMode() const {
    return options.enhanced;
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
  bool campaignComplete() const {
    return victorious;
  }
  virtual bool bossEncounter() const {
    return false;
  }
  virtual int bossHealth() const {
    return 0;
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
