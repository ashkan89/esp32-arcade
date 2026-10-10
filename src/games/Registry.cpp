#include "arcade/Game.h"
#include <cstddef>
namespace arcade {
#define ARCADE_GAME(id, type, number) GameEntry describe##type();
#include "games/Catalog.def"
#undef ARCADE_GAME
static const std::array<GameEntry, GameCount> &entries() {
  static const std::array<GameEntry, GameCount> table = {{
#define ARCADE_GAME(id, type, number) describe##type(),
#include "games/Catalog.def"
#undef ARCADE_GAME
  }};
  return table;
}
const std::array<GameInfo, GameCount> &registry() {
  static const auto infos = [] {
    std::array<GameInfo, GameCount> result{};
    for (size_t i = 0; i < GameCount; i++)
      result[i] = entries()[i].info;
    return result;
  }();
  return infos;
}
Game &gameFor(GameId id) {
  // One internal RAM arena: adding games does not retain every game's working state.
  alignas(std::max_align_t) static uint8_t arena[8192];
  static Game *active = nullptr;
  static unsigned previous = 0;
  unsigned index = unsigned(id);
  if (index >= GameCount)
    index = 0;
  if (active) {
    active->cleanup();
    entries()[previous].destroy(active);
  }
  previous = index;
  active = entries()[index].construct(arena);
  return *active;
}
} // namespace arcade
