#include "arcade/Console.h"
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
Console console(storage.settings);
StepClock clockStep;
FramePacer framePacer;
FrameBudget frameBudget;
InputFrame pendingInput;
uint32_t lastPoll = 0, lastFrame = 0, edgeAt = 0;
uint8_t pacedFps = 30;
Screen budgetScreen = Screen::Boot;
bool ready = false, edgePending = false;
uint32_t bootAt = 0;
bool bootConfirmed = false;
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
void checkArtwork() {
  audio.stop();
  uint32_t before = ESP.getFreeHeap();
  Serial.printf("ART_CHECK BEGIN games=%u heap=%lu\n", unsigned(GameCount), (unsigned long)before);
  for (unsigned id = 0; id < GameCount; ++id) {
    Game &g = gameFor(GameId(id));
    GameOptions options;
    options.seed = 1000 + id;
    g.initialize();
    g.start(options);
    uint32_t updateMax = 0, drawMax = 0, transferMax = 0;
    for (unsigned tick = 0; tick < 180; ++tick) {
      if (readButtons() & Back) {
        g.cleanup();
        Serial.println("ART_CHECK ABORTED BY BACK");
        return;
      }
      InputFrame f;
      f.held = Fire | (tick / 45 % 2 ? Right : Left);
      f.pressed = tick % 24 == 0 ? Fire : 0;
      uint32_t at = micros();
      g.handle(f);
      g.update();
      updateMax = std::max(updateMax, elapsed(micros(), at));
      if (g.gameOver()) g.start(options);
      if (tick % 3 == 0) {
        at = micros();
        display.canvas.clear();
        g.render(display.canvas);
        if (!g.ownsHud()) {
          display.canvas.rect(0, 0, W, Top, Navy);
          display.canvas.centered(6, registry()[id].title, White);
        }
        drawMax = std::max(drawMax, elapsed(micros(), at));
        at = micros();
        display.present();
        transferMax = std::max(transferMax, elapsed(micros(), at));
      }
      delay(1); // Keep normal RTOS/watchdog service during the diagnostic.
    }
    g.cleanup();
    uint32_t hash = checksum(static_cast<const uint8_t *>(display.canvas.data()),
                             W * H * (display.canvas.sixteen() ? 2 : 1));
    Serial.printf("ART_CHECK game=%u heap=%lu update=%lu draw=%lu transfer=%lu crc=%08lx stack=%u\n",
                  id, (unsigned long)ESP.getFreeHeap(), (unsigned long)updateMax,
                  (unsigned long)drawMax, (unsigned long)transferMax, (unsigned long)hash,
                  unsigned(uxTaskGetStackHighWaterMark(nullptr)));
  }
  for (unsigned i = 0; i < 10; ++i) {
    bool compactOk = display.compact(true);
    bool small = !display.canvas.sixteen();
    bool fullOk = display.compact(false);
    Serial.printf("ART_CHECK FORMAT cycle=%u compact=%u full=%u rgb=%u heap=%lu\n", i,
                  compactOk && small, fullOk, display.canvas.sixteen() ? 16 : 8,
                  (unsigned long)ESP.getFreeHeap());
    if (!compactOk || !small || !fullOk || !display.canvas.sixteen()) {
      Serial.println("ART_CHECK FORMAT FAILED");
      return;
    }
    delay(1);
  }
  Serial.printf("ART_CHECK COMPLETE games=%u before=%lu after=%lu min=%lu\n", unsigned(GameCount),
                (unsigned long)before, (unsigned long)ESP.getFreeHeap(),
                (unsigned long)ESP.getMinFreeHeap());
}
#endif
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
// COM9 measured only 364 bytes of startup/render stack headroom at 8 KiB.
// Use the pinned Arduino core's weak hook; keep watchdog protection unchanged.
size_t getArduinoLoopTaskStackSize() {
  return 12 * 1024;
}
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
#if defined(ARCADE_ART_SOAK) && ARCADE_ART_SOAK
  checkArtwork();
#endif
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
  lastPoll = lastFrame = micros();
  pacedFps = std::min(storage.settings.fps, display.maxFps);
  clockStep.reset(lastFrame);
  framePacer.reset(lastFrame, pacedFps);
  ready = true;
  bootAt = millis();
}
void loop() {
  if (!ready) {
    delay(100);
    return;
  }
  network.service(console.state() == Screen::Updates);
  if (ARCADE_WROOM) {
    static bool wasCompact = false;
    bool wanted = console.state() == Screen::Updates || network.busy();
    if (wanted != wasCompact) {
      display.compact(wanted);
      wasCompact = wanted;
    }
  }
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
  // Sample both clocks together after input/audio service, never with a stale timestamp.
  us = micros();
  uint32_t workAt = us;
  bool renderDue = framePacer.advance(us);
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
      uint32_t resetAt = micros();
      clockStep.reset(resetAt);
      framePacer.reset(resetAt, pacedFps);
      renderDue = true;
    }
    if (console.consumeNewRun()) {
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
      if (display.compact(true))
        network.request(action);
      else
        network.memoryUnavailable();
    }
    diagnostics.updates++;
    diagnostics.updateMax = std::max(diagnostics.updateMax, elapsed(micros(), started));
    if (transitioned)
      break;
  }
  display.invert(storage.settings.invert);
  if (console.state() != budgetScreen) {
    budgetScreen = console.state();
    display.maxFps = 60;
    frameBudget.reset();
  }
  uint8_t fps = std::min(storage.settings.fps, display.maxFps);
  if (fps != pacedFps) {
    pacedFps = fps;
    uint32_t resetAt = micros();
    clockStep.reset(resetAt);
    framePacer.reset(resetAt, fps);
    frameBudget.reset();
  }
  if (renderDue) {
    uint32_t started = micros();
    display.canvas.clear();
    console.render(display.canvas);
    serviceAudio();
    if (console.state() == Screen::Updates)
      network.draw(display.canvas);
    if (console.state() == Screen::System)
      diagnostics.draw(display.canvas, display, storage.available());
    diagnostics.drawMax = std::max(diagnostics.drawMax, elapsed(micros(), started));
    uint32_t transferAt = micros();
    display.present();
    diagnostics.transferMax = std::max(diagnostics.transferMax, elapsed(micros(), transferAt));
    uint32_t renderTime = elapsed(micros(), started);
    diagnostics.renderMax = std::max(diagnostics.renderMax, renderTime);
    if (fps == 60 && frameBudget.observe(elapsed(micros(), workAt))) {
      display.maxFps = 30;
      Serial.println("Sustained frame workload exceeds 60 FPS; pacing this screen at 30 FPS");
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
    Serial.printf("PACING target=%u FPS missedDeadlines=%lu\n", pacedFps,
                  (unsigned long)framePacer.missed);
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
  // Keep yielding away from deadlines; a full RTOS tick at the deadline adds visible jitter.
  uint32_t slack = framePacer.remaining(micros());
  if (!slack || slack > 1500)
    delay(1);
  else if (slack)
    delayMicroseconds(std::min(slack, 200u));
}
