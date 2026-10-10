#include "arcade/Controller.h"
namespace arcade {
#if ARCADE_XBOX
void Controller::subscribe(NimBLEClient *client) {
  auto *service = client->getService(NimBLEUUID("1812"));
  if (!service)
    return;
  for (auto *characteristic : service->getCharacteristics())
    if (characteristic->getUUID() == NimBLEUUID("2a4d") && characteristic->canNotify()) {
      characteristic->subscribe(
          true, [this](NimBLERemoteCharacteristic *, uint8_t *data, size_t length, bool) {
            XboxControllerNotificationParser b;
            if (b.update(data, length) != 0)
              return;
            uint16_t bits = 0;
            if (b.btnDirLeft)
              bits |= Left;
            if (b.btnDirRight)
              bits |= Right;
            if (b.btnDirUp)
              bits |= Up;
            if (b.btnDirDown)
              bits |= Down;
            if (b.btnA)
              bits |= Fire;
            if (b.btnB)
              bits |= Back;
            if (b.btnX)
              bits |= Special;
            if (b.btnXbox || b.btnStart)
              bits |= Pause;
            // Seqlock publishes the mask and full rollover-safe timestamp as one snapshot.
            generation.fetch_add(1, std::memory_order_acq_rel);
            packet.store(bits, std::memory_order_relaxed);
            stamp.store(millis(), std::memory_order_relaxed);
            generation.fetch_add(1, std::memory_order_release);
          });
    }
  subscribed = client;
}
#endif
void Controller::begin() {
#if ARCADE_XBOX
  xbox.begin();
#endif
}
void Controller::service(bool mayConnect) {
#if ARCADE_XBOX
  // Library discovery/connect can block. Only run it in non-playing menus.
  if (mayConnect && elapsed(millis(), lastService) >= 250) {
    lastService = millis();
    xbox.onLoop();
    auto *client = XboxSeriesXControllerESP32_asukiaaa::pConnectedClient;
    if (client && client != subscribed)
      subscribe(client);
    else if (!client) {
      subscribed = nullptr;
      stamp.store(0, std::memory_order_release);
    }
  }
#else
  (void)mayConnect;
#endif
}
bool Controller::connected() {
#if ARCADE_XBOX
  auto v = stamp.load(std::memory_order_acquire);
  return v != 0 && elapsed(millis(), v) < 500;
#else
  return false;
#endif
}
uint16_t Controller::buttons() {
#if ARCADE_XBOX
  for (int attempt = 0; attempt < 3; attempt++) {
    uint32_t before = generation.load(std::memory_order_acquire);
    if (before & 1)
      continue;
    auto time = stamp.load(std::memory_order_relaxed);
    auto mask = packet.load(std::memory_order_relaxed);
    if (before == generation.load(std::memory_order_acquire))
      return time != 0 && elapsed(millis(), time) < 500 ? uint16_t(mask) : 0;
  }
  return 0;
#else
  return 0;
#endif
}
void Controller::rumble() {
#if ARCADE_XBOX
  if (connected()) {
    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase report;
    report.v.select.center = true;
    report.v.power.center = 30;
    report.v.timeActive = 10;
    xbox.writeHIDReport(report);
  }
#endif
}
} // namespace arcade
