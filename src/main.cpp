#include "arcade/Console.h"
#include "arcade/FrameHistory.h"
#include "arcade/Hardware.h"
#include "arcade/Network.h"
using namespace arcade;
namespace {
Display display;
Audio audio;
Storage storage;
Diagnostics diagnostics;
Input buttons;
Network network;
FrameHistory history;
Console console(storage.settings);
StepClock clockStep;
InputFrame pendingInput;
uint32_t lastRender = 0, lastPoll = 0, lastFrame = 0, edgeAt = 0;
bool ready = false, edgePending = false;
uint32_t bootAt = 0, lastCapture = 0;
bool bootConfirmed = false;
// Preserve the opaque marker expected by previously shipped OTA clients.
const char HistoricalUpdaterIdentity[] = "\x53\x4c\x59"
                                         "_ARCADE_N16R8|" ARCADE_VERSION;
} // namespace
// Arduino otherwise accepts a pending image before setup() has checked hardware.
extern "C" bool verifyRollbackLater() {
  return true;
}
void setup() {
  Serial.begin(115200);
  Serial.println("SKY_ARCADE_N16R8|" ARCADE_VERSION);
  // Keep this compatibility data in flash even with compiler/linker garbage collection.
  asm volatile("" : : "r"(HistoricalUpdaterIdentity) : "memory");
  diagnostics.begin();
  beginButtons();
  storage.begin();
  audio.begin();
  if (diagnostics.chipFlash != board::ExpectedFlash ||
      diagnostics.psramPhysical != 8u * 1024u * 1024u) {
    Serial.println("N16R8 flash / PSRAM check failed. Console halted.");
    Network::failBoot();
    return;
  }
  if (!display.begin()) {
    Network::failBoot();
    return;
  }
  history.begin();
  network.begin();
  GameOptions o;
  o.seed = esp_random();
  console.begin(o, millis());
  clockStep.reset(micros());
  lastRender = lastPoll = lastFrame = micros();
  ready = true;
  bootAt = millis();
}
void loop() {
  if (!ready) {
    delay(100);
    return;
  }
  network.service(console.state() == Screen::Updates);
  uint32_t now = millis(), us = micros();
  diagnostics.maxInputGap = std::max(diagnostics.maxInputGap, elapsed(us, lastPoll));
  lastPoll = us;
  auto f = buttons.poll(readButtons(), now);
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
    if (!network.busy())
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
    if (console.consumeNewRun())
      history.reset();
    if (console.state() != before)
      display.invalidate();
    if (console.consumeDirty())
      storage.mark(now);
    Sound effect = console.takeSound();
    audio.play(effect, now);
    auto action = console.takeAction();
    if (action != ConsoleAction::None) {
      audio.stop();
      buttons.suppress();
      pendingInput = {};
      network.request(action);
    }
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
    if (console.state() == Screen::Updates)
      network.draw(display.canvas);
    if (console.state() == Screen::History &&
        history.show(display.canvas, unsigned(console.historyOffset()))) {
      display.canvas.rect(0, 0, W, Top, Navy);
      display.canvas.centered(5, "LAST RUN FRAME HISTORY", Cyan);
      display.canvas.rect(0, 204, W, 16, Navy);
      display.canvas.centered(208, "L/R SCRUB  A BACK", White);
    }
    if (console.state() == Screen::Playing && elapsed(now, lastCapture) >= 250) {
      history.capture(display.canvas);
      lastCapture = now;
    }
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
  if (!bootConfirmed && elapsed(now, bootAt) >= 8000 && diagnostics.frames >= 20 &&
      diagnostics.updates >= 60) {
    if (storage.available() && Network::confirmBoot()) {
      bootConfirmed = true;
      Serial.println("Boot health checks passed; firmware confirmed");
    } else {
      Serial.println("Boot health / NVS confirmation failed");
      Network::failBoot();
      bootConfirmed = true;
    }
  }
  delay(1);
}
