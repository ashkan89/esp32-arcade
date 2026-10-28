#pragma once
#include "Game.h"
namespace arcade {
enum class Category : uint8_t { Combat, Action, Puzzle, Atari };
enum class LibraryFilter : uint8_t { All, Favorites, Recent, Combat, Action, Puzzle, Atari, Count };
enum class LibrarySort : uint8_t { Catalog, Title, Best, Count };
Category category(unsigned id);
const char *categoryName(Category value);
const char *filterName(LibraryFilter value);
const char *sortName(LibrarySort value);
// Rebuilt on navigation/settings changes only. IDs always refer to the shipped catalog.
class Library {
  std::array<uint8_t, GameCount> ids{};
  unsigned count = 0;

public:
  LibraryFilter filter = LibraryFilter::All;
  LibrarySort sort = LibrarySort::Catalog;
  void rebuild(const Settings &settings);
  unsigned size() const { return count; }
  unsigned at(unsigned row) const { return row < count ? ids[row] : 0; }
  unsigned position(unsigned id) const;
};
} // namespace arcade
