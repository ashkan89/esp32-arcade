#pragma once
#include "arcade/Canvas.h"
namespace arcade { namespace art {
enum class Sprite : uint8_t {
  Jet, EnemyJet, Alien, AlienPink, Rock, Boss, Ufo, Missile,
  Car, EnemyCar, Truck, Coin, Frog, Log, Turtle, Apple,
  Cop, CopStep, Thief, ThiefStep, Hero, HeroStep, Guard, Zombie,
  Robot, FriendRobot, Bug, Bee, Dragon, Bird, Tank, EnemyTank,
  Diamond, Ruby, Amber, Amethyst, Emerald, Block, Crate, Wall,
  Bomb, Explosion, GrassPlatform, WoodPlatform, SteelPlatform, Mushroom, Chef, Sausage,
  SnowHero, SnowMonster, Skier, Cowboy, Helicopter, Tanker, Fuel, Tree,
  Cart, Radio, Suitcase, Money, Ball, Biplane, Card, Pinball,
  SnakeHead, SnakeBody, SnakeCorner, SnakeTail, Lander, Planet, Paddle, EnemyPaddle,
  Bun, Lettuce, Cheese, Patty, Door, Elevator, TrainCar, Locomotive,
  Spear, Demon, Samurai, EnemySamurai, RocketBase, City, Barrel, Flag,
  GhostRed, GhostBlue, GhostPink, GhostOrange, MazeHero, Hovercraft, Key, Crown,
  RearCar, RearEnemyCar, CopDuck, ThiefDuck, HeroJump, Lily, Bubble, WoodSlab
};
constexpr unsigned SceneCount = 56, SpriteCount = 104;
extern const uint8_t scenePixels[SceneCount][4400];
extern const uint16_t sceneColors[SceneCount][16];
extern const uint8_t spritePixels[SpriteCount][288];
extern const uint16_t spriteColors[SpriteCount][16];
extern const uint8_t spriteBounds[SpriteCount][4];
void scene(Canvas &c, unsigned game, int x = 0, int y = Top, int w = W, int h = H - Top, unsigned light = 55);
void sprite(Canvas &c, Sprite id, int x, int y, int w = 16, int h = 16, bool flip = false);
void tinted(Canvas &c, Sprite id, int x, int y, int w, int h, uint16_t color);
void object(Canvas &c, Sprite id, int cx, int cy, int w = 14, int h = 14, bool flip = false);
void tile(Canvas &c, int x, int y, int w, int h, uint16_t color);
void platform(Canvas &c, int x, int y, int w, int h, Sprite type = Sprite::GrassPlatform);
Sprite hero(unsigned game);
void cover(Canvas &c, unsigned game, int x, int y, int w, int h);
void rotated(Canvas &c, Sprite id, int cx, int cy, int size, float angle);
void river(Canvas &c, int y, int bank, int island, unsigned scroll);
void water(Canvas &c, int x, int y, int w, int h, unsigned scroll);
void mall(Canvas &c, int x, int y, int w, int h);
} }
