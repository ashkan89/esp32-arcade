#pragma once
#include "BoardConfig.h"
#include "Core.h"
#if ARCADE_XBOX
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <atomic>
#endif
namespace arcade {
class Controller {
#if ARCADE_XBOX
  // Empty address discovers a paired Series controller; no personal MAC in firmware.
  XboxSeriesXControllerESP32_asukiaaa::Core xbox;
  std::atomic<uint32_t> packet{0}, stamp{0}, generation{0};
  NimBLEClient *subscribed = nullptr;
  uint32_t lastService = 0;
  void subscribe(NimBLEClient *client);
#endif
public:
  void begin();
  void service(bool mayConnect);
  uint16_t buttons();
  bool connected();
  void rumble();
};
} // namespace arcade
