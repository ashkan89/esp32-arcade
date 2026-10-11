#include "arcade/FrameHistory.h"
#include <Arduino.h>
#include <cstring>
// scripts/defer_himem.py disables the SDK's premature global constructor.
// Arduino must initialize PSRAM before himem queries its physical size.
extern "C" void esp_himem_init();
namespace arcade {
#if ARCADE_WROOM
bool FrameHistory::begin() { return false; }
void FrameHistory::capture(Canvas &) {}
bool FrameHistory::show(Canvas &, unsigned) { return false; }
#else
bool FrameHistory::begin() {
  if (!psramFound())
    return false;
  esp_himem_init();
  capacity = std::min(size_t(40), esp_himem_get_free_size() / SlotBytes);
  if (!capacity || esp_himem_alloc(capacity * SlotBytes, &memory) != ESP_OK) {
    capacity = 0;
    return false;
  }
  if (esp_himem_alloc_map_range(ESP_HIMEM_BLKSZ, &range) != ESP_OK) {
    esp_himem_free(memory);
    memory = nullptr;
    capacity = 0;
    return false;
  }
  Serial.printf("Banked PSRAM history: %u frames, %u KB; mapped window 32 KB\n", capacity,
                unsigned(capacity * SlotBytes / 1024));
  return true;
}
bool FrameHistory::transfer(void *pixels, unsigned slot, bool write) {
  auto *bytes = static_cast<uint8_t *>(pixels);
  for (size_t offset = 0; offset < FrameBytes; offset += ESP_HIMEM_BLKSZ) {
    void *window = nullptr;
    if (esp_himem_map(memory, range, slot * SlotBytes + offset, 0, ESP_HIMEM_BLKSZ, 0, &window) !=
        ESP_OK)
      return false;
    size_t n = std::min(size_t(ESP_HIMEM_BLKSZ), FrameBytes - offset);
    if (write)
      std::memcpy(window, bytes + offset, n);
    else
      std::memcpy(bytes + offset, window, n);
    if (esp_himem_unmap(range, window, ESP_HIMEM_BLKSZ) != ESP_OK)
      return false;
  }
  return true;
}
void FrameHistory::capture(Canvas &c) {
  if (capacity && c.sixteen() && transfer(c.data(), next, true)) {
    next = (next + 1) % capacity;
    count = std::min(capacity, count + 1);
  }
}
bool FrameHistory::show(Canvas &c, unsigned age) {
  if (!count || !c.sixteen())
    return false;
  age = std::min(age, count - 1);
  return transfer(c.data(), (next + capacity - 1 - age) % capacity, false);
}
#endif
} // namespace arcade
