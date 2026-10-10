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
  selection = settings.lastGame;
  lastAction = now;
  transition(Screen::Boot, now);
}
bool Console::confirm(const InputFrame &f) const {
  return options.oneButton ? (f.longPress & Fire) != 0 : f.hit(Fire);
}
int Console::navigation(const InputFrame &f) const {
  if (options.oneButton)
    return (f.released & Fire) ? 1 : 0;
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
  recorded = false;
  if (settings.lastGame != selection) {
    settings.lastGame = uint8_t(selection);
    dirty = true;
  }
  transition(Screen::Playing, now);
}
void Console::record() {
  if (current && !recorded) {
    if (settings.record(unsigned(selection), current->score()))
      dirty = true;
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
  if (f.pressed || f.released || f.longPress || f.repeated)
    lastAction = now;
  if (screen == Screen::Boot) {
    if (elapsed(now, entered) > 1100 || f.pressed)
      transition(Screen::Launcher, now);
    return;
  }
  if (screen == Screen::Attract) {
    if (f.pressed || f.longPress || elapsed(now, entered) > 15000) {
      if (current) {
        current->cleanup();
        current = nullptr;
      }
      lastAction = now;
      transition(Screen::Launcher, now);
      return;
    }
    InputFrame demo;
    demo.held = Fire;
    if (current) {
      current->handle(demo);
      current->update();
      if (current->gameOver())
        current->start(
            GameOptions{settings.difficulty, 0, bool(settings.effects), true, false, now | 1});
    }
    return;
  }
  if (screen == Screen::Playing) {
    bool pause = f.hit(Back) || f.hit(Pause) || (options.oneButton && (f.longPress & Fire));
    if (pause) {
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
      pending = Sound::Over;
    }
    return;
  }
  int nav = navigation(f);
  bool ok = confirm(f);
  bool back = f.hit(Back) || f.hit(Special) || f.hit(Pause);
  if (screen == Screen::Launcher) {
    if (nav) {
      if (options.oneButton && selection == 7) {
        transition(Screen::Utilities, now);
        return;
      }
      selection = (selection + nav + 8) % 8;
      pending = Sound::Move;
    }
    if (ok) {
      fromPause = false;
      returnTo = Screen::Launcher;
      transition(Screen::Instructions, now);
      pending = Sound::Select;
    } else if (back)
      transition(Screen::Utilities, now);
    else if (options.oneButton && (f.longPress & Special))
      transition(Screen::Utilities, now);
    else if (settings.attract && elapsed(now, lastAction) > 20000) {
      current = &gameFor(GameId::Sly);
      GameOptions o = options;
      o.oneButton = true;
      o.seed = now | 1;
      current->start(o);
      transition(Screen::Attract, now);
    }
    return;
  }
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
    else if (nav) {
      leave(now);
    }
    return;
  }
  if (screen == Screen::Utilities) {
    if (nav) {
      cursor = (cursor + nav + 4) % 4;
      pending = Sound::Move;
    }
    if (back)
      transition(Screen::Launcher, now);
    else if (ok) {
      Screen next[] = {Screen::Settings, Screen::Scores, Screen::System, Screen::Launcher};
      transition(next[cursor], now);
    }
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
        settings.difficulty = uint8_t((settings.difficulty + 1) % 3);
        break;
      case 2:
        settings.effects ^= 1;
        break;
      case 3:
        settings.layout = uint8_t((settings.layout + 1) % 3);
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
      case 8:
        transition(Screen::Utilities, now);
        return;
      }
      dirty = true;
      pending = Sound::Select;
    }
    return;
  }
  if (screen == Screen::Scores) {
    if (nav) {
      cursor = (cursor + nav + 3) % 3;
      pending = Sound::Move;
    }
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
  uint16_t col = registry()[unsigned(index)].accent;
  c.rect(x, y, 40, 36, Navy);
  c.rect(x, y, 40, 36, col, false);
  switch (index) {
  case 0:
  case 2:
    c.rect(x + 10, y + 10, 20, 13, col);
    c.rect(x + 14, y + 14, 3, 3, Black);
    c.rect(x + 24, y + 14, 3, 3, Black);
    c.line(x + 10, y + 26, x + 15, y + 22, col);
    c.line(x + 30, y + 26, x + 25, y + 22, col);
    break;
  case 1:
    c.line(x + 20, y + 6, x + 20, y + 29, col);
    c.line(x + 5, y + 22, x + 34, y + 22, col);
    c.line(x + 12, y + 30, x + 28, y + 30, col);
    break;
  case 3:
    c.circle(x + 20, y + 18, 12, col, true);
    c.circle(x + 15, y + 13, 3, Black, true);
    break;
  case 4:
    c.rect(x + 6, y + 6, 28, 7, col);
    c.rect(x + 12, y + 28, 16, 3, col);
    c.circle(x + 23, y + 20, 2, White, true);
    break;
  case 5:
    c.rect(x + 8, y + 9, 24, 5, col);
    c.rect(x + 8, y + 9, 5, 16, col);
    c.rect(x + 8, y + 24, 19, 5, col);
    break;
  case 6:
    c.rect(x + 15, y + 7, 10, 24, col);
    c.rect(x + 17, y + 11, 6, 5, White);
    break;
  case 7:
    c.circle(x + 19, y + 18, 8, col, true);
    c.rect(x + 24, y + 18, 9, 3, Orange);
    c.pixel(x + 23, y + 15, Black);
    break;
  }
}
void Console::render(Canvas &c) const {
  if (screen == Screen::Boot) {
    c.centered(56, "SLY", Green, 3);
    c.centered(87, "ARCADE", Cyan, 3);
    c.centered(128, "EIGHT WORLDS. ONE CABINET.", White);
    c.centered(173, "FIRE TO SKIP", Grey);
    return;
  }
  if (screen == Screen::Launcher) {
    header(c, "SLY ARCADE");
    const auto &info = registry()[unsigned(selection)];
    for (int i = 0; i < 8; i++) {
      int x = 5 + (i % 4) * 43, y = 34 + (i / 4) * 43;
      icon(c, i, x, y);
      if (i == selection)
        c.rect(x - 1, y - 1, 42, 38, White, false);
    }
    c.centered(133, info.title, info.accent);
    c.centered(149, info.description, White);
    c.text(16, 171, "BEST", Grey);
    c.number(51, 171, settings.best[unsigned(selection)], Yellow);
    c.centered(192, options.oneButton ? "TAP NEXT / HOLD START" : "L/R SELECT  FIRE START", Grey);
    c.centered(205, options.oneButton ? "UTILITIES AFTER GAME 8" : "WEAPON: SYSTEM MENU", Grey);
    return;
  }
  if (screen == Screen::Instructions) {
    const auto &info = registry()[unsigned(selection)];
    header(c, info.title, info.accent);
    icon(c, selection, 68, 35);
    for (unsigned i = 0; i < 6; i++)
      c.text(5, 83 + int(i) * 17, info.instructions[i], White);
    if (options.oneButton)
      c.centered(188, selection < 5 ? "TAP: REVERSE MOVEMENT" : "TAP FIRE TO CONTROL", Yellow);
    c.centered(206,
               options.oneButton ? "HOLD FIRE TO CONTINUE"
               : fromPause       ? "FIRE: BACK TO PAUSE"
                                 : "FIRE START / WEAPON BACK",
               Grey);
    return;
  }
  if (screen == Screen::Playing || screen == Screen::Attract) {
    if (current) {
      current->render(c);
      c.rect(0, 0, W, Top, Navy);
      c.text(2, 3, "S", Cyan);
      c.number(11, 3, current->score());
      c.text(83, 3, "L", Green);
      c.number(93, 3, unsigned(std::max(0, current->lives())));
      c.text(118, 3, "W", Pink);
      c.number(128, 3, current->stage());
    }
    if (screen == Screen::Attract) {
      c.rect(24, 103, 128, 20, Navy);
      c.centered(109, "DEMO - PRESS ANY KEY", Yellow);
    }
    return;
  }
  if (screen == Screen::Paused) {
    header(c, "PAUSED", Yellow);
    const char *items[] = {"RESUME", "RESTART", "CONTROLS", "EXIT TO LAUNCHER"};
    for (int i = 0; i < 4; i++) {
      if (i == cursor)
        c.rect(12, 56 + i * 29, 152, 22, Navy);
      c.centered(64 + i * 29, items[i], i == cursor ? Yellow : White);
    }
    c.centered(201, options.oneButton ? "TAP NEXT / HOLD OK" : "L/R SELECT  FIRE OK", Grey);
    return;
  }
  if (screen == Screen::Over) {
    header(c, "GAME OVER", Red);
    c.centered(51, registry()[unsigned(selection)].title, Cyan);
    c.centered(82, "SCORE", Grey);
    char s[16];
    std::snprintf(s, sizeof(s), "%lu", static_cast<unsigned long>(current ? current->score() : 0));
    c.centered(99, s, Yellow, 2);
    c.centered(134, "PERSONAL BEST", Grey);
    std::snprintf(s, sizeof(s), "%lu",
                  static_cast<unsigned long>(settings.best[unsigned(selection)]));
    c.centered(152, s, Green, 2);
    c.centered(192, options.oneButton ? "HOLD REPLAY / TAP EXIT" : "FIRE: INSTANT REPLAY", White);
    c.centered(207, "WEAPON: LAUNCHER", Grey);
    return;
  }
  if (screen == Screen::Utilities) {
    header(c, "SYSTEM MENU");
    const char *rows[] = {"SETTINGS", "HIGH SCORES", "SYSTEM INFO", "BACK TO GAMES"};
    for (int i = 0; i < 4; i++)
      c.centered(60 + i * 30, rows[i], cursor == i ? Yellow : White);
    c.centered(204, options.oneButton ? "TAP NEXT / HOLD OK" : "L/R SELECT  FIRE OK", Grey);
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
    std::snprintf(rows[7], 28, "SYSTEM INFO");
    std::snprintf(rows[8], 28, "BACK");
    for (int i = 0; i < 9; i++) {
      if (cursor == i)
        c.rect(3, 35 + i * 18, 170, 15, Navy);
      c.text(7, 39 + i * 18, rows[i], cursor == i ? Yellow : White);
    }
    c.centered(208, "FIRE CHANGE / WEAPON BACK", Grey);
    return;
  }
  if (screen == Screen::Scores) {
    header(c, cursor == 2 ? "SLY TOP FIVE" : "HIGH SCORES");
    if (cursor == 2) {
      for (unsigned i = 0; i < 5; i++) {
        c.number(9, 48 + int(i) * 25, i + 1, Cyan);
        c.number(43, 48 + int(i) * 25, settings.slyLeaders[i], Yellow);
      }
    } else
      for (unsigned i = unsigned(cursor) * 4; i < unsigned(cursor) * 4 + 4; i++) {
        c.text(8, 47 + int(i % 4) * 36, registry()[i].title, Cyan);
        c.number(8, 60 + int(i % 4) * 36, settings.best[i], Yellow);
      }
    c.centered(200, "L/R PAGE - FIRE BACK", Grey);
    return;
  }
  if (screen == Screen::System) {
    header(c, "SYSTEM INFORMATION");
    c.text(6, 35, "CLASSIC ESP32-WROVER", White);
    c.text(6, 51, "ILI9225 176 X 220", White);
    c.text(6, 67, "SPI 20 MHZ / ROTATION 0", Grey);
    c.text(6, 83, options.bluetooth ? "BLE ON / WI-FI OFF" : "WI-FI / BLE OFF", Grey);
    c.centered(205, "FIRE OR WEAPON: BACK", Grey);
  }
}
} // namespace arcade
