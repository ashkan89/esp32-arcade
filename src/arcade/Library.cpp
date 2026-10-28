#include "arcade/Library.h"
#include <cstring>
namespace arcade {
Category category(unsigned id) {
  if (id >= unsigned(GameId::Keystone))
    return Category::Atari;
  switch (GameId(id)) {
  case GameId::Blocks: case GameId::Mines: case GameId::Merge: case GameId::Memory:
  case GameId::Cargo: case GameId::Color:
    return Category::Puzzle;
  case GameId::Invaders: case GameId::Sky: case GameId::Star: case GameId::Meteor:
  case GameId::Orbit: case GameId::Blast: case GameId::Tank: case GameId::Bubble:
  case GameId::Vector: case GameId::Bug: case GameId::City: case GameId::Canyon:
  case GameId::Solar: case GameId::Fortress: case GameId::Iron: case GameId::Blade:
  case GameId::Elevator: case GameId::Rails: case GameId::Rescue:
    return Category::Combat;
  default:
    return Category::Action;
  }
}
const char *categoryName(Category value) {
  static const char *const names[] = {"COMBAT", "ACTION", "PUZZLE", "ATARI"};
  return names[unsigned(value) < 4 ? unsigned(value) : 0];
}
const char *filterName(LibraryFilter value) {
  static const char *const names[] = {"ALL GAMES", "FAVORITES", "RECENT", "COMBAT", "ACTION", "PUZZLE", "ATARI"};
  return names[unsigned(value) < unsigned(LibraryFilter::Count) ? unsigned(value) : 0];
}
const char *sortName(LibrarySort value) {
  static const char *const names[] = {"CATALOG", "TITLE A-Z", "MODE BEST"};
  return names[unsigned(value) < unsigned(LibrarySort::Count) ? unsigned(value) : 0];
}
unsigned Library::position(unsigned id) const {
  for (unsigned i = 0; i < count; ++i)
    if (ids[i] == id)
      return i;
  return 0;
}
void Library::rebuild(const Settings &settings) {
  count = 0;
  if (filter == LibraryFilter::Recent) {
    for (auto id : settings.recent)
      if (id < GameCount)
        ids[count++] = id;
    return; // Recency order is meaningful; other sorts do not apply here.
  }
  for (unsigned id = 0; id < GameCount; ++id) {
    if (filter == LibraryFilter::Favorites && !(settings.favorites & (uint64_t(1) << id)))
      continue;
    if (filter >= LibraryFilter::Combat &&
        unsigned(category(id)) != unsigned(filter) - unsigned(LibraryFilter::Combat))
      continue;
    ids[count++] = uint8_t(id);
  }
  auto best = [&](unsigned id) {
    return settings.comparableBest(id, settings.difficulty,
                                   settings.enhanced && id >= unsigned(GameId::Keystone));
  };
  // Bounded insertion sort: no allocations, stable catalog tie-breaks.
  for (unsigned i = 1; i < count; ++i) {
    uint8_t id = ids[i];
    unsigned j = i;
    auto before = [&](unsigned other) {
      if (sort == LibrarySort::Title)
        return std::strcmp(registry()[id].title, registry()[other].title) < 0;
      return sort == LibrarySort::Best && best(id) > best(other);
    };
    while (j && before(ids[j - 1])) {
      ids[j] = ids[j - 1];
      --j;
    }
    ids[j] = id;
  }
}
} // namespace arcade
