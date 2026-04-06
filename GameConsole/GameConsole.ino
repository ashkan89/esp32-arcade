#include "Config.h"
#include "Game.h"

Game game;

void setup() {
  game.begin();
}

void loop() {
  game.update();
}