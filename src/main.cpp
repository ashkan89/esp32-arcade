#include "arcade/Console.h"
#include "arcade/FrameHistory.h"
#include "arcade/Hardware.h"
#include "arcade/Network.h"
#include <bootloader_random.h>
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
void serviceAudio() {
  auto state = console.state();
  bool frozen = state == Screen::Paused || (state == Screen::Instructions && console.active());
  audio.sequencer.configure(storage.settings.sound && state != Screen::Attract,
                            storage.settings.music, storage.settings.sfx);
  uint8_t track = SilentTrack;
  if (state == Screen::SoundPreview)
    track = uint8_t(console.previewTrack());
  else if (state == Screen::Playing || frozen)
    track = uint8_t(console.selected());
  else if (state == Screen::Boot || state == Screen::Launcher || state == Screen::Settings)
    track = MenuTrack;
  audio.sequencer.setPaused(frozen, millis());
  audio.sequencer.select(track, millis());
  audio.update(millis());
}
// Preserve the opaque marker expected by previously shipped OTA clients.
#if !ARCADE_WROOM
const char HistoricalUpdaterIdentity[] = "\x53\x4c\x59"
                                         "_ARCADE_N16R8|" ARCADE_VERSION;
#endif
} // namespace
// Arduino otherwise accepts a pending image before setup() has checked hardware.
extern "C" bool verifyRollbackLater() {
  return true;
}
void setup() {
  Serial.begin(115200);
  Serial.println(ARCADE_IDENTITY ARCADE_VERSION);
  // Keep this compatibility data in flash even with compiler/linker garbage collection.
#if !ARCADE_WROOM
  asm volatile("" : : "r"(HistoricalUpdaterIdentity) : "memory");
#endif
  diagnostics.begin();
  beginButtons();
  storage.begin();
  audio.begin();
  if (diagnostics.chipFlash != board::ExpectedFlash ||
      diagnostics.psramPhysical != board::ExpectedPsram) {
    Serial.println("Target flash / PSRAM check failed. Console halted.");
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
  // Short startup entropy window; disabled before RF/ADC/I2S use.
  bootloader_random_enable();
  o.seed = esp_random();
  bootloader_random_disable();
#ifdef ARCADE_FIXED_SEED
  o.seed = ARCADE_FIXED_SEED;
#endif
  console.begin(o, millis());
  display.service = serviceAudio;
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
  serviceAudio();
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
      serviceAudio();
      display.invalidate();
      clockStep.reset(micros());
    }
    if (console.consumeNewRun()) {
      history.reset();
      audio.stop();
      if (console.active())
        Serial.printf("RUN game=%d difficulty=%u enhanced=%u seed=%lu\n", console.selected(),
                      console.active()->runDifficulty(), console.active()->enhancedMode(),
                      (unsigned long)console.active()->runSeed());
    }
    if (console.state() != before)
      display.invalidate();
    if (console.consumeDirty())
      storage.mark(now);
    Sound effect = console.takeSound();
    serviceAudio();
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
    serviceAudio();
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
    diagnostics.drawMax = std::max(diagnostics.drawMax, elapsed(micros(), started));
    uint32_t transferAt = micros();
    display.present();
    diagnostics.transferMax = std::max(diagnostics.transferMax, elapsed(micros(), transferAt));
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
  static uint32_t audioReport = 0;
  if (ARCADE_PROFILE && elapsed(now, audioReport) >= 30000) {
    audioReport = now;
    Serial.printf("AUDIO maxServiceGap=%lu ms maxCost=%lu us coalesced=%lu dropped=%lu\n",
                  (unsigned long)audio.sequencer.maxServiceGap, (unsigned long)audio.maxCostUs,
                  (unsigned long)audio.sequencer.coalesced, (unsigned long)audio.sequencer.dropped);
    Serial.printf("COST updateIncludingAI=%lu us draw=%lu us transfer=%lu us\n",
                  (unsigned long)diagnostics.updateMax, (unsigned long)diagnostics.drawMax,
                  (unsigned long)diagnostics.transferMax);
  }
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
