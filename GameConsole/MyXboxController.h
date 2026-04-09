#pragma once
#include <Arduino.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

class MyXboxController {
public:
  void begin();
  bool isConnected();
  XboxControllerNotificationParser getButtons();
  void vibrate();
  uint8_t getBattery();

private:
  // XboxSeriesXControllerESP32_asukiaaa::Core xboxController;
  // XboxSeriesXControllerESP32_asukiaaa::Core
  //   xboxController("40:8e:2c:3e:b4:f0");
  //   uint8_t buzzerPin = 255;
  //   bool active = false;
  //   bool enabled = true;
  //   unsigned long toneEnd = 0;
  //   void play(uint16_t f, uint16_t d, uint8_t v);
};