#include "games/ArcadeGames.h"
#include "games/Shooters.h"
namespace arcade {
const std::array<GameInfo, 8> &registry() {
  static const std::array<GameInfo, 8> info = {
      {{"SLY INVADERS",
        "DEFEND THE LAST CITY",
        {"L/R MOVE - FIRE SHOOTS", "WEAPON CYCLES 3 GUNS", "R RAPID / L EXTRA LIFE",
         "S SHIELD / D DOUBLE", "BREAKABLE COVER BELOW", "BOSS EVERY FIFTH WAVE"},
        Green,
        0,
        Fire},
       {"SKY FIGHTERS",
        "TAKE BACK THE SKIES",
        {"L/R MOVE - FIRE SHOOTS", "WEAPON: ONE BOMB/WAVE", "R RAPID / S SHIELD",
         "D SPREAD / L LIFE", "DODGE THREE SQUADRONS", "BOSS EVERY FOUR WAVES"},
        Cyan,
        1,
        Fire},
       {"STAR ASSAULT",
        "CHASE THE DIVING SWARM",
        {"L/R MOVE - FIRE SHOOTS", "WEAPON CYCLES GUNS", "CHAIN KILLS FOR COMBO",
         "DIVING ALIENS RETURN", "NO DAMAGE STAGE BONUS", "BOSS EVERY FIFTH STAGE"},
        Pink,
        2,
        Fire},
       {"METEOR PATROL",
        "SURVIVE THE ROCK STORM",
        {"L/R MOVE - FIRE SHOOTS", "WEAPON CYCLES GUNS", "LARGE ROCKS SPLIT", "CHAIN KILLS: X8 MAX",
         "PRESSURE RISES IN 15S", "DODGE THE FRAGMENTS"},
        Orange,
        3,
        Fire},
       {"NEON BREAKER",
        "BREAK EVERY NEON WALL",
        {"L/R MOVE THE PADDLE", "FIRE SERVES THE BALL", "W WIDE / S SLOW BALL",
         "L GIVES AN EXTRA LIFE", "AIM WITH PADDLE EDGE", "CLEAR BRICKS TO ADVANCE"},
        Cyan,
        4,
        Fire},
       {"SNAKE RUSH",
        "GROW WITHOUT A CRASH",
        {"L/R TURN RELATIVE", "FIRE TURNS CLOCKWISE", "D-PAD IN EXPANDED MODE", "EAT GOLD TO GROW",
         "TAIL VACATES EACH STEP", "LAYOUT IN SETTINGS"},
        Green,
        5,
        Fire},
       {"TURBO DODGE",
        "FIND THE OPEN LANE",
        {"L/R CHANGE LANES", "ONE BUTTON: FIRE CYCLES", "GOLD COINS GIVE 50",
         "EACH ROW HAS SAFE LANE", "THREE LIVES TO SURVIVE", "SPEED RISES EVERY 10S"},
        Yellow,
        6,
        Fire},
       {"FLAP ESCAPE",
        "THREAD THE GREEN GATES",
        {"TAP FIRE TO FLAP", "RELEASE BEFORE NEXT TAP", "ONE POINT PER GATE",
         "GAPS NARROW GRADUALLY", "AVOID FLOOR AND CEILING", "FIRE REPLAYS AFTER LOSS"},
        Yellow,
        7,
        Fire}}};
  return info;
}
Game &gameFor(GameId id) {
  static SlyInvaders sly;
  static SkyFighters sky;
  static StarAssault star;
  static MeteorPatrol meteor;
  static NeonBreaker breaker;
  static SnakeRush snake;
  static TurboDodge turbo;
  static FlapEscape flap;
  Game *games[] = {&sly, &sky, &star, &meteor, &breaker, &snake, &turbo, &flap};
  unsigned i = unsigned(id);
  return *games[i < 8 ? i : 0];
}
} // namespace arcade
