#pragma once
#include "Config.h"
#include "Sprites.h"
#include "AudioEngine.h"

enum GameState {
  GS_BOOT,
  GS_MENU,
  GS_SETTINGS,
  GS_LEADERBOARD,
  GS_ATTRACT,
  GS_PLAYING,
  GS_PAUSED,
  GS_WAVE_CLEAR,
  GS_GAME_OVER
};

enum WeaponType {
  WPN_LASER,
  WPN_SPREAD,
  WPN_PLASMA
};

enum EnemyType {
  ET_A,
  ET_B,
  ET_C
};

enum PowerType {
  P_NONE,
  P_RAPID,
  P_LIFE,
  P_SHIELD,
  P_DOUBLESHOT,
  P_WEAPONUP
};

struct Bullet {
  int x, y, vx, vy;
  bool active;
  uint16_t color;
  uint8_t size;
  bool playerOwned;
};

struct Enemy {
  int x, y;
  int baseX, baseY;
  bool alive;
  EnemyType type;
  uint8_t hp;
};

struct Diver {
  int enemyIndex;
  float x, y;
  float vx, vy;
  bool active;
};

struct Particle {
  float x, y;
  float vx, vy;
  uint16_t color;
  uint8_t life;
  bool active;
};

struct PowerUp {
  int x, y;
  bool active;
  PowerType type;
};

struct Star {
  int x, y, speed;
};

struct Shield {
  int x, y;
  uint8_t hp[SHIELD_W][SHIELD_H];
};

struct LeaderEntry {
  int score;
};

class Game {
public:
  void begin();
  void update();

private:
  TFT_eSPI tft = TFT_eSPI();
  TFT_eSprite spr = TFT_eSprite(&tft);
  Preferences prefs;
  AudioEngine audio;

  GameState state = GS_BOOT;

  unsigned long lastFrame = 0;
  unsigned long lastInputTime = 0;
  unsigned long bootStart = 0;
  unsigned long attractStart = 0;
  unsigned long lastEnemyMove = 0;
  unsigned long lastEnemyShot = 0;
  unsigned long lastFire = 0;
  unsigned long lastWeapon = 0;
  unsigned long lastDive = 0;
  unsigned long lastUFO = 0;
  unsigned long lastBossMove = 0;
  unsigned long lastBossPattern = 0;
  unsigned long pauseHold = 0;
  unsigned long lastMenuMove = 0;

  int score = 0;
  int highScore = 0;
  int wave = 1;
  int lives = 3;

  int playerX = 80;
  const int playerY = 198;
  const int playerW = 16;
  const int playerH = 12;

  WeaponType weapon = WPN_LASER;

  bool rapidFire = false;
  bool shieldActive = false;
  bool doubleShot = false;
  unsigned long rapidEnd = 0;
  unsigned long shieldEnd = 0;
  unsigned long doubleEnd = 0;

  Bullet pBullets[MAX_PBULLETS];
  Bullet eBullets[MAX_EBULLETS];
  Enemy enemies[MAX_ENEMIES];
  Diver divers[MAX_DIVERS];
  Particle particles[MAX_PARTICLES];
  PowerUp powerUps[MAX_POWERUPS];
  Shield shields[SHIELD_COUNT];
  Star stars[MAX_STARS];
  LeaderEntry leaders[MAX_LEADERS];

  int enemyDir = 1;
  int enemyMoveInterval = 300;
  int enemyShotInterval = 650;
  bool alienAnim = false;

  bool bossWave = false;
  bool bossAlive = false;
  int bossX = 50, bossY = 34, bossW = 72, bossH = 26;
  int bossHP = 0, bossMaxHP = 0;
  int bossDir = 1;
  int bossPhase = 1;
  int bossPattern = 0;

  bool ufoActive = false;
  int ufoX = -30, ufoY = 18, ufoDir = 1, ufoValue = 100;
  bool ufoBeamOn = false;
  int ufoBeamW = 10;

  bool attractEnabled = true;
  bool soundEnabled = true;
  bool crtEnabled = true;

  int menuIndex = 0;
  int settingsIndex = 0;

private:
  void loadSettings();
  void saveSettings();
  void saveHighScore();
  void loadLeaderboard();
  void saveLeaderboard();
  void insertLeaderboard(int s);

  void resetGame();
  void nextWave();
  void spawnEnemies();
  void spawnBoss();
  void spawnExplosion(int x, int y, uint16_t color = TFT_ORANGE, int count = 12);
  void spawnPowerup(int x, int y);
  void spawnUFO();
  void spawnDive();
  void initStars();
  void initShields();

  void updateInput();
  void updateBoot();
  void updateMenu();
  void updateSettings();
  void updateLeaderboard();
  void updateAttract();
  void updateGame();

  void updateStars();
  void updateBullets();
  void updateEnemies();
  void updateDivers();
  void updateBoss();
  void updateBossAttack();
  void updateParticles();
  void updatePowerups();
  void updateUFO();
  void updatePowerTimers();
  void checkCollisions();

  void firePlayer();
  void enemyShoot();
  void bossShootSpread();
  void bossShootRain();
  void bossShootAim();
  void applyPowerup(PowerType type);
  bool damageShieldAt(int x, int y, uint8_t amount = 1);
  void loseLife();

  void render();
  void drawBitmapTransparent(TFT_eSprite& dst, int x, int y, const uint16_t* bmp, int w, int h);
  void drawBackground();
  void drawHUD();
  void drawPlayer();
  void drawBullets();
  void drawEnemies();
  void drawDivers();
  void drawBoss();
  void drawParticles();
  void drawShields();
  void drawPowerups();
  void drawUFO();
  void drawMenu();
  void drawSettings();
  void drawLeaderboard();
  void drawPause();
  void drawWaveClear();
  void drawGameOver();
  void drawBoot();
  void drawCRT();
  void drawHearts(int x, int y, int count);
};