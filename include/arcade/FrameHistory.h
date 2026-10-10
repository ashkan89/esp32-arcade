#pragma once
#include "Canvas.h"
#include <esp32/himem.h>
namespace arcade {
class FrameHistory {
  esp_himem_handle_t memory = nullptr;
  esp_himem_rangehandle_t range = nullptr;
  static constexpr size_t FrameBytes = W * H * 2;
  static constexpr size_t SlotBytes = 3 * ESP_HIMEM_BLKSZ;
  unsigned capacity = 0, next = 0, count = 0;
  bool transfer(void *pixels, unsigned slot, bool write);

public:
  bool begin();
  void reset() {
    next = count = 0;
  }
  void capture(Canvas &c);
  bool show(Canvas &c, unsigned age);
  unsigned frames() const {
    return count;
  }
};
} // namespace arcade
