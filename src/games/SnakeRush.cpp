#include "assets/Art.h"
#include "games/SnakeRush.h"
#include "assets/Visuals.h"
namespace arcade {
bool SnakeRush::wall(Cell c) const {
  if (c.x < 0 || c.y < 0 || c.x >= Cols || c.y >= Rows)
    return true;
  if (options.layout == 1)
    return ((c.x == 5 || c.x == 14) && c.y >= 6 && c.y < 16);
  if (options.layout == 2)
    return c.y == 10 && ((c.x >= 2 && c.x < 8) || (c.x >= 12 && c.x < 18));
  return false;
}
bool SnakeRush::occupied(Cell c, int count) const {
  for (int i = 0; i < count; i++)
    if (body[unsigned(i)] == c)
      return true;
  return false;
}
bool SnakeRush::placeFood() {
  int available = 0;
  for (int y = 0; y < Rows; y++)
    for (int x = 0; x < Cols; x++) {
      Cell c{int8_t(x), int8_t(y)};
      if (!wall(c) && !occupied(c, length))
        available++;
    }
  if (!available) {
    completeCampaign();
    add(1000);
    sound(Sound::Wave);
    return false;
  }
  int choice = rng.range(0, available);
  for (int y = 0; y < Rows; y++)
    for (int x = 0; x < Cols; x++) {
      Cell c{int8_t(x), int8_t(y)};
      if (!wall(c) && !occupied(c, length) && choice-- == 0) {
        food = c;
        return true;
      }
    }
  return false;
}
void SnakeRush::start(const GameOptions &o) {
  baseStart(o);
  length = 4;
  direction = nextDir = 0;
  moveTicks = 0;
  turnQueued = false;
  for (int i = 0; i < length; i++)
    body[unsigned(i)] = {int8_t(9 - i), 3};
  placeFood();
  health = 1;
}
void SnakeRush::handle(const InputFrame &f) {
  Game::handle(f);
  if (turnQueued || paused || ended)
    return;
  int candidate = direction;
  if (f.hit(Right))
    candidate = 0;
  else if (f.hit(Down))
    candidate = 1;
  else if (f.hit(Left))
    candidate = 2;
  else if (f.hit(Up))
    candidate = 3;
  if (candidate != direction && (candidate + 2) % 4 != direction) {
    nextDir = candidate;
    turnQueued = true;
  }
}
void SnakeRush::step() {
  direction = nextDir;
  turnQueued = false;
  Cell head = body[0];
  const int dx[] = {1, 0, -1, 0}, dy[] = {0, 1, 0, -1};
  head.x += int8_t(dx[direction]);
  head.y += int8_t(dy[direction]);
  bool eating = head == food;
  // Tail moves away this tick unless growing; moving into that vacated cell is legal.
  if (wall(head) || occupied(head, int(length) - (eating ? 0 : 1))) {
    ended = true;
    health = 0;
    sound(Sound::Over);
    return;
  }
  if (eating && length < Capacity)
    length++;
  for (int i = int(length) - 1; i > 0; i--)
    body[unsigned(i)] = body[unsigned(i - 1)];
  body[0] = head;
  if (eating) {
    add(10 + level * 2);
    progressTo(uint16_t(1 + (length - 4) / 5));
    sound(Sound::Power);
    placeFood();
  }
}
void SnakeRush::update() {
  if (paused || ended)
    return;
  tick++;
  int interval = std::max(5, 18 - int(level) - challenge() * 2);
  if (++moveTicks >= interval) {
    moveTicks = 0;
    step();
  }
  input.pressed = 0;
}
void SnakeRush::render(Canvas &c) const {
    art::scene(c, 5);
  art::scene(c, 5, 8, 28, 160, 176, 30);
  c.rect(7, 27, 162, 178, Grey, false);
  for (int y = 0; y < Rows; y++)
    for (int x = 0; x < Cols; x++)
      if (wall({int8_t(x), int8_t(y)}))
        visual::tile(c, 8 + x * 8, 28 + y * 8, 7, 7, Pink);
  int fx = 11 + food.x * 8, fy = 31 + food.y * 8;
  art::object(c, art::Sprite::Apple, fx, fy, 8, 8);
  for (unsigned i = 0; i < length; i++) {
    int cx = 12 + body[i].x * 8, cy = 32 + body[i].y * 8;
    if (i == 0)
      art::rotated(c, art::Sprite::SnakeHead, cx, cy, 9, (int(direction) - 1) * 1.5707963f);
    else {
      bool horizontal = body[i - 1].x != body[i].x;
      auto id = i + 1 == length ? art::Sprite::SnakeTail : art::Sprite::SnakeBody;
      art::rotated(c, id, cx, cy, 8, horizontal ? 1.5707963f : 0.f);
    }
  }
  c.text(8, 211, "D-PAD TO TURN", Grey);
}
} // namespace arcade

#include "games/Registration.h"
namespace arcade {
GameEntry describeSnakeRush() {
  return gameEntry<SnakeRush>({"SNAKE RUSH",
                               "GROW WITHOUT A CRASH",
                               {"D-PAD MOVES", "NO REVERSE TURNS", "FOUR DIRECTION CONTROLS",
                                "EAT GOLD TO GROW", "TAIL VACATES EACH STEP", "LAYOUT IN SETTINGS"},
                               Green,
                               5,
                               Fire});
}
} // namespace arcade
