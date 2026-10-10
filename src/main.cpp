#include "arcade/Console.h"
#include "arcade/Controller.h"
#include "arcade/Hardware.h"
using namespace arcade;
namespace {
Display display;
Audio audio;
Storage storage;
Diagnostics diagnostics;
Input buttons;
Controller controller;
Console console(storage.settings);
StepClock clockStep;
InputFrame pendingInput;
uint32_t lastRender = 0, lastPoll = 0, lastFrame = 0, edgeAt = 0;
bool ready = false, edgePending = false;
} // namespace
void setup() {
  Serial.begin(115200);
  diagnostics.begin();
  beginButtons();
  storage.begin();
  audio.begin();
  if (diagnostics.chipFlash && diagnostics.chipFlash < 0x610000) {
    Serial.println("Physical flash too small for selected partition table. Console halted.");
    return;
  }
  if (!display.begin())
    return;
  controller.begin();
  GameOptions o;
  o.expanded = ARCADE_EXPANDED;
  o.oneButton = ARCADE_ONE_BUTTON;
  o.bluetooth = ARCADE_XBOX;
  o.seed = esp_random();
  console.begin(o, millis());
  clockStep.reset(micros());
  lastRender = lastPoll = lastFrame = micros();
  ready = true;
}
void loop() {
  if (!ready) {
    delay(100);
    return;
  }
  controller.service(console.state() != Screen::Playing && console.state() != Screen::Attract);
  uint32_t now = millis(), us = micros();
  diagnostics.maxInputGap = std::max(diagnostics.maxInputGap, elapsed(us, lastPoll));
  lastPoll = us;
  auto f = buttons.poll(uint16_t(readButtons() | controller.buttons()), now);
  pendingInput.held = f.held;
  pendingInput.pressed |= f.pressed;
  pendingInput.released |= f.released;
  pendingInput.repeated |= f.repeated;
  pendingInput.longPress |= f.longPress;
  if (f.pressed && !edgePending) {
    edgeAt = micros();
    edgePending = true;
  }
  audio.setEnabled(storage.settings.sound);
  audio.setMusic(console.state() == Screen::Boot || console.state() == Screen::Launcher);
  audio.update(now);
  unsigned steps = clockStep.advance(us);
  for (unsigned i = 0; i < steps; i++) {
    uint32_t started = micros();
    Screen before = console.state();
    console.update(pendingInput, now);
    pendingInput = {};
    if (edgePending) {
      diagnostics.pressToUpdateMax =
          std::max(diagnostics.pressToUpdateMax, elapsed(started, edgeAt));
      edgePending = false;
    }
    // Keep held state for bounded catch-up while consuming edges once.
    pendingInput.held = f.held;
    bool transitioned = console.consumeSuppression();
    if (transitioned) {
      buttons.suppress();
      pendingInput = {};
      f = {};
      audio.stop();
      display.invalidate();
      clockStep.reset(micros());
    }
    if (console.state() != before)
      display.invalidate();
    if (console.consumeDirty())
      storage.mark(now);
    Sound effect = console.takeSound();
    audio.play(effect, now);
    if (effect == Sound::Select)
      controller.rumble();
    diagnostics.updates++;
    diagnostics.updateMax = std::max(diagnostics.updateMax, elapsed(micros(), started));
    if (transitioned)
      break;
  }
  display.invert(storage.settings.invert);
  uint8_t fps = std::min(storage.settings.fps, display.maxFps);
  uint32_t interval = 1000000u / fps;
  us = micros();
  if (elapsed(us, lastRender) >= interval) {
    lastRender += interval;
    if (elapsed(us, lastRender) >= interval)
      lastRender = us;
    uint32_t started = micros();
    display.canvas.clear();
    console.render(display.canvas);
    if (console.state() == Screen::System)
      diagnostics.draw(display.canvas, display, storage.available());
    display.present();
    uint32_t renderTime = elapsed(micros(), started);
    diagnostics.renderMax = std::max(diagnostics.renderMax, renderTime);
    if (fps == 60 && renderTime > 16667) {
      display.maxFps = 30;
      Serial.println("Renderer exceeded 60 FPS budget; capped at 30 FPS");
    }
    uint32_t end = micros();
    diagnostics.frame(elapsed(end, lastFrame));
    lastFrame = end;
  }
  bool safe = console.state() != Screen::Playing && console.state() != Screen::Attract;
  storage.update(now, safe);
  diagnostics.report(now, display, clockStep);
  delay(1);
}
