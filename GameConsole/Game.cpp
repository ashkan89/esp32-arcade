#include "esp32-hal-gpio.h"
#include "Game.h"

// ========================= BEGIN =========================
void Game::begin() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_FIRE, INPUT_PULLUP);
  pinMode(BTN_WEAPON, INPUT_PULLUP);
  pinMode(SIGNAL_LED, OUTPUT);
  pinMode(AMBIENT_LED, OUTPUT);

  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);

  spr.setColorDepth(16);
  spr.createSprite(SCREEN_W, SCREEN_H);

  prefs.begin("invadersv4", false);
  loadSettings();
  loadLeaderboard();

  audio.begin(BUZZER_PIN);
  audio.setEnabled(soundEnabled);
  audio.startMusic();

  ambientEnabled();

  randomSeed(analogRead(34));

  initStars();
  resetGame();

  bootStart = millis();
  state = GS_BOOT;
}

void Game::update() {
  if (millis() - lastFrame < FRAME_MS) return;
  lastFrame = millis();

  audio.update();
  updateInput();

  switch (state) {
    case GS_BOOT: updateBoot(); break;
    case GS_MENU: updateMenu(); break;
    case GS_SETTINGS: updateSettings(); break;
    case GS_LEADERBOARD: updateLeaderboard(); break;
    case GS_ATTRACT: updateAttract(); break;
    case GS_PLAYING: updateGame(); break;
    case GS_PAUSED: break;
    case GS_WAVE_CLEAR: break;
    case GS_GAME_OVER: break;
  }

  render();
}

// ========================= SETTINGS / SCORES =========================
void Game::loadSettings() {
  highScore = prefs.getInt("hiscore", 0);
  attractEnabled = prefs.getBool("attract", true);
  soundEnabled = prefs.getBool("sound", true);
  dificaulty = prefs.getInt("dificaulty", 1);
  crtEnabled = prefs.getBool("crt", true);
  ambientLightingEnabled = prefs.getBool("ambient", true);
}

void Game::saveSettings() {
  prefs.putBool("attract", attractEnabled);
  prefs.putBool("sound", soundEnabled);
  prefs.putInt("dificaulty", dificaulty);
  prefs.putBool("crt", crtEnabled);
  prefs.putBool("ambient", ambientLightingEnabled);
}

void Game::saveHighScore() {
  prefs.putInt("hiscore", highScore);
}

void Game::loadLeaderboard() {
  for (int i = 0; i < MAX_LEADERS; i++) {
    leaders[i].score = prefs.getInt(("lb" + String(i)).c_str(), 0);
  }
}

void Game::saveLeaderboard() {
  for (int i = 0; i < MAX_LEADERS; i++) {
    prefs.putInt(("lb" + String(i)).c_str(), leaders[i].score);
  }
}

void Game::insertLeaderboard(int s) {
  for (int i = 0; i < MAX_LEADERS; i++) {
    if (s > leaders[i].score) {
      for (int j = MAX_LEADERS - 1; j > i; j--) leaders[j] = leaders[j - 1];
      leaders[i].score = s;
      saveLeaderboard();
      break;
    }
  }
}

// ========================= RESET / WAVES =========================
void Game::resetGame() {
  score = 0;
  wave = 1;
  lives = 3;
  playerX = 80;
  weapon = WPN_LASER;

  rapidFire = shieldActive = doubleShot = false;
  ufoActive = false;
  ufoBeamOn = false;
  bossWave = false;
  bossAlive = false;

  for (auto &b : pBullets) b.active = false;
  for (auto &b : eBullets) b.active = false;
  for (auto &d : divers) d.active = false;
  for (auto &p : particles) p.active = false;
  for (auto &p : powerUps) p.active = false;

  enemyMoveInterval = 300;
  enemyShotInterval = 650;
  enemyDir = 1;

  spawnEnemies();
  initShields();
}

void Game::nextWave() {
  wave++;

  for (auto &b : pBullets) b.active = false;
  for (auto &b : eBullets) b.active = false;
  for (auto &d : divers) d.active = false;
  for (auto &p : particles) p.active = false;
  for (auto &p : powerUps) p.active = false;

  rapidFire = shieldActive = doubleShot = false;
  ufoActive = false;
  ufoBeamOn = false;

  bossWave = (wave % 5 == 0);
  if (bossWave) spawnBoss();
  else spawnEnemies();

  initShields();

  enemyMoveInterval = max(90, 300 - wave * 15);
  enemyShotInterval = max(220, 650 - wave * 25);
  state = GS_PLAYING;

  if (soundEnabled) audio.waveClear();
}

void Game::spawnEnemies() {
  bossAlive = false;
  int idx = 0;
  for (int r = 0; r < ENEMY_ROWS; r++) {
    for (int c = 0; c < ENEMY_COLS; c++) {
      enemies[idx].x = 12 + c * 24;
      enemies[idx].y = 34 + r * 18;
      enemies[idx].baseX = enemies[idx].x;
      enemies[idx].baseY = enemies[idx].y;
      enemies[idx].alive = true;

      if (r == 0) enemies[idx].type = ET_C;
      else if (r == 1) enemies[idx].type = ET_B;
      else enemies[idx].type = ET_A;

      enemies[idx].hp = (enemies[idx].type == ET_C) ? 2 : 1;
      idx++;
    }
  }
}

void Game::spawnBoss() {
  bossWave = true;
  bossAlive = true;
  bossX = 52;
  bossY = 34;
  bossDir = 1;
  bossPhase = 1;
  bossPattern = 0;
  bossMaxHP = 65 + wave * 5;
  bossHP = bossMaxHP;
  if (soundEnabled) audio.bossRoar();
}

void Game::spawnPowerup(int x, int y) {
  for (auto &p : powerUps) {
    if (!p.active) {
      p.active = true;
      p.x = x;
      p.y = y;
      p.type = (PowerType)(random(5) + 1);
      return;
    }
  }
}

void Game::spawnExplosion(int x, int y, uint16_t color, int count) {
  for (int i = 0; i < count; i++) {
    for (auto &p : particles) {
      if (!p.active) {
        p.active = true;
        p.x = x;
        p.y = y;
        p.vx = random(-20, 21) / 10.0f;
        p.vy = random(-20, 21) / 10.0f;
        p.life = random(8, 18);
        p.color = color;
        break;
      }
    }
  }
}

void Game::spawnUFO() {
  ufoActive = true;
  ufoBeamOn = false;
  ufoDir = random(2) ? 1 : -1;
  ufoX = (ufoDir > 0) ? -28 : SCREEN_W + 4;
  ufoY = 18;
  ufoValue = random(100, 301);
  if (soundEnabled) audio.ufo();
}

void Game::spawnDive() {
  for (int d = 0; d < MAX_DIVERS; d++) {
    if (divers[d].active) continue;

    int candidates[MAX_ENEMIES];
    int count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
      if (enemies[i].alive) candidates[count++] = i;
    }
    if (count == 0) return;

    int idx = candidates[random(count)];
    divers[d].enemyIndex = idx;
    divers[d].x = enemies[idx].x;
    divers[d].y = enemies[idx].y;
    divers[d].vx = (playerX - enemies[idx].x) / 28.0f;
    divers[d].vy = 2.2f + random(0, 10) / 10.0f;
    divers[d].active = true;
    enemies[idx].alive = false;
    return;
  }
}

void Game::initStars() {
  for (auto &s : stars) {
    s.x = random(SCREEN_W);
    s.y = random(PLAYFIELD_Y, SCREEN_H);
    s.speed = random(1, 4);
  }
}

void Game::initShields() {
  int pos[3] = { 22, 74, 126 };
  for (int s = 0; s < SHIELD_COUNT; s++) {
    shields[s].x = pos[s];
    shields[s].y = 168;
    for (int x = 0; x < SHIELD_W; x++) {
      for (int y = 0; y < SHIELD_H; y++) {
        shields[s].hp[x][y] = 3;
      }
    }
    for (int x = 7; x < 13; x++) {
      for (int y = 7; y < SHIELD_H; y++) {
        shields[s].hp[x][y] = 0;
      }
    }
  }
}

// ========================= INPUT =========================
void Game::updateInput() {
  static bool firePrev = false, leftPrev = false, rightPrev = false, weaponPrev = false;
  static GameState statePrev = state;

  bool fireNow = digitalRead(BTN_FIRE) == LOW;
  bool leftNow = digitalRead(BTN_LEFT) == LOW;
  bool rightNow = digitalRead(BTN_RIGHT) == LOW;
  bool weaponNow = digitalRead(BTN_WEAPON) == LOW;

  if (fireNow) digitalWrite(SIGNAL_LED, HIGH);
  if (millis() - lastInputTime > 25) digitalWrite(SIGNAL_LED, LOW);

  if (fireNow || leftNow || rightNow || weaponNow) lastInputTime = millis();

  if (leftNow && rightNow && fireNow) {
    if (pauseHold == 0) pauseHold = millis();
    if (millis() - pauseHold > 700) {
      if (state == GS_PLAYING) state = GS_PAUSED;
      else if (state == GS_PAUSED) state = GS_PLAYING;
      pauseHold = 0;
    }
  } else pauseHold = 0;

  if ((leftNow && !leftPrev) || (rightNow && !rightPrev)) {
    if (millis() - lastMenuMove > 140) {
      lastMenuMove = millis();

      if (state == GS_MENU) {
        menuIndex += rightNow ? 1 : -1;
        if (menuIndex < 0) menuIndex = 3;
        if (menuIndex > 3) menuIndex = 0;
        if (soundEnabled) audio.menuMove();
      } else if (state == GS_SETTINGS) {
        settingsIndex += rightNow ? 1 : -1;
        if (settingsIndex < 0) settingsIndex = 4;
        if (settingsIndex > 4) settingsIndex = 0;
        if (soundEnabled) audio.menuMove();
      }
    }
  }

  if (state == GS_SETTINGS && fireNow && !firePrev && statePrev == GS_SETTINGS) {
    if (settingsIndex == 0) soundEnabled = !soundEnabled;
    else if (settingsIndex == 1) dificaulty >= 3 ? dificaulty = 1 : ++dificaulty;
    else if (settingsIndex == 2) attractEnabled = !attractEnabled;
    else if (settingsIndex == 3) crtEnabled = !crtEnabled;
    else if (settingsIndex == 4) ambientLightingEnabled = !ambientLightingEnabled;

    audio.setEnabled(soundEnabled);
    ambientEnabled();
    saveSettings();

    if (soundEnabled) audio.menuSelect();
  }

  if (state == GS_WAVE_CLEAR && fireNow && !firePrev) {
    nextWave();
  }

  if (state == GS_PLAYING && weaponNow && !weaponPrev && millis() - lastWeapon > 180) {
    lastWeapon = millis();
    weapon = (WeaponType)((weapon + 1) % 3);
    if (soundEnabled) audio.menuMove();
  }

  if (state == GS_MENU && fireNow && !firePrev) {
    if (menuIndex == 0) {
      resetGame();
      state = GS_PLAYING;
    } else if (menuIndex == 1) {
      state = GS_SETTINGS;
    } else if (menuIndex == 2) {
      state = GS_LEADERBOARD;
    } else if (menuIndex == 3) {
      resetGame();
      state = GS_ATTRACT;
      attractStart = millis();
    }
    if (soundEnabled) audio.menuSelect();
  }

  if ((state == GS_SETTINGS || state == GS_LEADERBOARD || state == GS_ATTRACT) && leftNow && rightNow && !fireNow) {
    if (state = GS_SETTINGS)
      settingsIndex = 0;

    state = GS_MENU;
  }

  if (state == GS_GAME_OVER && fireNow && !firePrev) {
    state = GS_MENU;
  }

  if (state == GS_PAUSED && fireNow && !firePrev) {
    state = GS_PLAYING;
  }

  firePrev = fireNow;
  leftPrev = leftNow;
  rightPrev = rightNow;
  weaponPrev = weaponNow;
  statePrev = state;
}

// ========================= STATES =========================
void Game::updateBoot() {
  updateStars();
  if (millis() - bootStart > 3200) state = GS_MENU;
}

void Game::updateMenu() {
  updateStars();
  if (attractEnabled && millis() - lastInputTime > ATTRACT_TIMEOUT) {
    resetGame();
    state = GS_ATTRACT;
    attractStart = millis();
  }
}

void Game::updateSettings() {
  updateStars();
}

void Game::updateLeaderboard() {
  updateStars();
}

void Game::updateAttract() {
  updateStars();

  int targetX = playerX;
  if (bossWave && bossAlive) targetX = bossX + bossW / 2 - 8;
  else {
    for (auto &e : enemies) {
      if (e.alive) {
        targetX = e.x;
        break;
      }
    }
  }

  if (targetX < playerX) playerX -= 2;
  if (targetX > playerX) playerX += 2;
  playerX = constrain(playerX, 2, SCREEN_W - playerW - 2);

  if (millis() - lastFire > 260) {
    firePlayer();
    lastFire = millis();
  }

  updateBullets();
  updateEnemies();
  updateDivers();
  updateBoss();
  updateParticles();
  updatePowerups();
  updateUFO();
  updatePowerTimers();
  checkCollisions();

  bool anyAlive = false;
  if (bossWave) anyAlive = bossAlive;
  else {
    for (auto &e : enemies)
      if (e.alive) {
        anyAlive = true;
        break;
      }
    for (auto &d : divers)
      if (d.active) {
        anyAlive = true;
        break;
      }
  }

  if (!anyAlive) nextWave();
  if (millis() - attractStart > 22000) state = GS_MENU;
}

void Game::updateGame() {
  bool left = digitalRead(BTN_LEFT) == LOW;
  bool right = digitalRead(BTN_RIGHT) == LOW;
  bool fire = digitalRead(BTN_FIRE) == LOW;

  if (left) playerX -= 3;
  if (right) playerX += 3;
  playerX = constrain(playerX, 2, SCREEN_W - playerW - 2);

  int fireCooldown = rapidFire ? 80 : 200;
  if (fire && millis() - lastFire > fireCooldown) {
    firePlayer();
    lastFire = millis();
  }

  updatePowerTimers();
  updateStars();
  updateBullets();
  updateEnemies();
  updateDivers();
  updateBoss();
  updateParticles();
  updatePowerups();
  updateUFO();
  checkCollisions();

  bool anyAlive = false;
  if (bossWave) anyAlive = bossAlive;
  else {
    for (auto &e : enemies)
      if (e.alive) {
        anyAlive = true;
        break;
      }
    for (auto &d : divers)
      if (d.active) {
        anyAlive = true;
        break;
      }
  }

  if (!anyAlive) {
    state = GS_WAVE_CLEAR;
    if (score > highScore) {
      highScore = score;
      saveHighScore();
    }
  }

  if (!ufoActive && millis() - lastUFO > 15000 && random(100) < 3) {
    spawnUFO();
    lastUFO = millis();
  }
}

// ========================= UPDATES =========================
void Game::updateStars() {
  for (auto &s : stars) {
    s.y += s.speed;
    if (s.y >= SCREEN_H) {
      s.y = PLAYFIELD_Y;
      s.x = random(SCREEN_W);
    }
  }
}

void Game::updateBullets() {
  for (auto &b : pBullets) {
    if (!b.active) continue;
    b.x += b.vx;
    b.y += b.vy;
    if (b.y < PLAYFIELD_Y || b.x < 0 || b.x >= SCREEN_W) b.active = false;
  }

  for (auto &b : eBullets) {
    if (!b.active) continue;
    b.x += b.vx;
    b.y += b.vy;
    if (b.y > SCREEN_H || b.x < 0 || b.x >= SCREEN_W) b.active = false;
  }
}

void Game::updateEnemies() {
  if (bossWave) return;
  if (millis() - lastEnemyMove < (unsigned long)enemyMoveInterval) return;
  lastEnemyMove = millis();
  alienAnim = !alienAnim;

  int minX = 999, maxX = -999;
  for (auto &e : enemies) {
    if (!e.alive) continue;
    minX = min(minX, e.x);
    maxX = max(maxX, e.x);
  }

  bool edge = false;
  if (maxX + 14 >= SCREEN_W - 4 && enemyDir > 0) edge = true;
  if (minX <= 4 && enemyDir < 0) edge = true;

  for (auto &e : enemies) {
    if (!e.alive) continue;
    if (edge) e.y += 8;
    else {
      int drift = (e.type == ET_B) ? (alienAnim ? 1 : -1) : 0;
      e.x += enemyDir * 2 + drift;
    }

    if (e.y + 10 >= playerY - 2) {
      state = GS_GAME_OVER;
      if (soundEnabled) audio.gameOver();
    }
  }

  if (edge) enemyDir *= -1;

  if (millis() - lastEnemyShot > (unsigned long)enemyShotInterval) {
    lastEnemyShot = millis();
    enemyShoot();
  }

  if (wave >= 3 && millis() - lastDive > 2500 && random(100) < 30) {
    lastDive = millis();
    spawnDive();
  }
}

void Game::updateDivers() {
  for (auto &d : divers) {
    if (!d.active) continue;
    d.x += d.vx;
    d.y += d.vy;
    d.vx += sin(millis() * 0.01f) * 0.02f;

    if (random(100) < 3) {
      for (auto &b : eBullets) {
        if (!b.active) {
          b.active = true;
          b.playerOwned = false;
          b.x = (int)d.x + 7;
          b.y = (int)d.y + 10;
          b.vx = 0;
          b.vy = 4;
          b.size = 2;
          b.color = TFT_RED;
          break;
        }
      }
    }

    if (d.y > SCREEN_H) d.active = false;
  }
}

void Game::updateBoss() {
  if (!bossAlive) return;

  if (bossHP < bossMaxHP * 0.66f) bossPhase = 2;
  if (bossHP < bossMaxHP * 0.33f) bossPhase = 3;

  if (millis() - lastBossMove > (unsigned long)(bossPhase == 1 ? 70 : bossPhase == 2 ? 45
                                                                                     : 28)) {
    lastBossMove = millis();
    bossX += bossDir * (bossPhase == 3 ? 4 : bossPhase == 2 ? 3
                                                            : 2);
    if (bossX <= 4 || bossX + bossW >= SCREEN_W - 4) bossDir *= -1;
  }

  updateBossAttack();
}

void Game::updateBossAttack() {
  unsigned long interval = (bossPhase == 1 ? 900 : bossPhase == 2 ? 550
                                                                  : 300);
  if (millis() - lastBossPattern < interval) return;
  lastBossPattern = millis();

  bossPattern = (bossPattern + 1) % 3;

  if (bossPattern == 0) bossShootSpread();
  else if (bossPattern == 1) bossShootRain();
  else bossShootAim();
}

void Game::bossShootSpread() {
  for (int i = -2; i <= 2; i++) {
    for (auto &b : eBullets) {
      if (!b.active) {
        b.active = true;
        b.playerOwned = false;
        b.x = bossX + bossW / 2;
        b.y = bossY + bossH;
        b.vx = i;
        b.vy = 3 + bossPhase;
        b.size = 2;
        b.color = TFT_MAGENTA;
        break;
      }
    }
  }
}

void Game::bossShootRain() {
  for (int i = 0; i < 5 + bossPhase; i++) {
    for (auto &b : eBullets) {
      if (!b.active) {
        b.active = true;
        b.playerOwned = false;
        b.x = random(10, SCREEN_W - 10);
        b.y = bossY + bossH - 2;
        b.vx = 0;
        b.vy = 3 + bossPhase;
        b.size = 2;
        b.color = TFT_RED;
        break;
      }
    }
  }
}

void Game::bossShootAim() {
  int dx = (playerX + 8) - (bossX + bossW / 2);
  int vx = dx / 18;
  vx = constrain(vx, -3, 3);

  for (int i = -1; i <= 1; i++) {
    for (auto &b : eBullets) {
      if (!b.active) {
        b.active = true;
        b.playerOwned = false;
        b.x = bossX + bossW / 2 + i * 10;
        b.y = bossY + bossH;
        b.vx = vx + i;
        b.vy = 4 + bossPhase;
        b.size = 2;
        b.color = TFT_YELLOW;
        break;
      }
    }
  }
}

void Game::updateParticles() {
  for (auto &p : particles) {
    if (!p.active) continue;
    p.x += p.vx;
    p.y += p.vy;
    p.vy += 0.08f;
    if (p.life > 0) p.life--;
    else p.active = false;
  }
}

void Game::updatePowerups() {
  for (auto &p : powerUps) {
    if (!p.active) continue;
    p.y += 2;
    if (p.y > SCREEN_H) p.active = false;

    if (p.x >= playerX && p.x <= playerX + playerW && p.y >= playerY && p.y <= playerY + playerH) {
      applyPowerup(p.type);
      p.active = false;
    }
  }
}

void Game::updateUFO() {
  if (!ufoActive) return;
  ufoX += ufoDir * 2;

  if (random(100) < 2) ufoBeamOn = !ufoBeamOn;

  if (ufoBeamOn) {
    int beamX = ufoX + 12;
    if (beamX >= playerX && beamX <= playerX + playerW && playerY > ufoY) {
      if (!shieldActive && random(100) < 4) loseLife();
    }
  }

  if (ufoX < -40 || ufoX > SCREEN_W + 40) {
    ufoActive = false;
    ufoBeamOn = false;
  }
}

void Game::updatePowerTimers() {
  if (rapidFire && millis() > rapidEnd) rapidFire = false;
  if (shieldActive && millis() > shieldEnd) shieldActive = false;
  if (doubleShot && millis() > doubleEnd) doubleShot = false;
}

// ========================= FIRE =========================
void Game::firePlayer() {
  if (weapon == WPN_LASER) {
    if (soundEnabled) audio.laser();
    for (auto &b : pBullets) {
      if (!b.active) {
        b.active = true;
        b.playerOwned = true;
        b.vx = 0;
        b.vy = -7;
        b.size = 2;
        b.color = TFT_CYAN;
        b.x = playerX + 8;
        b.y = playerY - 4;
        break;
      }
    }
  } else if (weapon == WPN_SPREAD) {
    if (soundEnabled) audio.spread();
    int dirs[3] = { -2, 0, 2 };
    for (int i = 0; i < 3; i++) {
      for (auto &b : pBullets) {
        if (!b.active) {
          b.active = true;
          b.playerOwned = true;
          b.vx = dirs[i];
          b.vy = -5;
          b.size = 2;
          b.color = TFT_YELLOW;
          b.x = playerX + 8;
          b.y = playerY - 4;
          break;
        }
      }
    }
  } else if (weapon == WPN_PLASMA) {
    if (soundEnabled) audio.plasma();
    for (auto &b : pBullets) {
      if (!b.active) {
        b.active = true;
        b.playerOwned = true;
        b.vx = 0;
        b.vy = -4;
        b.size = 4;
        b.color = TFT_MAGENTA;
        b.x = playerX + 7;
        b.y = playerY - 6;
        break;
      }
    }
  }
}

void Game::enemyShoot() {
  int aliveCols[ENEMY_COLS];
  for (int i = 0; i < ENEMY_COLS; i++) aliveCols[i] = -1;

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].alive) continue;
    int col = i % ENEMY_COLS;
    if (aliveCols[col] == -1 || enemies[i].y > enemies[aliveCols[col]].y) aliveCols[col] = i;
  }

  int options[ENEMY_COLS];
  int count = 0;
  for (int i = 0; i < ENEMY_COLS; i++)
    if (aliveCols[i] != -1) options[count++] = aliveCols[i];
  if (count == 0) return;

  int shooter = options[random(count)];

  for (auto &b : eBullets) {
    if (!b.active) {
      b.active = true;
      b.playerOwned = false;
      b.x = enemies[shooter].x + 7;
      b.y = enemies[shooter].y + 10;
      b.vx = (enemies[shooter].type == ET_C) ? random(-1, 2) : 0;
      b.vy = 4 + min(3, wave / 3);
      b.size = 2;
      b.color = TFT_RED;
      break;
    }
  }
}

// ========================= HELPERS =========================
void Game::applyPowerup(PowerType type) {
  if (soundEnabled) audio.powerup();

  switch (type) {
    case P_RAPID:
      rapidFire = true;
      rapidEnd = millis() + 8000;
      break;
    case P_LIFE:
      if (lives < 5) lives++;
      break;
    case P_SHIELD:
      shieldActive = true;
      shieldEnd = millis() + 8000;
      break;
    case P_DOUBLESHOT:
      doubleShot = true;
      doubleEnd = millis() + 8000;
      break;
    case P_WEAPONUP:
      weapon = (WeaponType)((weapon + 1) % 3);
      break;
    default: break;
  }
}

bool Game::damageShieldAt(int x, int y, uint8_t amount) {
  for (auto &s : shields) {
    int lx = x - s.x;
    int ly = y - s.y;
    if (lx >= 0 && lx < SHIELD_W && ly >= 0 && ly < SHIELD_H) {
      for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
          int px = lx + dx, py = ly + dy;
          if (px >= 0 && px < SHIELD_W && py >= 0 && py < SHIELD_H) {
            if (s.hp[px][py] > 0) s.hp[px][py] = max(0, s.hp[px][py] - amount);
          }
        }
      }
      return true;
    }
  }
  return false;
}

void Game::loseLife() {
  spawnExplosion(playerX + 8, playerY + 5, TFT_RED, 16);
  if (soundEnabled) audio.gameOver();
  lives--;
  playerX = 80;

  if (lives <= 0) {
    state = GS_GAME_OVER;
    if (score > highScore) {
      highScore = score;
      saveHighScore();
    }
    insertLeaderboard(score);
  }
}

// ========================= COLLISIONS =========================
void Game::checkCollisions() {
  for (auto &b : pBullets) {
    if (!b.active) continue;

    if (ufoActive && b.x >= ufoX && b.x <= ufoX + 24 && b.y >= ufoY && b.y <= ufoY + 10) {
      b.active = false;
      ufoActive = false;
      ufoBeamOn = false;
      score += ufoValue;
      spawnExplosion(ufoX + 12, ufoY + 5, TFT_ORANGE, 18);
      if (soundEnabled) audio.explosion();
      continue;
    }

    if (bossWave && bossAlive && b.x >= bossX && b.x <= bossX + bossW && b.y >= bossY && b.y <= bossY + bossH) {
      b.active = false;
      bossHP -= (weapon == WPN_PLASMA ? 2 : 1);
      spawnExplosion(b.x, b.y, TFT_YELLOW, 6);
      if (soundEnabled) audio.hit();
      if (bossHP <= 0) {
        bossAlive = false;
        score += 350;
        spawnExplosion(bossX + bossW / 2, bossY + bossH / 2, TFT_RED, 30);
        if (soundEnabled) audio.explosion();
      }
      continue;
    }

    for (auto &d : divers) {
      if (!d.active) continue;
      if (b.x >= d.x && b.x <= d.x + 14 && b.y >= d.y && b.y <= d.y + 10) {
        b.active = false;
        d.active = false;
        score += 15;
        spawnExplosion((int)d.x + 7, (int)d.y + 5, TFT_ORANGE, 12);
        if (soundEnabled) audio.explosion();
        break;
      }
    }

    for (auto &e : enemies) {
      if (!e.alive) continue;
      if (b.x >= e.x && b.x <= e.x + 14 && b.y >= e.y && b.y <= e.y + 10) {
        b.active = false;
        e.hp--;
        if (e.hp <= 0) {
          e.alive = false;
          score += (e.type == ET_C) ? 20 : (e.type == ET_B ? 15 : 10);
          spawnExplosion(e.x + 7, e.y + 5, TFT_ORANGE, 10);
          if (random(100) < 14) spawnPowerup(e.x + 7, e.y + 5);
          if (soundEnabled) audio.explosion();
        } else {
          if (soundEnabled) audio.hit();
        }
        break;
      }
    }

    if (damageShieldAt(b.x, b.y, (weapon == WPN_PLASMA) ? 3 : 2)) b.active = false;
  }

  for (auto &b : eBullets) {
    if (!b.active) continue;

    if (b.x >= playerX && b.x <= playerX + playerW && b.y >= playerY && b.y <= playerY + playerH) {
      b.active = false;
      if (shieldActive) {
        shieldActive = false;
        spawnExplosion(playerX + 8, playerY + 5, TFT_CYAN, 8);
      } else {
        loseLife();
      }
      continue;
    }

    if (damageShieldAt(b.x, b.y, 1)) b.active = false;
  }
}

// ========================= DRAW HELPERS =========================
void Game::drawBitmapTransparent(TFT_eSprite &dst, int x, int y, const uint16_t *bmp, int w, int h) {
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      uint16_t c = pgm_read_word(&bmp[yy * w + xx]);
      if (c != 0) dst.drawPixel(x + xx, y + yy, c);
    }
  }
}

// ========================= RENDER =========================
void Game::drawBackground() {
  for (auto &s : stars) {
    uint16_t c = (s.speed == 1) ? STAR1_COLOR : (s.speed == 2 ? STAR2_COLOR : STAR3_COLOR);
    spr.drawPixel(s.x, s.y, c);
  }
}

void Game::drawHUD() {
  spr.fillRect(0, 0, SCREEN_W, PLAYFIELD_Y, HUD_BG);
  spr.setTextColor(TFT_WHITE, HUD_BG);
  spr.setTextSize(1);
  spr.setCursor(2, 3);
  spr.printf("S:%d H:%d W:%d", score, highScore, wave);

  const char *wpn = (weapon == WPN_LASER) ? "LAS" : (weapon == WPN_SPREAD ? "SPR" : "PLA");
  spr.setCursor(80, 3);
  spr.printf("W:%s", wpn);

  drawHearts(130, 2, lives);
}

void Game::drawHearts(int x, int y, int count) {
  for (int i = 0; i < count; i++) {
    int hx = x + i * 12;
    spr.fillCircle(hx + 2, y + 2, 2, TFT_RED);
    spr.fillCircle(hx + 6, y + 2, 2, TFT_RED);
    spr.fillTriangle(hx, y + 3, hx + 8, y + 3, hx + 4, y + 9, TFT_RED);
  }
}

void Game::drawPlayer() {
  drawBitmapTransparent(spr, playerX, playerY, playerSprite, 16, 12);
  if (shieldActive) spr.drawCircle(playerX + 8, playerY + 6, 12, TFT_CYAN);
}

void Game::drawBullets() {
  for (auto &b : pBullets)
    if (b.active) spr.fillRect(b.x, b.y, b.size, max(4, b.size + 1), b.color);
  for (auto &b : eBullets)
    if (b.active) spr.fillRect(b.x, b.y, b.size, 5, b.color);
}

void Game::drawEnemies() {
  if (bossWave) return;
  for (auto &e : enemies) {
    if (!e.alive) continue;
    if (e.type == ET_A) drawBitmapTransparent(spr, e.x, e.y, alienAnim ? alienA : alienB, 14, 10);
    else if (e.type == ET_B) drawBitmapTransparent(spr, e.x, e.y, alienB, 14, 10);
    else drawBitmapTransparent(spr, e.x, e.y, alienC, 14, 10);
  }
}

void Game::drawDivers() {
  for (auto &d : divers) {
    if (!d.active) continue;
    drawBitmapTransparent(spr, (int)d.x, (int)d.y, alienC, 14, 10);
  }
}

void Game::drawBoss() {
  spr.fillRoundRect(bossX, bossY, bossW, bossH, 4, BOSS_COLOR);
  spr.fillRect(bossX + 8, bossY + 6, 10, 6, TFT_BLACK);
  spr.fillRect(bossX + 54, bossY + 6, 10, 6, TFT_BLACK);
  spr.drawFastHLine(bossX + 20, bossY + 17, 30, TFT_BLACK);
  spr.fillTriangle(bossX + 8, bossY, bossX + 2, bossY - 6, bossX + 12, bossY, TFT_RED);
  spr.fillTriangle(bossX + bossW - 8, bossY, bossX + bossW - 2, bossY - 6, bossX + bossW - 12, bossY, TFT_RED);

  spr.drawRect(38, 14, 100, 6, TFT_WHITE);
  int hpBar = map(bossHP, 0, bossMaxHP, 0, 98);
  spr.fillRect(39, 15, hpBar, 4, TFT_RED);
}

void Game::drawParticles() {
  for (auto &p : particles) {
    if (!p.active) continue;
    spr.drawPixel((int)p.x, (int)p.y, p.color);
  }
}

void Game::drawShields() {
  for (auto &s : shields) {
    for (int x = 0; x < SHIELD_W; x++) {
      for (int y = 0; y < SHIELD_H; y++) {
        if (s.hp[x][y] == 0) continue;
        uint16_t c = SHIELD_COLOR;
        if (s.hp[x][y] == 2) c = TFT_CYAN;
        if (s.hp[x][y] == 1) c = TFT_YELLOW;
        spr.drawPixel(s.x + x, s.y + y, c);
      }
    }
  }
}

void Game::drawPowerups() {
  for (auto &p : powerUps) {
    if (!p.active) continue;
    uint16_t c = TFT_YELLOW;
    if (p.type == P_LIFE) c = TFT_RED;
    else if (p.type == P_SHIELD) c = TFT_CYAN;
    else if (p.type == P_DOUBLESHOT) c = TFT_GREENYELLOW;
    else if (p.type == P_WEAPONUP) c = TFT_ORANGE;
    spr.fillCircle(p.x, p.y, 3, c);
  }
}

void Game::drawUFO() {
  if (!ufoActive) return;
  drawBitmapTransparent(spr, ufoX, ufoY, ufoSprite, 24, 10);

  if (ufoBeamOn) {
    spr.fillRect(ufoX + 10, ufoY + 10, ufoBeamW, SCREEN_H - (ufoY + 10), TFT_DARKCYAN);
    spr.drawFastVLine(ufoX + 15, ufoY + 10, SCREEN_H - (ufoY + 10), TFT_CYAN);
  }
}

void Game::drawMenu() {
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_GREEN, BG_COLOR);
  spr.setTextSize(2);
  spr.drawString("SPACE", SCREEN_W / 2, 44);
  spr.drawString("INVADERS", SCREEN_W / 2, 68);

  spr.setTextColor(TFT_CYAN, BG_COLOR);
  spr.setTextSize(1);
  spr.drawString("V4 ULTRA CABINET", SCREEN_W / 2, 88);

  const char *items[4] = { "START", "SETTINGS", "LEADERBOARD", "ATTRACT" };
  for (int i = 0; i < 4; i++) {
    spr.setTextColor(i == menuIndex ? TFT_YELLOW : TFT_WHITE, BG_COLOR);
    spr.drawString(items[i], SCREEN_W / 2, 116 + i * 16);
  }

  spr.setTextColor(TFT_LIGHTGREY, BG_COLOR);
  spr.drawString("High Score: " + String(highScore), SCREEN_W / 2, 188);
  spr.drawString("L/R=select  FIRE=OK", SCREEN_W / 2, 202);
}

void Game::drawSettings() {
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_GREENYELLOW, BG_COLOR);
  spr.setTextSize(2);
  spr.drawString("SETTINGS", SCREEN_W / 2, 42);

  spr.setTextSize(1);
  String items[5] = {
    String("SOUND: ") + (soundEnabled ? "ON" : "OFF"),
    String("DIFICAULTY: ") + (dificaulty == 1 ? "EASY" : dificaulty == 2 ? "NORMAL"
                                                                         : "HARD"),
    String("ATTRACT: ") + (attractEnabled ? "ON" : "OFF"),
    String("CRT FX: ") + (crtEnabled ? "ON" : "OFF"),
    String("AMBIENT LIGHT: ") + (ambientLightingEnabled ? "ON" : "OFF")
  };

  for (int i = 0; i < 5; i++) {
    spr.setTextColor(i == settingsIndex ? TFT_YELLOW : TFT_WHITE, BG_COLOR);
    spr.drawString(items[i], SCREEN_W / 2, 72 + i * 20);
  }

  spr.setTextColor(TFT_LIGHTGREY, BG_COLOR);
  spr.drawString("FIRE = toggle", SCREEN_W / 2, 174);
  spr.drawString("LEFT+RIGHT = back", SCREEN_W / 2, 190);
}

void Game::drawLeaderboard() {
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_ORANGE, BG_COLOR);
  spr.setTextSize(2);
  spr.drawString("TOP 5", SCREEN_W / 2, 40);

  spr.setTextSize(1);
  for (int i = 0; i < MAX_LEADERS; i++) {
    spr.setTextColor(i == 0 ? TFT_YELLOW : TFT_WHITE, BG_COLOR);
    spr.drawString(String(i + 1) + ". " + String(leaders[i].score), SCREEN_W / 2, 80 + i * 20);
  }

  spr.setTextColor(TFT_LIGHTGREY, BG_COLOR);
  spr.drawString("LEFT+RIGHT = back", SCREEN_W / 2, 190);
}

void Game::drawPause() {
  spr.fillRect(40, 84, 96, 38, TFT_DARKGREY);
  spr.drawRect(40, 84, 96, 38, TFT_WHITE);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_DARKGREY);
  spr.setTextSize(2);
  spr.drawString("PAUSE", SCREEN_W / 2, 103);
}

void Game::drawWaveClear() {
  spr.fillRect(28, 74, 120, 64, TFT_NAVY);
  spr.drawRect(28, 74, 120, 64, TFT_WHITE);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_GREENYELLOW, TFT_NAVY);
  spr.setTextSize(2);
  spr.drawString(bossWave ? "BOSS" : "WAVE", SCREEN_W / 2, 92);
  spr.drawString("CLEAR!", SCREEN_W / 2, 112);
  spr.setTextColor(TFT_WHITE, TFT_NAVY);
  spr.setTextSize(1);
  spr.drawString("Press FIRE", SCREEN_W / 2, 128);
}

void Game::drawGameOver() {
  spr.fillRect(22, 68, 132, 84, TFT_DARKGREY);
  spr.drawRect(22, 68, 132, 84, TFT_RED);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_RED, TFT_DARKGREY);
  spr.setTextSize(2);
  spr.drawString("GAME OVER", SCREEN_W / 2, 92);
  spr.setTextColor(TFT_WHITE, TFT_DARKGREY);
  spr.setTextSize(1);
  spr.drawString("Score: " + String(score), SCREEN_W / 2, 116);
  spr.drawString("Best: " + String(highScore), SCREEN_W / 2, 130);
  spr.drawString("Press FIRE", SCREEN_W / 2, 144);
}

void Game::drawBoot() {
  int t = millis() - bootStart;
  int alphaBar = map(min(t, 2500), 0, 2500, 0, 120);

  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_GREEN, BG_COLOR);
  spr.setTextSize(2);
  if (t > 400) spr.drawString("SPACE", SCREEN_W / 2, 60);
  if (t > 900) spr.drawString("INVADERS", SCREEN_W / 2, 86);

  spr.setTextColor(TFT_CYAN, BG_COLOR);
  spr.setTextSize(1);
  if (t > 1400) spr.drawString("ULTRA CABINET V4", SCREEN_W / 2, 110);

  spr.drawRect(28, 150, 120, 10, TFT_WHITE);
  spr.fillRect(29, 151, alphaBar, 8, TFT_GREENYELLOW);
}

void Game::drawCRT() {
  if (!crtEnabled) return;
  for (int y = PLAYFIELD_Y; y < SCREEN_H; y += 2) {
    spr.drawFastHLine(0, y, SCREEN_W, CRT_LINE);
  }
}

void Game::ambientEnabled() {
  if (ambientLightingEnabled) {
    digitalWrite(AMBIENT_LED, HIGH);
  } else {
    digitalWrite(AMBIENT_LED, LOW);
  }
}

void Game::render() {
  spr.fillSprite(BG_COLOR);
  drawBackground();

  switch (state) {
    case GS_BOOT:
      drawBoot();
      break;

    case GS_MENU:
      drawMenu();
      break;

    case GS_SETTINGS:
      drawSettings();
      break;

    case GS_LEADERBOARD:
      drawLeaderboard();
      break;

    case GS_ATTRACT:
    case GS_PLAYING:
    case GS_PAUSED:
    case GS_WAVE_CLEAR:
    case GS_GAME_OVER:
      drawShields();
      drawBullets();
      drawEnemies();
      drawDivers();
      drawUFO();
      if (bossWave && bossAlive) drawBoss();
      drawParticles();
      drawPowerups();
      drawPlayer();
      drawHUD();

      if (state == GS_PAUSED) drawPause();
      if (state == GS_WAVE_CLEAR) drawWaveClear();
      if (state == GS_GAME_OVER) drawGameOver();
      break;
  }

  drawCRT();
  spr.pushSprite(0, 0);
}