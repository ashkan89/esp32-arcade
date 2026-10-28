#include "arcade/Hardware.h"
#include "arcade/DirtySpan.h"
#include <cstring>
#include <esp32/spiram.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_spi_flash.h>
#include <esp_system.h>
#include <functional>
#include <nvs.h>
namespace arcade {
bool Display::begin() {
  tft.init();
  tft.setRotation(board::Rotation);
  tft.setSwapBytes(true);
  tft.fillScreen(Black);
  if (tft.width() != W || tft.height() != H) {
    error("DIMENSION MISMATCH");
    return false;
  }
  bool sixteen = !ARCADE_WROOM;
  if (ARCADE_WROOM) {
    constexpr uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    if (heap_caps_get_free_size(caps) >= W * H * 2 + 128 * 1024)
      memory = heap_caps_malloc(W * H * 2, caps);
    sixteen = memory != nullptr;
    if (!memory)
      memory = heap_caps_malloc(W * H, caps);
  } else if (psramFound())
    memory = heap_caps_malloc(W * H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  // This SPI path is synchronous, not DMA. Internal staging also keeps future DMA conversion safe.
  strip =
      static_cast<uint16_t *>(heap_caps_malloc(W * 8 * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!memory || !strip) {
    if (memory)
      heap_caps_free(memory);
    if (strip)
      heap_caps_free(strip);
    memory = nullptr;
    strip = nullptr;
    error("DISPLAY ALLOCATION FAILED");
    return false;
  }
  canvas.attach(memory, sixteen);
  canvas.clear();
  size_t bytes = W * H * (sixteen ? 2 : 1);
  if (!ARCADE_WROOM && psramFound())
    previous = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  Serial.println(previous ? "Renderer: exact dirty spans, merged by transfer cost"
                          : "Renderer: CRC dirty strips; periodic full refresh");
  uint32_t total = 0;
  for (int i = 0; i < 3; i++) {
    uint32_t begin = micros();
    present(true);
    total += elapsed(micros(), begin);
  }
  fullFrameUs = total / 3;
  total = 0;
  for (int i = 0; i < 3; i++) {
    uint32_t start = micros();
    for (int y = 0; y < 8; y++)
      for (int x = 0; x < 16; x++)
        strip[y * 16 + x] = canvas.pixel(x, y);
    tft.pushImage(0, 0, 16, 8, strip);
    total += elapsed(micros(), start);
  }
  tileUs = std::max(1u, total / 3);
  // Sparse gameplay can sustain 60 even when a full-screen transfer cannot.
  maxFps = 60;
  Serial.printf("display RGB%u full frame=%lu us, maximum request=%u FPS\n", sixteen ? 16 : 8,
                static_cast<unsigned long>(fullFrameUs), maxFps);
  Serial.printf("Tile transfer=%lu us; runtime pacing checks sustained load\n",
                (unsigned long)tileUs);
  invalidate();
  return true;
}
bool Display::compact(bool enabled) {
  if (!ARCADE_WROOM || !memory || canvas.sixteen() != enabled)
    return true;
  constexpr uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  size_t bytes = W * H * (enabled ? 1 : 2);
  // Keep the old framebuffer valid if growth fails. Changes occur only at
  // network screen edges or explicit diagnostics, never in game frame loops.
  if (!enabled && heap_caps_get_free_size(caps) + W * H < bytes + 128 * 1024)
    return false;
  void *replacement = heap_caps_realloc(memory, bytes, caps);
  if (!replacement) {
    Serial.println("Display memory transition deferred; current framebuffer retained");
    return false;
  }
  memory = replacement;
  canvas.attach(memory, !enabled);
  canvas.clear();
  invalidate();
  Serial.printf("Display mode RGB%u internalHeap=%lu\n", enabled ? 8 : 16,
                (unsigned long)heap_caps_get_free_size(caps));
  return true;
}
void Display::error(const char *message) {
  Serial.println(message);
  tft.fillScreen(Black);
  tft.setTextColor(Red, Black);
  tft.setTextSize(1);
  tft.setCursor(2, 40);
  tft.println(message);
}
// ILI9225's TFT_eSPI invert command is a placeholder: invert RGB in software instead.
void Display::invert(bool v) {
  if (v != inverted) {
    inverted = v;
    invalidate();
  }
}
void Display::present(bool force) {
  if (!strip || !canvas.ready())
    return;
  uint32_t started = micros();
  size_t bpp = canvas.sixteen() ? 2 : 1;
  auto *current = static_cast<const uint8_t *>(canvas.data());
  auto *old = static_cast<uint8_t *>(previous);
  uint32_t tileWireUs = uint32_t(uint64_t(16 * 8) * 16000000 / SPI_FREQUENCY);
  uint32_t windowUs = tileUs > tileWireUs ? tileUs - tileWireUs : 1;
  // CRC fallback is for boards without snapshot memory. Bound collision lifetime.
  if (!previous && ++presents % 120 == 0)
    force = true;
  for (int y = 0, index = 0; y < H; y += 8, ++index) {
    int rows = std::min(8, H - y);
    DirtyPlan plan;
    plan.count = 1;
    plan.spans[0] = {0, W};
    uint32_t hash = 0;
    if (previous && !force && !invalid) {
      if (canvas.sixteen())
        plan = dirtyPlan(reinterpret_cast<const uint16_t *>(current) + y * W,
                         reinterpret_cast<const uint16_t *>(old) + y * W, rows, windowUs,
                         SPI_FREQUENCY);
      else
        plan = dirtyPlan(current + y * W, old + y * W, rows, windowUs, SPI_FREQUENCY);
      if (!plan.count)
        continue;
    } else if (!previous) {
      // Hash native pixels before expansion; unchanged bands need no RGB conversion.
      hash = checksum(current + y * W * bpp, size_t(W * rows) * bpp);
      if (!force && !invalid && hash == hashes[unsigned(index)])
        continue;
    }
    for (unsigned i = 0; i < plan.count; ++i) {
      const auto &span = plan.spans[i];
      int width = span.right - span.left;
      canvas.copyRect(span.left, y, width, rows, strip, inverted);
      tft.pushImage(span.left, y, width, rows, strip);
      if (service)
        service();
    }
    if (previous)
      std::memcpy(old + y * W * bpp, current + y * W * bpp, size_t(W * rows) * bpp);
    else
      hashes[unsigned(index)] = hash;
  }
  invalid = false;
  lastTransferUs = elapsed(micros(), started);
}

void Audio::begin() {
  ready = ledcSetup(0, 1000, 8) > 0;
  if (ready) {
    ledcAttachPin(board::Buzzer, 0);
    ledcWrite(0, 0);
  } else
    Serial.println("Audio LEDC initialization failed");
}
void Audio::stop() {
  sequencer.stop(millis());
  pitch = 0;
  if (ready)
    ledcWrite(0, 0);
}
void Audio::play(Sound s, uint32_t now) {
  sequencer.play(s, now);
}
void Audio::update(uint32_t now) {
  uint32_t start = micros();
  uint16_t hz = sequencer.update(now);
  if (ready && hz != pitch) {
    pitch = hz;
    // Arduino 2.0.17 ledcWriteTone uses 10 bits and supplies approximately 50% duty.
    if (hz)
      ledcWriteTone(0, hz);
    else
      ledcWrite(0, 0);
  }
  maxCostUs = std::max(maxCostUs, elapsed(micros(), start));
}
void Storage::begin() {
  ready = prefs.begin("skyarcade", false);
  if (!ready) {
    Serial.println("NVS unavailable: settings are volatile");
    return;
  }
  SaveBytes bytes{};
  size_t len = prefs.isKey("state") ? prefs.getBytesLength("state") : 0;
  if ((len == SaveSize || len == PreviousSaveSize || len == 304 || len == 72) &&
      prefs.getBytes("state", bytes.data(), len) == len &&
      decodeStored(bytes.data(), len, settings)) {
    Serial.println("Settings and records loaded");
    if (len != SaveSize)
      mark(millis());
    else
      save.remember(bytes);
    return;
  }
  // Read the historical namespace without changing it, so older firmware can still boot.
  const char historicalNamespace[] = {0x73, 0x6c, 0x79, 0x61, 0x72, 0x63, 0x61, 0x64, 0x65, 0};
  Preferences previous;
  nvs_handle_t historicalHandle;
  bool historicalExists = nvs_open(historicalNamespace, NVS_READONLY, &historicalHandle) == ESP_OK;
  if (historicalExists)
    nvs_close(historicalHandle);
  if (historicalExists && previous.begin(historicalNamespace, true)) {
    len = previous.isKey("state") ? previous.getBytesLength("state") : 0;
    bool migrated = (len == SaveSize || len == PreviousSaveSize || len == 304 || len == 72) &&
                    previous.getBytes("state", bytes.data(), len) == len &&
                    decodeStored(bytes.data(), len, settings);
    previous.end();
    if (migrated) {
      Serial.println("Historical settings and records migrated");
      mark(millis());
      return;
    }
  }
  Serial.println("New or invalid settings: using defaults");
  // Defaults remain volatile until a real edit or record needs saving.
  save.remember(encode(settings));
}
void Storage::update(uint32_t now, bool safe) {
  if (!ready || !save.due(now, safe))
    return;
  SaveBytes bytes = encode(settings);
  auto result = save.update(bytes, now, safe, [this](const SaveBytes &data) {
    return prefs.putBytes("state", data.data(), data.size()) == data.size();
  });
  if (result == SaveResult::Saved)
    Serial.println("Settings and records saved");
  else if (result == SaveResult::Failed)
    Serial.println("NVS save failed; retry pending");
}
void beginButtons() {
  for (int pin : {board::Left, board::Right, board::Up, board::Down, board::Fire, board::Weapon,
                  board::Pause, board::Back})
    pinMode(pin, INPUT_PULLUP);
}
uint16_t readButtons() {
  const int pins[] = {board::Left, board::Right, board::Fire, board::Weapon,
                      board::Back, board::Up,    board::Down, board::Pause};
  uint16_t result = 0;
  for (int i = 0; i < 8; i++)
    if (digitalRead(pins[i]) == LOW)
      result |= uint16_t(1u << i);
  return result;
}
void Diagnostics::begin() {
  uint32_t actualFlash = 0;
  if (esp_flash_get_size(nullptr, &actualFlash) != ESP_OK)
    actualFlash = 0;
  chipFlash = actualFlash;
  psramPhysical = 0;
  if (psramFound()) {
    switch (esp_spiram_get_chip_size()) {
    case ESP_SPIRAM_SIZE_16MBITS:
      psramPhysical = 2u * 1024 * 1024;
      break;
    case ESP_SPIRAM_SIZE_32MBITS:
      psramPhysical = 4u * 1024 * 1024;
      break;
    case ESP_SPIRAM_SIZE_64MBITS:
      psramPhysical = 8u * 1024 * 1024;
      break;
    default:
      Serial.println("PSRAM chip capacity unknown");
      break;
    }
  }
  mappedPsram = uint32_t(heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
  Serial.printf("Chip=%s revision=%u cores=%u reset=%d\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getChipCores(), int(esp_reset_reason()));
  Serial.printf("Flash detected=%lu bytes configured=%lu bytes\n",
                static_cast<unsigned long>(chipFlash),
                static_cast<unsigned long>(ESP.getFlashChipSize()));
  Serial.printf("PSRAM initialized=%s physical=%lu mapped heap=%lu bytes\n",
                psramFound() ? "yes" : "no", static_cast<unsigned long>(psramPhysical),
                static_cast<unsigned long>(mappedPsram));
  Serial.printf("Heap internal free=%lu largest=%lu\n",
                static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
                static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
  if (chipFlash != board::ExpectedFlash)
    Serial.println("FLASH MISMATCH: use flash_id and correct partitions before further uploads");
  if (!ARCADE_WROOM && !psramFound())
    Serial.println("PSRAM unavailable: console requires the N16R8 memory hardware");
}
void Diagnostics::frame(uint32_t us) {
  frames++;
  windowUs += us;
  windowFrames++;
  windowMax = std::max(windowMax, us);
  if (windowUs >= 1000000) {
    achievedFps10 = uint32_t(uint64_t(windowFrames) * 10000000 / windowUs);
    recentMax = windowMax;
    windowUs = windowFrames = windowMax = 0;
  }
  frameMax = std::max(frameMax, us);
  unsigned bucket = us < 16667    ? 0
                    : us < 33334  ? 1
                    : us < 50000  ? 2
                    : us < 100000 ? 3
                    : us < 200000 ? 4
                                  : 5;
  frameBuckets[bucket]++;
}
void Diagnostics::report(uint32_t now, const Display &d, const StepClock &clock) {
  if (!ARCADE_PROFILE)
    return;
  if (elapsed(now, lastReport) < 30000)
    return;
  lastReport = now;
  Serial.printf("DISPLAY achieved=%lu.%lu FPS recentWorst=%lu us\n",
                (unsigned long)(achievedFps10 / 10), (unsigned long)(achievedFps10 % 10),
                (unsigned long)recentMax);
  Serial.printf("PROFILE frames=%lu updates=%lu updateMax=%lu renderMax=%lu frameMax=%lu "
                "transfer=%lu inputPollMax=%lu droppedSim=%lu us\n",
                (unsigned long)frames, (unsigned long)updates, (unsigned long)updateMax,
                (unsigned long)renderMax, (unsigned long)frameMax, (unsigned long)d.lastTransferUs,
                (unsigned long)maxInputGap, (unsigned long)clock.dropped);
  Serial.printf("FRAME buckets <16.7/33.4/50/100/200ms/>=200: %lu %lu %lu %lu %lu %lu\n",
                (unsigned long)frameBuckets[0], (unsigned long)frameBuckets[1],
                (unsigned long)frameBuckets[2], (unsigned long)frameBuckets[3],
                (unsigned long)frameBuckets[4], (unsigned long)frameBuckets[5]);
  Serial.printf("INPUT debounced edge to update max=%lu us (excludes debounce and physical display "
                "latency)\n",
                (unsigned long)pressToUpdateMax);
  Serial.printf("HEAP internal=%lu min=%lu PSRAM=%lu\n",
                (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned long)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
                (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
void Diagnostics::draw(Canvas &c, const Display &d, bool nvs) const {
  char s[28];
  std::snprintf(s, sizeof(s), "FLASH DETECTED %lu MB", (unsigned long)(chipFlash / (1024 * 1024)));
  c.text(6, 105, s, Yellow);
  std::snprintf(s, sizeof(s), "PSRAM PHYSICAL %lu MB",
                (unsigned long)(psramPhysical / (1024 * 1024)));
  c.text(6, 121, s, Yellow);
  std::snprintf(s, sizeof(s), "MAPPED HEAP %lu KB", (unsigned long)(mappedPsram / 1024));
  c.text(6, 137, s, White);
  std::snprintf(s, sizeof(s), "FULL FRAME %lu US", (unsigned long)d.fullFrameUs);
  c.text(6, 153, s, White);
  std::snprintf(s, sizeof(s), "FPS %lu.%lu CAP %u", (unsigned long)(achievedFps10 / 10),
                (unsigned long)(achievedFps10 % 10), unsigned(d.maxFps));
  c.text(6, 169, s, Cyan);
  c.text(6, 185, nvs ? "NVS READY" : "NVS FAILED - VOLATILE", nvs ? Green : Red);
}
} // namespace arcade
