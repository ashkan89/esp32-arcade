#pragma once
#include "Game.h"
#include "Library.h"
namespace arcade {
enum class Screen : uint8_t {
  Boot,
  Launcher,
  Instructions,
  Playing,
  Paused,
  Over,
  Settings,
  Scores,
  System,
  Utilities,
  Attract,
  Updates,
  SoundPreview,
  LibraryOptions,
  Reset,
  ResetConfirm
};
enum class ConsoleAction : uint8_t { None, CheckUpdate, InstallUpdate, SetupNetwork, Rollback };
class Console {
  Settings &settings;
  GameOptions options;
  Game *current = nullptr;
  Screen screen = Screen::Boot, returnTo = Screen::Launcher;
  uint32_t entered = 0, lastAction = 0, visualTime = 0;
  int selection = 0, cursor = 0;
  bool suppressNeeded = true, dirty = false, fromPause = false, recorded = false;
  Sound pending = Sound::None;
  ConsoleAction action = ConsoleAction::None;
  bool newRun = false;
  bool legacyScores = false;
  Random runSeeds;
  Library library;
  bool resetScores = false;
  void rebuildLibrary();
  uint32_t nextSeed() {
    return runSeeds.next();
  }
  void transition(Screen s, uint32_t now);
  void start(uint32_t now);
  void leave(uint32_t now);
  void record();
  void icon(Canvas &c, int index, int x, int y) const;
  void header(Canvas &c, const char *title, uint16_t color = Cyan) const;
  bool confirm(const InputFrame &f) const;
  int navigation(const InputFrame &f) const;

public:
  explicit Console(Settings &s) : settings(s) {}
  void begin(GameOptions o, uint32_t now);
  void update(const InputFrame &f, uint32_t now);
  void render(Canvas &c) const;
  Screen state() const {
    return screen;
  }
  int selected() const {
    return selection;
  }
  Game *active() const {
    return current;
  }
  ConsoleAction takeAction() {
    auto result = action;
    action = ConsoleAction::None;
    return result;
  }
  int previewTrack() const {
    return cursor;
  }
  bool consumeNewRun() {
    bool value = newRun;
    newRun = false;
    return value;
  }
  bool consumeSuppression() {
    bool b = suppressNeeded;
    suppressNeeded = false;
    return b;
  }
  bool consumeDirty() {
    bool b = dirty;
    dirty = false;
    return b;
  }
  Sound takeSound() {
    Sound s = pending;
    pending = Sound::None;
    return s;
  }
};
} // namespace arcade
