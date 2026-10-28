#pragma once
#include "Canvas.h"
#include "Console.h"
#include "Persistence.h"
#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <atomic>
namespace arcade {
class Network {
  Preferences prefs;
  WebServer server{80};
  DNSServer dns;
  String ssid, password, repository, firmwareUrl, releaseVersion;
  String persistedConfig;
  WriteCooldown configCooldown;
  uint8_t expectedHash[32]{};
  uint32_t expectedSize = 0, portalAt = 0, savedAt = 0;
  bool portal = false, savePending = false, storageReady = false;
  bool routesRegistered = false;
  char apPassword[13]{}, message[96] = "CONFIGURE WI-FI / REPOSITORY";
  portMUX_TYPE statusLock = portMUX_INITIALIZER_UNLOCKED;
  std::atomic<bool> working{false}, found{false};
  std::atomic<unsigned> progress{0};
  ConsoleAction job = ConsoleAction::None;
  void status(const char *text);
  void setupPortal();
  void closePortal(const char *reason);
  void work();
  bool connect();
  bool check();
  bool install();
  void rollback();
  static void task(void *self);

public:
  void begin();
  void service(bool menu);
  void request(ConsoleAction action);
  void memoryUnavailable() { status("LOW MEMORY - RETRY OR BACK"); }
  bool busy() const {
    return working.load();
  }
  void draw(Canvas &);
  static bool pendingBoot();
  static bool confirmBoot();
  static void failBoot();
};
} // namespace arcade
