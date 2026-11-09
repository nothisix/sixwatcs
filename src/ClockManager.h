#pragma once

#include <Arduino.h>
#include <ChronosESP32.h>

struct ActiveNotification {
  String app;
  String title;
  String message;
  String time;
  unsigned long receivedAt;
  bool active;
};

class ClockManager {
public:
  static void begin();
  static void update();

  // Time queries
  static bool isSynced();
  static bool isBleConnected();
  static void getTimeString(char* buf, size_t len);
  static void getDateString(char* buf, size_t len);
  static void getDayString(char* buf, size_t len);

  // Chronos App Smartwatch Features
  static uint8_t getPhoneBattery();
  static bool isPhoneCharging();
  static bool hasWeather();
  static int getWeatherTemp();
  static const char* getBleAddress();
  static const char* getBleName();

  // Notification features (WhatsApp, etc.)
  static bool hasActiveNotification();
  static void dismissNotification();
  static void onNotificationReceived(Notification notif);
  static void renderNotificationPopup();
  static uint8_t getUnreadNotifCount();
  static ActiveNotification& getLatestNotification();

  // Navigation features (Google Maps turn-by-turn)
  static bool isNavigationActive();
  static Navigation getNavigation();
  static void renderNavigationScreen();

  // Music Control (Chronos BLE)
  static void sendMusicControl(uint16_t command);

  // Render watch UI to OLED
  static void renderWatchFace();

  // Internal callbacks
  static void onConnectionChange(bool connected);
  static void onConfigChange(Config config, uint32_t a, uint32_t b);

private:
  static bool s_synced;
  static bool s_bleConnected;
  static unsigned long s_lastTimeSave;
  static uint8_t s_phoneBattery;
  static bool s_phoneCharging;
  static int s_weatherTemp;
  static bool s_hasWeather;
  static String s_bleMac;

  // Notification state
  static ActiveNotification s_currentNotif;
  static uint8_t s_unreadCount;

  // Navigation state
  static bool s_navActive;
};
