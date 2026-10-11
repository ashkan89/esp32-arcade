#include "arcade/Console.h"
#include "assets/Visuals.h"
#include <cstdio>
namespace arcade {
void Console::transition(Screen s, uint32_t now) {
  screen = s;
  entered = now;
  cursor = 0;
  suppressNeeded = true;
  pending = Sound::None;
}
void Console::begin(GameOptions o, uint32_t now) {
  visualTime = now;
  options = o;
  selection = settings.lastGame < GameCount ? settings.lastGame : 0;
  lastAction = now;
  transition(Screen::Boot, now);
}
bool Console::confirm(const InputFrame &f) const {
  return f.hit(Fire);
}
int Console::navigation(const InputFrame &f) const {
  return int(f.nav(Right) || f.nav(Down)) - int(f.nav(Left) || f.nav(Up));
}
void Console::start(uint32_t now) {
  if (current)
    current->cleanup();
  current = &gameFor(GameId(selection));
  current->initialize();
  options.difficulty = settings.difficulty;
  options.layout = settings.layout;
  options.effects = settings.effects;
  options.seed ^= now + 0x9e3779b9;
  current->start(options);
  newRun = true;
  recorded = false;
  if (settings.lastGame != selection) {
    settings.lastGame = uint8_t(selection);
    dirty = true;
  }
  transition(Screen::Playing, now);
}
void Console::record() {
  if (current && !recorded) {
    dirty |= settings.record(unsigned(selection), current->score());
    recorded = true;
  }
}
void Console::leave(uint32_t now) {
  record();
  if (current) {
    current->cleanup();
    current = nullptr;
  }
  lastAction = now;
  transition(Screen::Launcher, now);
}
void Console::update(const InputFrame &f, uint32_t now) {
  visualTime = now;
  if (f.pressed || f.released || f.repeated)
    lastAction = now;
  if (screen == Screen::Boot) {
    if (elapsed(now, entered) > 1100 || f.pressed)
      transition(Screen::Launcher, now);
    return;
  }
  if (screen == Screen::Attract) {
    if (f.pressed || elapsed(now, entered) > 15000) {
      if (current)
        current->cleanup();
      current = nullptr;
      lastAction = now;
      transition(Screen::Launcher, now);
      return;
    }
    InputFrame demo;
    demo.held = Fire | ((now / 1300) % 2 ? Left : Right);
    current->handle(demo);
    current->update();
    if (current->gameOver()) {
      GameOptions o = options;
      o.seed = now | 1;
      current->start(o);
    }
    return;
  }
  if (screen == Screen::Playing) {
    if (f.hit(Back) || f.hit(Pause)) {
      current->pause();
      transition(Screen::Paused, now);
      return;
    }
    current->handle(f);
    current->update();
    pending = current->takeSound();
    if (current->gameOver()) {
      record();
      transition(Screen::Over, now);
      pending = current->campaignComplete() ? Sound::Wave : Sound::Over;
    }
    return;
  }
  int nav = navigation(f);
  bool ok = confirm(f), back = f.hit(Back) || f.hit(Pause);
  if (screen == Screen::Launcher) {
    int move = int(f.nav(Down)) - int(f.nav(Up));
    if (f.nav(Right))
      move = 5;
    else if (f.nav(Left))
      move = -5;
    if (move) {
      selection = (selection + move + int(GameCount)) % int(GameCount);
      pending = Sound::Move;
    }
    if (f.hit(Special)) {
      settings.favorites ^= uint64_t(1) << selection;
      dirty = true;
      pending = Sound::Select;
    }
    if (ok) {
      fromPause = false;
      returnTo = Screen::Launcher;
      transition(Screen::Instructions, now);
      pending = Sound::Select;
    } else if (back)
      transition(Screen::Utilities, now);
    else if (settings.attract && elapsed(now, lastAction) > 20000) {
      current = &gameFor(GameId::Invaders);
      GameOptions o = options;
      o.seed = now | 1;
      current->start(o);
      transition(Screen::Attract, now);
    }
    return;
  }
  // B is the conventional back action outside the launcher and games.
  back |= f.hit(Special);
  if (screen == Screen::Instructions) {
    if (ok) {
      if (fromPause)
        transition(Screen::Paused, now);
      else
        start(now);
    } else if (back)
      transition(returnTo, now);
    return;
  }
  if (screen == Screen::Paused) {
    if (nav) {
      cursor = (cursor + nav + 4) % 4;
      pending = Sound::Move;
    }
    if (back) {
      current->resume();
      transition(Screen::Playing, now);
    } else if (ok) {
      if (cursor == 0) {
        current->resume();
        transition(Screen::Playing, now);
      } else if (cursor == 1) {
        record();
        start(now);
      } else if (cursor == 2) {
        fromPause = true;
        returnTo = Screen::Paused;
        transition(Screen::Instructions, now);
      } else
        leave(now);
    }
    return;
  }
  if (screen == Screen::Over) {
    if (back)
      leave(now);
    else if (ok)
      start(now);
    return;
  }
  if (screen == Screen::Utilities) {
    if (nav) {
      cursor = (cursor + nav + 6) % 6;
      pending = Sound::Move;
    }
    if (back)
      transition(Screen::Launcher, now);
    else if (ok) {
      const Screen next[] = {Screen::Settings, Screen::Scores,  Screen::System,
                             Screen::Updates,  Screen::History, Screen::Launcher};
      transition(next[cursor], now);
    }
    return;
  }
  if (screen == Screen::Updates) {
    if (nav)
      cursor = (cursor + nav + 5) % 5;
    if (back) {
      transition(Screen::Utilities, now);
      return;
    }
    if (ok) {
      const ConsoleAction actions[] = {ConsoleAction::CheckUpdate, ConsoleAction::InstallUpdate,
                                       ConsoleAction::SetupNetwork, ConsoleAction::Rollback,
                                       ConsoleAction::None};
      action = actions[cursor];
      if (cursor == 4)
        transition(Screen::Utilities, now);
    }
    return;
  }
  if (screen == Screen::History) {
    if (nav)
      cursor = clamp(cursor + nav, 0, 39);
    if (back || ok)
      transition(Screen::Utilities, now);
    return;
  }
  if (screen == Screen::Settings) {
    if (nav) {
      cursor = (cursor + nav + 9) % 9;
      pending = Sound::Move;
    }
    if (back) {
      transition(Screen::Utilities, now);
      return;
    }
    if (ok) {
      switch (cursor) {
      case 0:
        settings.sound ^= 1;
        break;
      case 1:
        settings.difficulty = (settings.difficulty + 1) % DifficultyCount;
        break;
      case 2:
        settings.effects ^= 1;
        break;
      case 3:
        settings.layout = (settings.layout + 1) % 3;
        break;
      case 4:
        settings.attract ^= 1;
        break;
      case 5:
        settings.invert ^= 1;
        break;
      case 6:
        settings.fps = settings.fps == 30 ? 60 : 30;
        break;
      case 7:
        transition(Screen::System, now);
        return;
      default:
        transition(Screen::Utilities, now);
        return;
      }
      dirty = true;
      pending = Sound::Select;
    }
    return;
  }
  if (screen == Screen::Scores) {
    constexpr int pages = (GameCount + 3) / 4 + 1;
    if (nav)
      cursor = (cursor + nav + pages) % pages;
    if (back || ok)
      transition(Screen::Utilities, now);
    return;
  }
  if (screen == Screen::System && (back || ok))
    transition(Screen::Utilities, now);
}
void Console::header(Canvas &c, const char *title, uint16_t color) const {
  c.rect(0, 0, W, 25, visual::Ink);
  c.centered(8, title, color);
  c.line(8, 24, W - 9, 24, visual::Edge);
  c.line(8, 24, 39, 24, color);
}
void Console::icon(Canvas &c, int index, int x, int y) const {
  uint16_t color = registry()[unsigned(index)].accent;
  visual::thumbnail(c, index, x, y, color, settings.effects ? visualTime / 33 : 0);
}
void Console::render(Canvas &c) const {
  const uint32_t frame = visualTime / 33;
  if (screen != Screen::Playing && screen != Screen::Attract)
    visual::backdrop(c, frame, settings.effects);
  if (screen == Screen::Boot) {
    visual::alien(c, 76, 31, Green, settings.effects ? frame / 18 : 0, 2);
    c.centered(66, "GAME", White, 3);
    c.centered(95, "ARCADE", Cyan, 3);
    char tagline[32];
    std::snprintf(tagline, sizeof(tagline), "%u GAMES. ONE CABINET.", unsigned(GameCount));
    c.centered(134, tagline, visual::Muted);
    c.rect(32, 156, 112, 3, visual::Edge);
    c.rect(32, 156, int(std::min(uint32_t(112), elapsed(visualTime, entered) * 112 / 1100)), 3,
           Cyan);
    c.centered(190, "A TO SKIP", White);
    return;
  }
  if (screen == Screen::Launcher) {
    char title[28];
    std::snprintf(title, sizeof(title), "ARCADE / %02u GAMES", unsigned(GameCount));
    header(c, title);
    int page = selection / 5;
    for (int row = 0; row < 5; row++) {
      int id = page * 5 + row;
      if (id >= int(GameCount))
        break;
      int y = 32 + row * 28;
      bool active = id == selection;
      visual::card(c, 4, y - 2, 168, 26, registry()[id].accent, active);
      icon(c, id, 9, y);
      c.text(40, y + 2, registry()[id].title, active ? White : visual::Muted);
      c.text(40, y + 13, "BEST", visual::Muted);
      c.number(70, y + 13, settings.best[id], active ? registry()[id].accent : visual::Muted);
      if (settings.favorites & (uint64_t(1) << id))
        visual::favorite(c, 162, y + 16, Yellow);
    }
    c.centered(176, registry()[selection].description, registry()[selection].accent);
    for (int i = 0; i < int((GameCount + 4) / 5); ++i)
      c.rect(50 + i * 8, 188, i == page ? 6 : 3, 2, i == page ? Cyan : visual::Edge);
    c.centered(197, "UP/DN PICK L/R PAGE", visual::Muted);
    c.centered(210, "A PLAY B FAV MENU MORE", White);
    return;
  }
  if (screen == Screen::Instructions) {
    const auto &i = registry()[selection];
    header(c, i.title, i.accent);
    icon(c, selection, 12, 32);
    c.text(46, 36, "HOW TO PLAY", i.accent);
    char mode[28];
    std::snprintf(mode, sizeof(mode), "%s / 12 STAGES", difficultyName(settings.difficulty));
    c.text(46, 47, mode, visual::Muted);
    visual::card(c, 4, 62, 168, 122, i.accent, false);
    for (int row = 0; row < 6; row++) {
      c.rect(10, 71 + row * 18, 2, 7, i.accent);
      c.text(17, 71 + row * 18, i.instructions[row], White);
    }
    c.centered(190, "MENU/BACK: PAUSE", visual::Muted);
    c.centered(204, fromPause ? "A: BACK TO PAUSE" : "A: START  B: BACK", Yellow);
    return;
  }
  if (screen == Screen::Playing || screen == Screen::Attract) {
    visual::stars(c, settings.effects ? frame : 0);
    if (current)
      current->render(c);
    c.rect(0, 0, W, Top, Navy);
    c.line(0, Top - 1, W - 1, Top - 1, visual::Edge);
    c.text(3, 6, "S", Cyan);
    c.number(13, 6, current ? current->score() : 0, White);
    c.text(98, 6, "L", Red);
    c.number(109, 6, current ? uint32_t(std::max(0, current->lives())) : 0, White);
    c.number(143, 6, current ? current->stage() : 1, Yellow);
    c.text(157, 6, "/12", Grey);
    if (screen == Screen::Attract) {
      c.rect(18, 103, 140, 21, Navy);
      c.centered(110, "PRESS ANY BUTTON", Yellow);
    }
    return;
  }
  if (screen == Screen::Paused) {
    header(c, "PAUSED", Yellow);
    const char *rows[] = {"RESUME", "RESTART", "CONTROLS", "EXIT TO LAUNCHER"};
    c.centered(36, registry()[selection].title, visual::Muted);
    for (int i = 0; i < 4; i++) {
      visual::card(c, 9, 54 + i * 32, 158, 26, Yellow, cursor == i);
      c.text(20, 63 + i * 32, rows[i], cursor == i ? Yellow : White);
    }
    c.centered(205, "A SELECT  B RESUME", visual::Muted);
    return;
  }
  if (screen == Screen::Over) {
    bool won = current && current->campaignComplete();
    header(c, won ? "CAMPAIGN COMPLETE" : "GAME OVER", won ? Green : Red);
    visual::card(c, 9, 65, 158, 57, won ? Green : Red, true);
    visual::card(c, 9, 128, 158, 48, Cyan, false);
    visual::favorite(c, 24, 47, won ? Yellow : Red);
    visual::favorite(c, 151, 47, won ? Yellow : Red);
    c.centered(49, registry()[selection].title, Cyan);
    c.centered(78, "SCORE", Grey);
    char text[16];
    std::snprintf(text, sizeof(text), "%lu", (unsigned long)(current ? current->score() : 0));
    c.centered(96, text, Yellow, 2);
    c.centered(134, "PERSONAL BEST", Grey);
    std::snprintf(text, sizeof(text), "%lu", (unsigned long)settings.best[selection]);
    c.centered(151, text, Green, 2);
    c.centered(195, "A REPLAY  B LAUNCHER", White);
    return;
  }
  if (screen == Screen::Utilities) {
    header(c, "SYSTEM MENU");
    const char *rows[] = {"SETTINGS",        "HIGH SCORES",   "SYSTEM INFO",
                          "WI-FI / UPDATES", "FRAME HISTORY", "BACK TO GAMES"};
    for (int i = 0; i < 6; i++) {
      int y = 33 + i * 27;
      visual::card(c, 5, y, 166, 24, Cyan, cursor == i);
      visual::thumbnail(c,
                        i == 0   ? 9
                        : i == 1 ? 21
                        : i == 3 ? 1
                        : i == 4 ? 19
                                 : 0,
                        10, y + 1, cursor == i ? Cyan : visual::Muted, 0);
      c.text(42, y + 8, rows[i], cursor == i ? Cyan : White);
    }
    c.centered(208, "A SELECT  B GAMES", visual::Muted);
    return;
  }
  if (screen == Screen::Updates) {
    header(c, "WI-FI / OTA");
    const char *rows[] = {"CHECK GITHUB RELEASE", "INSTALL VERIFIED UPDATE",
                          "WI-FI / REPOSITORY SETUP", "BOOT PREVIOUS FIRMWARE", "BACK"};
    for (int i = 0; i < 5; i++) {
      visual::card(c, 4, 30 + i * 21, 168, 19, Cyan, cursor == i);
      c.text(9, 36 + i * 21, rows[i], cursor == i ? Cyan : White);
    }
    return;
  }
  if (screen == Screen::History) {
    header(c, "LAST RUN FRAME HISTORY");
    visual::card(c, 9, 68, 158, 80, Cyan, false);
    visual::thumbnail(c, 19, 75, 78, Cyan, 0);
    c.centered(110, "NO FRAMES YET", White);
    c.centered(129, "PLAY A GAME TO RECORD", visual::Muted);
    c.centered(207, "L/R SCRUB  A BACK", White);
    return;
  }
  if (screen == Screen::Settings) {
    header(c, "SETTINGS");
    char rows[9][28];
    std::snprintf(rows[0], 28, "SOUND: %s", settings.sound ? "ON" : "OFF");
    std::snprintf(rows[1], 28, "DIFFICULTY: %s", difficultyName(settings.difficulty));
    std::snprintf(rows[2], 28, "EFFECTS: %s", settings.effects ? "ON" : "REDUCED");
    std::snprintf(rows[3], 28, "SNAKE BOARD: %u", unsigned(settings.layout) + 1);
    std::snprintf(rows[4], 28, "ATTRACT: %s", settings.attract ? "ON" : "OFF");
    std::snprintf(rows[5], 28, "INVERT: %s", settings.invert ? "ON" : "OFF");
    std::snprintf(rows[6], 28, "FPS REQUEST: %u", unsigned(settings.fps));
    std::snprintf(rows[7], 28, "SYSTEM INFORMATION");
    std::snprintf(rows[8], 28, "BACK");
    for (int i = 0; i < 9; i++) {
      visual::card(c, 5, 31 + i * 18, 166, 16, Cyan, cursor == i);
      c.text(12, 36 + i * 18, rows[i], cursor == i ? Cyan : White);
    }
    c.centered(208, cursor < 7 ? "A CHANGE  B BACK" : "A OPEN  B BACK", visual::Muted);
    return;
  }
  if (screen == Screen::Scores) {
    bool leaders = cursor == int((GameCount + 3) / 4);
    header(c, leaders ? "SKY TOP FIVE" : "HIGH SCORES");
    if (leaders) {
      for (unsigned i = 0; i < 5; i++) {
        visual::card(c, 7, 36 + int(i) * 30, 162, 25, Yellow, i == 0);
        c.number(17, 45 + i * 30, i + 1, Cyan);
        c.number(46, 45 + i * 30, settings.skyLeaders[i], Yellow);
      }
    } else
      for (unsigned row = 0; row < 4; row++) {
        unsigned id = cursor * 4 + row;
        if (id >= GameCount)
          break;
        int y = 34 + int(row) * 39;
        visual::card(c, 5, y, 166, 34, registry()[id].accent, false);
        icon(c, int(id), 10, y + 5);
        c.text(42, y + 6, registry()[id].title, White);
        c.number(42, y + 20, settings.best[id], registry()[id].accent);
      }
    c.centered(205, "L/R PAGE  A BACK", Grey);
    return;
  }
  if (screen == Screen::System) {
    header(c, "SYSTEM INFORMATION");
    visual::card(c, 2, 29, 172, 64, Cyan, false);
    visual::card(c, 2, 98, 172, 100, Cyan, false);
    c.text(6, 35, "CLASSIC ESP32-WROVER", White);
    c.text(6, 51, "ILI9225 176 X 220", White);
    c.text(6, 67, "SPI 20 MHZ / 8 BUTTONS", Grey);
    c.text(6, 83, "A/B OTA + BOOT ROLLBACK", Green);
    c.centered(205, "A OR B: BACK", Grey);
  }
}
} // namespace arcade
