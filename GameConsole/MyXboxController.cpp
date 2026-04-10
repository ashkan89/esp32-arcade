#include "MyXboxController.h"

XboxSeriesXControllerESP32_asukiaaa::Core
  xboxController("40:8e:2c:3e:b4:f0");

void MyXboxController::begin() {
  xboxController.begin();
  xboxController.onLoop();
}

bool MyXboxController::isConnected() {
  xboxController.onLoop();
  return xboxController.isConnected();
}

void MyXboxController::disconnect() {
  // xboxController.onLoop();
}

uint8_t MyXboxController::getBattery() {
  xboxController.onLoop();
  if (xboxController.isConnected()) {
    if (xboxController.isWaitingForFirstNotification()) {
      return 0;
    } else {
      return xboxController.battery;
    }
  }

  return 0;
}

XboxControllerNotificationParser MyXboxController::getButtons() {
  xboxController.onLoop();
  if (xboxController.isConnected()) {
    if (xboxController.isWaitingForFirstNotification()) {
      return XboxControllerNotificationParser();
    } else {
      return xboxController.xboxNotif;
    }
  }

  return XboxControllerNotificationParser();
}

void MyXboxController::vibrate() {
  xboxController.onLoop();
  if (xboxController.isConnected()) {
    if (xboxController.isWaitingForFirstNotification()) {
      return;
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
    }
  }
}