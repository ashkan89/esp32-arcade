#include "games/ArcadeGames.h"
namespace arcade {
void NeonBreaker::layout() {
  for (unsigned i = 0; i < bricks.size(); i++)
    bricks[i] = ((int(i) / 8 + int(i) % 8 + int(level)) % 5 == 0)
                    ? 0
                    : uint8_t(level >= 3 && i / 8 < 2 ? 2 : 1);
}
void NeonBreaker::start(const GameOptions &o) {
  baseStart(o);
  paddle = 70;
  ballX = 88;
  ballY = 185;
  vx = 1.5f;
  vy = -2.5f;
  launched = false;
  wide = slow = 0;
  paddleDir = 1;
  dropKind = -1;
  layout();
}
void NeonBreaker::lose() {
  if (--health <= 0) {
    ended = true;
    sound(Sound::Over);
  } else {
    launched = false;
    vx = 1.5f;
    vy = -2.5f;
    wide = slow = 0;
    sound(Sound::Hit);
  }
}
void NeonBreaker::update() {
  if (paused || ended)
    return;
  tick++;
  if (wide > 0)
    wide--;
  if (slow > 0)
    slow--;
  int width = wide ? 48 : 32;
  if (options.oneButton) {
    if (input.hit(Fire))
      paddleDir = -paddleDir;
    paddle += paddleDir * 2.6f;
    if (paddle <= 2 || paddle >= W - width - 2)
      paddleDir = -paddleDir;
  } else
    paddle += (int(input.down(Right)) - int(input.down(Left))) * 3;
  paddle = clamp(paddle, 2.0f, float(W - width - 2));
  if (!launched) {
    ballX = paddle + width / 2;
    ballY = 189;
    if (input.hit(Fire) || options.oneButton) {
      launched = true;
      sound(Sound::Shot);
    }
    input.pressed = 0;
    return;
  }
  float speed = slow ? 0.7f : 1.0f;
  speed *= 1 + std::min(0.6f, level * 0.04f) + options.difficulty * 0.05f;
  // Four substeps keep the ball displacement below brick thickness even at max speed.
  for (int step = 0; step < 4 && !ended && launched; step++) {
    float ox = ballX, oy = ballY;
    ballX += vx * speed / 4;
    ballY += vy * speed / 4;
    if (ballX < 4) {
      ballX = 4;
      vx = std::abs(vx);
    }
    if (ballX > W - 4) {
      ballX = W - 4;
      vx = -std::abs(vx);
    }
    if (ballY < Top + 4) {
      ballY = Top + 4;
      vy = std::abs(vy);
    }
    if (vy > 0 && oy + 3 <= 198 && ballY + 3 >= 198 && ballX + 3 > paddle &&
        ballX - 3 < paddle + width) {
      ballY = 195;
      float offset = clamp((ballX - paddle - width / 2) / (width / 2), -1.0f, 1.0f);
      vx = offset * 2.5f;
      if (std::abs(vx) < 0.35f)
        vx = offset < 0 ? -0.35f : 0.35f;
      vy = -std::sqrt(9 - vx * vx);
      sound(Sound::Hit);
    }
    for (unsigned i = 0; i < bricks.size(); i++) {
      if (!bricks[i])
        continue;
      Rect b{float(4 + int(i % 8) * 21), float(34 + int(i / 8) * 13), 19, 10};
      if (overlaps({ballX - 3, ballY - 3, 6, 6}, b)) {
        bricks[i]--;
        add(bricks[i] ? 5 : 20);
        sound(Sound::Hit);
        if (ox + 3 <= b.x || ox - 3 >= b.x + b.w) {
          vx = -vx;
          ballX = ox;
        } else {
          vy = -vy;
          ballY = oy;
        }
        if (!bricks[i] && dropKind < 0 && rng.range(0, 100) < 18) {
          dropKind = rng.range(0, 3);
          dropX = b.x + 9;
          dropY = b.y;
        }
        break;
      }
    }
    if (ballY > H + 3)
      lose();
  }
  if (dropKind >= 0) {
    dropY += 1;
    if (overlaps({dropX - 4, dropY - 4, 8, 8}, {paddle, 198, float(width), 5})) {
      if (dropKind == 0)
        wide = 600;
      else if (dropKind == 1)
        slow = 600;
      else
        health = std::min(5, health + 1);
      dropKind = -1;
      sound(Sound::Power);
    } else if (dropY > H)
      dropKind = -1;
  }
  bool any = false;
  for (auto b : bricks)
    any |= b != 0;
  if (!any) {
    level = uint16_t(std::min(999, int(level) + 1));
    add(100);
    layout();
    launched = false;
    dropKind = -1;
    sound(Sound::Wave);
  }
  input.pressed = 0;
}
void NeonBreaker::render(Canvas &c) const {
  for (unsigned i = 0; i < bricks.size(); i++)
    if (bricks[i]) {
      int x = 4 + int(i % 8) * 21, y = 34 + int(i / 8) * 13;
      c.rect(x, y, 19, 10, i / 8 % 2 ? Pink : Cyan);
      if (bricks[i] > 1)
        c.rect(x + 3, y + 3, 13, 4, White);
    }
  c.rect(int(paddle), 198, wide ? 48 : 32, 5, Green);
  c.circle(int(ballX), int(ballY), 3, White, true);
  if (!launched)
    c.centered(158, "FIRE TO SERVE", Yellow);
  if (dropKind >= 0) {
    c.circle(int(dropX), int(dropY), 4, Yellow, true);
    const char *labels = "WSL";
    char s[2] = {labels[dropKind], 0};
    c.text(int(dropX) - 2, int(dropY) - 3, s, Black);
  }
}
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
    ended = true;
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
  if (options.expanded) {
    if (f.hit(Right))
      candidate = 0;
    else if (f.hit(Down))
      candidate = 1;
    else if (f.hit(Left))
      candidate = 2;
    else if (f.hit(Up))
      candidate = 3;
  } else if (f.hit(Left))
    candidate = (direction + 3) % 4;
  else if (f.hit(Right) || f.hit(Fire))
    candidate = (direction + 1) % 4;
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
    level = uint16_t(1 + (length - 4) / 5);
    sound(Sound::Power);
    placeFood();
  }
}
void SnakeRush::update() {
  if (paused || ended)
    return;
  tick++;
  int interval = std::max(5, 18 - int(level) - options.difficulty * 2);
  if (++moveTicks >= interval) {
    moveTicks = 0;
    step();
  }
  input.pressed = 0;
}
void SnakeRush::render(Canvas &c) const {
  c.rect(7, 27, 162, 178, Grey, false);
  for (int y = 0; y < Rows; y++)
    for (int x = 0; x < Cols; x++)
      if (wall({int8_t(x), int8_t(y)}))
        c.rect(8 + x * 8, 28 + y * 8, 7, 7, Pink);
  c.rect(8 + food.x * 8, 28 + food.y * 8, 7, 7, Yellow);
  for (unsigned i = 0; i < length; i++)
    c.rect(8 + body[i].x * 8, 28 + body[i].y * 8, 7, 7, i == 0 ? White : Green);
  c.text(8, 211, options.expanded ? "D-PAD TO TURN" : "L/R: RELATIVE TURN", Grey);
}
void TurboDodge::start(const GameOptions &o) {
  baseStart(o);
  obstacles.clear();
  lane = 1;
  spawn = 0;
  invincible = coins = 0;
  carX = 81;
}
void TurboDodge::spawnRow() {
  int safe = rng.range(0, 3);
  for (int l = 0; l < 3; l++) {
    if (l == safe) {
      if (rng.range(0, 100) < 55)
        if (auto *o = obstacles.acquire()) {
          o->lane = uint8_t(l);
          o->y = -12;
          o->coin = true;
        }
      continue;
    }
    if (auto *o = obstacles.acquire()) {
      o->lane = uint8_t(l);
      o->y = -12;
    }
  }
}
void TurboDodge::update() {
  if (paused || ended)
    return;
  tick++;
  if (invincible > 0)
    invincible--;
  if (input.hit(Left))
    lane = std::max(0, lane - 1);
  if (input.hit(Right))
    lane = std::min(2, lane + 1);
  if (options.oneButton && input.hit(Fire))
    lane = (lane + 1) % 3;
  float target = 29 + lane * 52;
  carX += clamp(target - carX, -6.0f, 6.0f);
  float speed = 1.2f + std::min(2.0f, level * 0.10f) + options.difficulty * 0.15f;
  // Rows have at least 86 pixels of separation: enough warning and travel time for two lanes.
  if (--spawn <= 0) {
    spawn = int(96 / speed);
    spawnRow();
  }
  for (auto &o : obstacles.items) {
    if (!o.active)
      continue;
    o.y += speed;
    if (overlaps({float(29 + o.lane * 52), o.y, 14, 18}, {carX, 185, 14, 22})) {
      o.active = false;
      if (o.coin) {
        coins++;
        add(50);
        sound(Sound::Power);
      } else if (invincible == 0) {
        health--;
        invincible = 90;
        sound(Sound::Explosion);
        if (health <= 0) {
          ended = true;
          sound(Sound::Over);
        }
      }
    } else if (o.y > H) {
      if (!o.coin)
        add(10);
      o.active = false;
    }
  }
  if (tick % 600 == 0) {
    level = uint16_t(std::min(999, int(level) + 1));
    sound(Sound::Wave);
  }
  if (tick % 60 == 0)
    add(1);
  input.pressed = 0;
}
void TurboDodge::render(Canvas &c) const {
  c.rect(17, Top, 142, H - Top, 0x1082);
  for (int l = 1; l < 3; l++)
    for (int y = Top; y < H; y++)
      if ((y + tick / 2) % 22 < 10)
        c.pixel(17 + l * 52, y, Grey);
  for (auto &o : obstacles.items)
    if (o.active) {
      int x = 29 + o.lane * 52;
      if (o.coin)
        c.circle(x + 7, int(o.y) + 8, 5, Yellow, true);
      else {
        c.rect(x, int(o.y), 14, 18, Red);
        c.rect(x + 2, int(o.y) + 3, 10, 4, White);
      }
    }
  if (invincible == 0 || tick % 10 < 5) {
    c.rect(int(carX), 185, 14, 22, Cyan);
    c.rect(int(carX) + 2, 188, 10, 5, White);
    c.rect(int(carX) + 2, 203, 10, 3, Navy);
  }
  c.text(4, H - 8, "COINS", Grey);
  c.number(40, H - 8, unsigned(coins), Yellow);
}
void FlapEscape::start(const GameOptions &o) {
  baseStart(o);
  gates.clear();
  y = 110;
  vy = 0;
  spawn = 35;
  flying = false;
  health = 1;
}
void FlapEscape::update() {
  if (paused || ended)
    return;
  tick++;
  if (input.hit(Fire)) {
    flying = true;
    vy = -2.65f;
    sound(Sound::Shot);
  }
  input.pressed = 0;
  if (!flying)
    return;
  vy = std::min(3.2f, vy + 0.085f);
  y += vy;
  float speed = 1.0f + std::min(0.6f, level * 0.04f) + options.difficulty * 0.08f;
  int gap = 78 - options.difficulty * 4 - std::min(10, int(level));
  if (--spawn <= 0) {
    spawn = int(104 / speed);
    if (auto *g = gates.acquire()) {
      g->x = W + 8;
      g->gapY = rng.range(48, 124);
    }
  }
  for (auto &g : gates.items) {
    if (!g.active)
      continue;
    g.x -= speed;
    if (overlaps({38, y - 5, 10, 10}, {g.x, float(Top), 20, float(g.gapY - Top)}) ||
        overlaps({38, y - 5, 10, 10}, {g.x, float(g.gapY + gap), 20, float(H - g.gapY - gap)})) {
      ended = true;
      health = 0;
      sound(Sound::Over);
    }
    if (!g.passed && g.x + 20 < 38) {
      g.passed = true;
      add(1);
      level = uint16_t(std::min(999u, 1 + points / 5));
      sound(Sound::Power);
    }
    if (g.x < -24)
      g.active = false;
  }
  if (y - 5 < Top || y + 5 > H) {
    ended = true;
    health = 0;
    sound(Sound::Over);
  }
}
void FlapEscape::render(Canvas &c) const {
  int gap = 78 - options.difficulty * 4 - std::min(10, int(level));
  for (auto &g : gates.items)
    if (g.active) {
      c.rect(int(g.x), Top, 20, g.gapY - Top, Green);
      c.rect(int(g.x), g.gapY + gap, 20, H - g.gapY - gap, Green);
      c.rect(int(g.x) - 2, g.gapY - 5, 24, 5, Cyan);
      c.rect(int(g.x) - 2, g.gapY + gap, 24, 5, Cyan);
    }
  c.circle(43, int(y), 5, Yellow, true);
  c.pixel(45, int(y) - 2, Black);
  c.rect(47, int(y), 5, 2, Orange);
  if (!flying) {
    c.centered(148, "FIRE TO FLAP", Yellow);
    c.centered(164, "TAP. RELEASE. REPEAT.", White);
  }
}
} // namespace arcade
