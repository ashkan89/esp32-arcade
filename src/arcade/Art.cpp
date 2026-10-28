#include "assets/Art.h"
namespace arcade { namespace art {
void scene(Canvas &c, unsigned game, int x, int y, int w, int h, unsigned light) {
  game %= SceneCount;
  uint16_t colors[16];
  light = std::min(100u, light);
  for (unsigned i = 0; i < 16; ++i) {
    uint16_t p = sceneColors[game][i];
    colors[i] = uint16_t((((p >> 11) * light / 100) << 11) |
                         ((((p >> 5) & 63) * light / 100) << 5) | ((p & 31) * light / 100));
  }
  c.indexed(x, y, w, h, scenePixels[game], colors, 88, 100, false);
}
void sprite(Canvas &c, Sprite id, int x, int y, int w, int h, bool flip) {
  unsigned index = unsigned(id);
  if (index >= SpriteCount)
    return;
  const auto &b = spriteBounds[index];
  c.indexed(x, y, w, h, spritePixels[index], spriteColors[index], 24, 24, true, flip,
            b[0], b[1], b[2], b[3]);
}
void object(Canvas &c, Sprite id, int cx, int cy, int w, int h, bool flip) {
  sprite(c, id, cx - w / 2, cy - h / 2, w, h, flip);
}
void tinted(Canvas &c, Sprite id, int x, int y, int w, int h, uint16_t color) {
  unsigned index = unsigned(id);
  if (index >= SpriteCount) return;
  uint16_t colors[16];
  for (unsigned i = 0; i < 16; ++i) {
    uint16_t p = spriteColors[index][i];
    unsigned r = (p >> 11) * 2, g = (p >> 5) & 63, b = (p & 31) * 2;
    unsigned spec = std::min(r, std::min(g, b));
    unsigned pigment = std::max(r, std::max(g, b)) - spec;
    colors[i] = uint16_t((std::min(31u, spec / 2 + (color >> 11) * pigment / 63) << 11) |
                         (std::min(63u, spec + ((color >> 5) & 63) * pigment / 63) << 5) |
                         std::min(31u, spec / 2 + (color & 31) * pigment / 63));
  }
  const auto &bounds = spriteBounds[index];
  c.indexed(x, y, w, h, spritePixels[index], colors, 24, 24, true, false,
            bounds[0], bounds[1], bounds[2], bounds[3]);
}
void tile(Canvas &c, int x, int y, int w, int h, uint16_t color) {
  // Real bitmap material, recolored once per draw in a bounded sixteen-color palette.
  uint16_t colors[16];
  for (unsigned i = 0; i < 16; ++i) {
    uint16_t p = spriteColors[unsigned(Sprite::Block)][i];
    unsigned light = ((p >> 11) * 2 + ((p >> 5) & 63) + (p & 31) * 2) / 4;
    unsigned strength = 40 + light * 60 / 47;
    unsigned r = ((color >> 11) * strength / 100), g = (((color >> 5) & 63) * strength / 100), b = ((color & 31) * strength / 100);
    colors[i] = uint16_t((std::min(31u, r) << 11) | (std::min(63u, g) << 5) | std::min(31u, b));
  }
  const auto &b = spriteBounds[unsigned(Sprite::Block)];
  c.indexed(x, y, w, h, spritePixels[unsigned(Sprite::Block)], colors, 24, 24, true, false,
            b[0], b[1], b[2], b[3]);
}
void platform(Canvas &c, int x, int y, int w, int h, Sprite type) {
  for (int xx = 0; xx < w; xx += 16)
    sprite(c, type, x + xx, y, std::min(16, w - xx), h);
}
Sprite hero(unsigned game) {
  static constexpr Sprite ids[] = {
    Sprite::Jet, Sprite::Jet, Sprite::Jet, Sprite::Jet, Sprite::Pinball, Sprite::SnakeHead,
    Sprite::Car, Sprite::Bird, Sprite::Pinball, Sprite::Block, Sprite::Bomb, Sprite::Hero,
    Sprite::Frog, Sprite::Jet, Sprite::Lander, Sprite::Bomb, Sprite::Tank, Sprite::Dragon,
    Sprite::Block, Sprite::Card, Sprite::Robot, Sprite::Block, Sprite::Hero, Sprite::Crate,
    Sprite::Ruby, Sprite::Jet, Sprite::Bug, Sprite::Missile, Sprite::Jet, Sprite::Diamond,
    Sprite::MazeHero, Sprite::Hero, Sprite::Hero, Sprite::Bird, Sprite::Chef, Sprite::Dragon,
    Sprite::SnowHero, Sprite::Jet, Sprite::Jet, Sprite::Hero, Sprite::Hero, Sprite::Tank,
    Sprite::Samurai, Sprite::Guard, Sprite::Skier, Sprite::Cowboy, Sprite::Diamond, Sprite::RearCar,
    Sprite::Pinball, Sprite::Jet, Sprite::Cop, Sprite::Jet
  };
  return ids[game < 52 ? game : 0];
}
void cover(Canvas &c, unsigned game, int x, int y, int w, int h) {
  scene(c, game, x, y, w, h, 95);
  int size = std::min(w, h) * 3 / 4;
  object(c, hero(game), x + w / 2, y + h / 2, size, size);
}
void rotated(Canvas &c, Sprite id, int cx, int cy, int size, float angle) {
  if (size <= 0 || unsigned(id) >= SpriteCount) return;
  float cs = std::cos(angle), sn = std::sin(angle);
  int bound = size * 3 / 4 + 1;
  for (int y = -bound; y <= bound; ++y)
    for (int x = -bound; x <= bound; ++x) {
      int sx = int((x * cs + y * sn) * 24 / size + 12);
      int sy = int((-x * sn + y * cs) * 24 / size + 12);
      if (sx < 0 || sx >= 24 || sy < 0 || sy >= 24) continue;
      unsigned offset = unsigned(sy * 24 + sx);
      uint8_t p = spritePixels[unsigned(id)][offset / 2];
      unsigned index = offset & 1 ? p & 15 : p >> 4;
      if (index) c.pixel(cx + x, cy + y, spriteColors[unsigned(id)][index]);
    }
}
static uint16_t riverColor(int x, int y, unsigned scroll, bool land) {
  unsigned row = (unsigned(y) + scroll) % 100;
  unsigned sample = row * 88 + (land ? unsigned(x % 16) : unsigned(39 + x % 9));
  uint8_t p = scenePixels[51][sample / 2];
  uint16_t color = sceneColors[51][sample & 1 ? p & 15 : p >> 4];
  unsigned light = ((color >> 11) + ((color >> 5) & 63) + (color & 31)) / 4;
  return land ? uint16_t(((3 + light / 3) << 11) | ((15 + light) << 5) | (2 + light / 4))
              : uint16_t(((1 + light / 8) << 11) | ((12 + light) << 5) | (14 + light / 2));
}
void river(Canvas &c, int y, int bank, int island, unsigned scroll) {
  // Sample authored jungle/water material under the actual collision shoreline.
  // The texture never chooses navigable geometry.
  for (int x = 0; x < 160; ++x) {
    bool land = x < bank || x >= 160 - bank || (island && x >= 80 - island && x < 80 + island);
    // Distinct material hue guarantees terrain/water legibility at every texture sample.
    c.pixel(8 + x, y, riverColor(x, y, scroll, land));
  }
}
void water(Canvas &c, int x, int y, int w, int h, unsigned scroll) {
  for (int yy = std::max(20, y); yy < std::min(180, y + h); ++yy)
    for (int xx = std::max(8, x); xx < std::min(168, x + w); ++xx)
      c.pixel(xx, yy, riverColor(xx - 8, yy, scroll, false));
}
void mall(Canvas &c, int x, int y, int w, int h) {
  c.indexed(x, y, w, h, scenePixels[50] + 20 * 44, sceneColors[50], 88, 28, false);
}
} }
