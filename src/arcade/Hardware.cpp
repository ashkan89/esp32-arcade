#include "arcade/Hardware.h"
#include <cstring>
#include <esp32/spiram.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_spi_flash.h>
#include <esp_system.h>
#include <functional>
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
  bool sixteen = true;
  if (psramFound())
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
  if (psramFound())
    previous = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  Serial.println(previous ? "Renderer: exact dirty tiles with full-strip fallback"
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
  tileCutoff = clamp(fullFrameUs / tileUs, 1u, 308u);
  // Leave 25% plus 3 ms CPU headroom. This is a boot measurement, not a promised FPS.
  maxFps = fullFrameUs * 5 / 4 + 3000 < 16667 ? 60 : 30;
  Serial.printf("display RGB%u full frame=%lu us, maximum request=%u FPS\n", sixteen ? 16 : 8,
                static_cast<unsigned long>(fullFrameUs), maxFps);
  Serial.printf("Tile transfer=%lu us; dense-frame crossover=%u tiles\n", (unsigned long)tileUs,
                tileCutoff);
  invalidate();
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
  // Exact comparison avoids hash collisions when a backing snapshot is available.
  if (previous && !force && !invalid) {
    std::array<bool, 308> changed{};
    unsigned count = 0;
    size_t bpp = canvas.sixteen() ? 2 : 1;
    auto *current = static_cast<uint8_t *>(canvas.data());
    auto *old = static_cast<uint8_t *>(previous);
    for (int y = 0, t = 0; y < H; y += 8)
      for (int x = 0; x < W; x += 16, t++) {
        int rows = std::min(8, H - y);
        bool different = false;
        for (int j = 0; j < rows && !different; j++)
          different = std::memcmp(current + ((y + j) * W + x) * bpp, old + ((y + j) * W + x) * bpp,
                                  16 * bpp) != 0;
        changed[unsigned(t)] = different;
        count += different;
      }
    // Sparse frames use tiles. Dense frames amortize controller window commands in strips.
    if (count < tileCutoff) {
      for (int y = 0, t = 0; y < H; y += 8)
        for (int x = 0; x < W; x += 16, t++)
          if (changed[unsigned(t)]) {
            int rows = std::min(8, H - y);
            for (int j = 0; j < rows; j++)
              for (int i = 0; i < 16; i++)
                strip[j * 16 + i] = uint16_t(canvas.pixel(x + i, y + j) ^ (inverted ? 0xffff : 0));
            tft.pushImage(x, y, 16, rows, strip);
            for (int j = 0; j < rows; j++)
              std::memcpy(old + ((y + j) * W + x) * bpp, current + ((y + j) * W + x) * bpp,
                          16 * bpp);
          }
      lastTransferUs = elapsed(micros(), started);
      return;
    }
    force = true;
  }
  // Without a snapshot, refresh periodically to bound the effect of any CRC collision.
  if (!previous && ++presents % 120 == 0)
    force = true;
  for (int y = 0, index = 0; y < H; y += 8, index++) {
    int rows = std::min(8, H - y);
    for (int j = 0; j < rows; j++)
      for (int x = 0; x < W; x++)
        strip[j * W + x] = uint16_t(canvas.pixel(x, y + j) ^ (inverted ? 0xffff : 0));
    uint32_t hash = checksum(reinterpret_cast<uint8_t *>(strip), size_t(W * rows * 2));
    if (force || invalid || hash != hashes[unsigned(index)]) {
      tft.pushImage(0, y, W, rows, strip);
      hashes[unsigned(index)] = hash;
    }
  }
  invalid = false;
  lastTransferUs = elapsed(micros(), started);
  if (previous)
    std::memcpy(previous, canvas.data(), size_t(W * H) * (canvas.sixteen() ? 2 : 1));
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
  length = index = priority = 0;
  active = false;
  if (ready)
    ledcWrite(0, 0);
}
void Audio::next(uint32_t now) {
  if (index >= length) {
    stop();
    return;
  }
  auto n = sequence[index++];
  deadline = now + n.ms;
  active = true;
  ledcWriteTone(0, n.hz);
  ledcWrite(0, n.hz ? 128 : 0); // passive transducer, fixed 50% duty, no pretend volume
}
void Audio::play(Sound s, uint32_t now) {
  if (!ready || !enabled || s == Sound::None)
    return;
  uint8_t p = uint8_t(s);
  if (active && p < priority)
    return;
  stop();
  priority = p;
  switch (s) {
  case Sound::Move:
    sequence[0] = {1200, 12};
    length = 1;
    break;
  case Sound::Shot:
    sequence[0] = {1800, 16};
    length = 1;
    break;
  case Sound::Hit:
    sequence[0] = {900, 20};
    length = 1;
    break;
  case Sound::Explosion:
    sequence[0] = {1200, 18};
    sequence[1] = {800, 18};
    sequence[2] = {400, 28};
    length = 3;
    break;
  case Sound::Power:
  case Sound::Wave:
  case Sound::Select:
    sequence[0] = {900, 20};
    sequence[1] = {1200, 20};
    sequence[2] = {1800, 35};
    length = 3;
    break;
  case Sound::Over:
    sequence[0] = {700, 70};
    sequence[1] = {500, 80};
    sequence[2] = {300, 100};
    length = 3;
    break;
  default:
    break;
  }
  next(now);
}
void Audio::update(uint32_t now) {
  if (active && reached(now, deadline))
    next(now);
  if (ready && enabled && music && !active && elapsed(now, musicTick) >= 180) {
    static constexpr uint16_t notes[] = {262, 330, 392, 330, 294, 349, 440, 349};
    musicTick = now;
    sequence[0] = {notes[musicStep++ % 8], 35};
    length = 1;
    index = priority = 0;
    next(now);
  }
}
void Storage::begin() {
  ready = prefs.begin("slyarcade", false);
  if (!ready) {
    Serial.println("NVS unavailable: settings are volatile");
    return;
  }
  SaveBytes bytes{};
  size_t len = prefs.getBytesLength("state");
  if ((len == SaveSize || len == 72) && prefs.getBytes("state", bytes.data(), len) == len &&
      decodeStored(bytes.data(), len, settings)) {
    Serial.println("Settings and records loaded");
    if (len != SaveSize)
      mark(millis());
    return;
  }
  Serial.println("New or invalid settings: using defaults");
  mark(millis());
}
void Storage::update(uint32_t now, bool safe) {
  if (!ready || !dirty || !safe || elapsed(now, changed) < 2000 || elapsed(now, lastAttempt) < 5000)
    return;
  lastAttempt = now;
  SaveBytes bytes = encode(settings);
  if (prefs.putBytes("state", bytes.data(), bytes.size()) == bytes.size()) {
    dirty = false;
    Serial.println("Settings and records saved");
  } else
    Serial.println("NVS save failed; retry pending");
}
void beginButtons() {
  for (int pin : {board::Left, board::Right, board::Up, board::Down, board::Fire, board::Weapon})
    pinMode(pin, INPUT_PULLUP);
  pinMode(board::Pause, INPUT);
  pinMode(board::Back, INPUT);
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
  if (!psramFound())
    Serial.println("PSRAM unavailable: console requires the N16R8 memory hardware");
}
void Diagnostics::frame(uint32_t us) {
  frames++;
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
  std::snprintf(s, sizeof(s), "FRAME CAP %u FPS", unsigned(d.maxFps));
  c.text(6, 169, s, Cyan);
  c.text(6, 185, nvs ? "NVS READY" : "NVS FAILED - VOLATILE", nvs ? Green : Red);
}
} // namespace arcade
