#include "MyXboxController.h"

XboxSeriesXControllerESP32_asukiaaa::Core
  xboxController("40:8e:2c:3e:b4:f0");

void MyXboxController::begin() {
  xboxController.begin();
}

bool MyXboxController::isConnected() {
  return xboxController.isConnected();
}

uint8_t MyXboxController::getBattery() {
  while (1) {
    xboxController.onLoop();
    if (xboxController.isConnected()) {
      if (xboxController.isWaitingForFirstNotification()) {
      } else {
        return xboxController.battery;
      }
    }
  }
}

XboxControllerNotificationParser MyXboxController::getButtons() {
  while (1) {
    xboxController.onLoop();
    if (xboxController.isConnected()) {
      if (xboxController.isWaitingForFirstNotification()) {
        continue;
      } else {
        return xboxController.xboxNotif;
      }
    }
  }
}

void MyXboxController::vibrate() {
  while (1) {
    xboxController.onLoop();
    if (xboxController.isConnected()) {
      if (xboxController.isWaitingForFirstNotification()) {
        continue;
      } else {
        XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase repo;

        // repo.v.select.shake = false;
        // repo.v.select.center = true;
        // repo.v.power.center = 50;
        // repo.v.timeActive = 20;
        // repo.v.timeSilent = 20;
        // repo.v.countRepeat = 1;
        // xboxController.writeHIDReport(repo);

        repo.v.select.center = true;
        repo.v.select.left = false;
        repo.v.select.right = false;
        repo.v.select.shake = false;
        repo.v.power.center = 30;
        repo.v.timeActive = 20;
        xboxController.writeHIDReport(repo);

        break;
      }
    }
  }
}