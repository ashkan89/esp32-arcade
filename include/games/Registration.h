#pragma once
#include "arcade/Game.h"
#include <new>
namespace arcade {
class TickGame : public Game {
protected:
  virtual void simulate() = 0;
  void nextStage() {
    level = uint16_t(std::min(999, int(level) + 1));
    sound(Sound::Wave);
  }
  void loseLife() {
    if (--health <= 0) {
      ended = true;
      sound(Sound::Over);
    } else
      sound(Sound::Hit);
  }

public:
  void update() final {
    if (paused || ended)
      return;
    tick++;
    simulate();
    input.pressed = input.repeated = input.released = 0;
  }
};
template <class T> GameEntry gameEntry(GameInfo info) {
  static_assert(sizeof(T) <= 8192, "Increase the shared game arena for this game");
  return {info, sizeof(T), [](void *p) -> Game * { return new (p) T; },
          [](Game *g) { static_cast<T *>(g)->~T(); }};
}
// All grid games use the same repeat-aware movement convention.
inline int gridMove(const InputFrame &f, int position, int columns, int rows) {
  int x = position % columns, y = position / columns;
  if (f.nav(Left))
    x = std::max(0, x - 1);
  else if (f.nav(Right))
    x = std::min(columns - 1, x + 1);
  else if (f.nav(Up))
    y = std::max(0, y - 1);
  else if (f.nav(Down))
    y = std::min(rows - 1, y + 1);
  return y * columns + x;
}
} // namespace arcade
