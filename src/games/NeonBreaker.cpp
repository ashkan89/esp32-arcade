#include "games/NeonBreaker.h"
namespace arcade {
void NeonBreaker::layout() {
  hazards.clear();
  bossGrace = 90;
  bossHP = bossMaximum = level % 4 == 0 ? 12 + int(level) : 0;
  if (bossHP) {
    bricks.fill(0);
    return;
  }
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
  if (bossGrace)
    --bossGrace;
  if (bossHP) {
    bossX = 88 + std::sin(tick * .018f) * 50;
    if (launched && tick % 100 == 0)
      projectile(hazards, bossX, 74, 0, 1.7f);
    moveProjectiles(hazards);
    for (auto &h : hazards.items)
      if (h.active && h.y >= 194 && h.x >= paddle && h.x <= paddle + (wide ? 48 : 32) &&
          !bossGrace) {
        h.active = false;
        lose();
        bossGrace = 120;
      }
  }
  if (ended)
    return;
  if (wide > 0)
    wide--;
  if (slow > 0)
    slow--;
  int width = wide ? 48 : 32;
  paddle += (int(input.down(Right)) - int(input.down(Left))) * 3;
  paddle = clamp(paddle, 2.0f, float(W - width - 2));
  if (!launched) {
    ballX = paddle + width / 2;
    ballY = 189;
    if (input.hit(Fire)) {
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
    if (bossHP && swept(ox, oy, ballX, ballY, {bossX - 24, 48, 48, 22}, 3)) {
      --bossHP;
      ballY = 74;
      vy = std::abs(vy);
      add(75);
      sound(Sound::Hit);
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
  if (!any && !bossHP) {
    advanceStage();
    if (ended)
      return;
    add(100);
    layout();
    launched = false;
    dropKind = -1;
    sound(Sound::Wave);
  }
  input.pressed = 0;
}
void NeonBreaker::render(Canvas &c) const {
  if (bossHP) {
    c.rect(int(bossX) - 24, 48, 48, 22, Red);
    c.rect(int(bossX) - 17, 53, 9, 7, Yellow);
    c.rect(int(bossX) + 8, 53, 9, 7, Yellow);
    c.text(5, 24, "BOSS", Pink);
    c.rect(35, 24, bossHP * 136 / std::max(1, bossMaximum), 5, Red);
    drawProjectiles(c, hazards, Pink);
  }
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
} // namespace arcade

#include "games/Registration.h"
namespace arcade {
GameEntry describeNeonBreaker() {
  return gameEntry<NeonBreaker>(
      {"NEON BREAKER",
       "BREAK EVERY NEON WALL",
       {"L/R MOVE THE PADDLE", "A SERVES THE BALL", "W WIDE / S SLOW BALL", "L GIVES AN EXTRA LIFE",
        "AIM WITH PADDLE EDGE", "4/8/12: SENTINEL BOSS"},
       Cyan,
       4,
       Fire});
}
} // namespace arcade
