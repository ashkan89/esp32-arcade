#pragma once
// Migration build uses the original physical buttons. BLE remains opt-in in the console.
struct XboxControllerNotificationParser { int btnDirRight=0,btnDirLeft=0,btnDirUp=0,btnDirDown=0,btnA=0,btnB=0,btnXbox=0,btnX=0; };
class MyXboxController { public: void begin() {} bool isConnected() { return false; } void disconnect() {} int getBattery(){return 0;} void vibrate() {} XboxControllerNotificationParser getButtons(){return {};} };
