#include "arcade/Console.h"
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
      current = &gameFor(GameId::Sly);
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
        settings.difficulty = (settings.difficulty + 1) % 3;
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
  c.rect(0, 0, W, 25, Navy);
  c.centered(8, title, color);
  c.line(0, 24, W - 1, 24, color);
}
void Console::icon(Canvas &c, int index, int x, int y) const {
  uint16_t color = registry()[unsigned(index)].accent;
  c.rect(x, y, 28, 24, Navy);
  c.rect(x, y, 28, 24, color, false);
  c.number(x + 5, y + 8, unsigned(index + 1), color);
}
void Console::render(Canvas &c) const {
  if (screen == Screen::Boot) {
    c.centered(55, "GAME", Green, 3);
    c.centered(86, "ARCADE", Cyan, 3);
    char tagline[32];
    std::snprintf(tagline, sizeof(tagline), "%u GAMES. ONE CABINET.", unsigned(GameCount));
    c.centered(128, tagline, White);
    c.centered(174, "A TO SKIP", Grey);
    return;
  }
  if (screen == Screen::Launcher) {
    char title[28];
    std::snprintf(title, sizeof(title), "GAME ARCADE / %u", unsigned(GameCount));
    header(c, title);
    int page = selection / 5;
    for (int row = 0; row < 5; row++) {
      int id = page * 5 + row;
      if (id >= int(GameCount))
        break;
      int y = 33 + row * 27;
      bool active = id == selection;
      if (active)
        c.rect(2, y - 2, 172, 26, Navy);
      icon(c, id, 5, y);
      c.text(38, y + 3, registry()[id].title, active ? registry()[id].accent : White);
      c.text(38, y + 14, (settings.favorites & (uint64_t(1) << id)) ? "* FAVORITE" : "BEST", Grey);
      if (!(settings.favorites & (uint64_t(1) << id)))
        c.number(70, y + 14, settings.best[id], Grey);
    }
    c.centered(176, registry()[selection].description, registry()[selection].accent);
    c.centered(193, "UP/DN PICK  L/R PAGE", Grey);
    c.centered(207, "A PLAY B FAV MENU SYSTEM", White);
    return;
  }
  if (screen == Screen::Instructions) {
    const auto &i = registry()[selection];
    header(c, i.title, i.accent);
    for (int row = 0; row < 6; row++)
      c.text(6, 42 + row * 22, i.instructions[row], White);
    c.centered(184, "MENU/BACK: PAUSE MENU", Grey);
    c.centered(204, fromPause ? "A: BACK TO PAUSE" : "A: START  B: BACK", Yellow);
    return;
  }
  if (screen == Screen::Playing || screen == Screen::Attract) {
    if (current)
      current->render(c);
    c.rect(0, 0, W, Top, Navy);
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
    for (int i = 0; i < 4; i++)
      c.centered(62 + i * 30, rows[i], cursor == i ? Yellow : White);
    c.centered(205, "D-PAD PICK  A CONFIRM", Grey);
    return;
  }
  if (screen == Screen::Over) {
    bool won = current && current->campaignComplete();
    header(c, won ? "CAMPAIGN COMPLETE" : "GAME OVER", won ? Green : Red);
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
    const char *rows[] = {"SETTINGS",
                          "HIGH SCORES",
                          "SYSTEM INFORMATION",
                          "WI-FI / FIRMWARE UPDATE",
                          "LAST RUN FRAME HISTORY",
                          "BACK TO GAMES"};
    for (int i = 0; i < 6; i++)
      c.centered(44 + i * 26, rows[i], cursor == i ? Yellow : White);
    c.centered(208, "D-PAD PICK  A CONFIRM", Grey);
    return;
  }
  if (screen == Screen::Updates) {
    header(c, "WI-FI / OTA");
    const char *rows[] = {"CHECK GITHUB RELEASE", "INSTALL VERIFIED UPDATE",
                          "WI-FI / REPOSITORY SETUP", "BOOT PREVIOUS FIRMWARE", "BACK"};
    for (int i = 0; i < 5; i++)
      c.text(6, 35 + i * 20, rows[i], cursor == i ? Yellow : White);
    return;
  }
  if (screen == Screen::History) {
    header(c, "LAST RUN FRAME HISTORY");
    c.centered(94, "NO FRAME HISTORY YET", Grey);
    c.centered(207, "L/R SCRUB  A BACK", White);
    return;
  }
  if (screen == Screen::Settings) {
    header(c, "SETTINGS");
    char rows[9][28];
    std::snprintf(rows[0], 28, "SOUND: %s", settings.sound ? "ON" : "OFF");
    std::snprintf(rows[1], 28, "DIFFICULTY: %s",
                  settings.difficulty == 0   ? "EASY"
                  : settings.difficulty == 1 ? "NORMAL"
                                             : "HARD");
    std::snprintf(rows[2], 28, "EFFECTS: %s", settings.effects ? "ON" : "REDUCED");
    std::snprintf(rows[3], 28, "SNAKE BOARD: %u", unsigned(settings.layout) + 1);
    std::snprintf(rows[4], 28, "ATTRACT: %s", settings.attract ? "ON" : "OFF");
    std::snprintf(rows[5], 28, "INVERT: %s", settings.invert ? "ON" : "OFF");
    std::snprintf(rows[6], 28, "FPS REQUEST: %u", unsigned(settings.fps));
    std::snprintf(rows[7], 28, "SYSTEM INFORMATION");
    std::snprintf(rows[8], 28, "BACK");
    for (int i = 0; i < 9; i++)
      c.text(7, 37 + i * 18, rows[i], cursor == i ? Yellow : White);
    c.centered(208, "A CHANGE  B BACK", Grey);
    return;
  }
  if (screen == Screen::Scores) {
    bool leaders = cursor == int((GameCount + 3) / 4);
    header(c, leaders ? "SLY TOP FIVE" : "HIGH SCORES");
    if (leaders) {
      for (unsigned i = 0; i < 5; i++) {
        c.number(10, 48 + i * 25, i + 1, Cyan);
        c.number(43, 48 + i * 25, settings.slyLeaders[i], Yellow);
      }
    } else
      for (unsigned row = 0; row < 4; row++) {
        unsigned id = cursor * 4 + row;
        if (id >= GameCount)
          break;
        c.text(8, 43 + row * 37, registry()[id].title, Cyan);
        c.number(8, 58 + row * 37, settings.best[id], Yellow);
      }
    c.centered(205, "L/R PAGE  A BACK", Grey);
    return;
  }
  if (screen == Screen::System) {
    header(c, "SYSTEM INFORMATION");
    c.text(6, 35, "CLASSIC ESP32-WROVER", White);
    c.text(6, 51, "ILI9225 176 X 220", White);
    c.text(6, 67, "SPI 20 MHZ / 8 BUTTONS", Grey);
    c.text(6, 83, "A/B OTA + BOOT ROLLBACK", Green);
    c.centered(205, "A OR B: BACK", Grey);
  }
}
} // namespace arcade
