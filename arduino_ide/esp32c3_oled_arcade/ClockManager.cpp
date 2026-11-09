#include "ClockManager.h"
#include "Display.h"
#include "Config.h"
#include "SettingsManager.h"
#include "Engine.h"
#include "MusicManager.h"
#include <Preferences.h>

static ChronosESP32 s_watch;
static Preferences s_clockPrefs;

bool ClockManager::s_synced = false;
bool ClockManager::s_bleConnected = false;
unsigned long ClockManager::s_lastTimeSave = 0;
uint8_t ClockManager::s_phoneBattery = 0;
bool ClockManager::s_phoneCharging = false;
int ClockManager::s_weatherTemp = 0;
bool ClockManager::s_hasWeather = false;
String ClockManager::s_bleMac = "";

ActiveNotification ClockManager::s_currentNotif = {"", "", "", "", 0, false};
uint8_t ClockManager::s_unreadCount = 0;
bool ClockManager::s_navActive = false;

static void onChronosConnection(bool state) {
  ClockManager::onConnectionChange(state);
}

static void onChronosConfig(Config config, uint32_t a, uint32_t b) {
  ClockManager::onConfigChange(config, a, b);
}

static void onChronosNotification(Notification notif) {
  ClockManager::onNotificationReceived(notif);
}

static void onChronosRawData(uint8_t *data, int length) {
  if (!data || length < 4) return;
  if (data[0] == 0xAB && length >= 5) {
    if (data[4] == 0xEF) {
      Serial.printf("[Chronos NAV PKT] sub=0x%02X, len=%d\n", data[5], length);
    } else if (data[4] == 0xEE) {
      Serial.printf("[Chronos NAV ICON PKT] sub=0x%02X, len=%d\n", data[5], length);
    } else if (data[4] != 0x90 && data[4] != 0x91 && data[4] != 0x92) {
      // General packet logging (skip noisy battery/sync heartbeats if needed)
      Serial.printf("[Chronos RAW] cmd=0x%02X, sub=0x%02X, len=%d\n", data[4], data[5], length);
    }
  }
}

void ClockManager::onNotificationReceived(Notification notif) {
  s_currentNotif.app = notif.app;
  s_currentNotif.title = notif.title;
  s_currentNotif.message = notif.message;
  s_currentNotif.time = notif.time;
  s_currentNotif.receivedAt = millis();
  s_currentNotif.active = true;
  s_unreadCount++;

  Serial.printf("[Chronos] Notification: [%s] %s: %s (%s)\n", 
                notif.app.c_str(), notif.title.c_str(), notif.message.c_str(), notif.time.c_str());
}

bool ClockManager::hasActiveNotification() {
  if (!s_currentNotif.active) return false;
  // Auto-dismiss after 8 seconds
  if (millis() - s_currentNotif.receivedAt > 8000) {
    s_currentNotif.active = false;
    return false;
  }
  return true;
}

void ClockManager::dismissNotification() {
  s_currentNotif.active = false;
}

uint8_t ClockManager::getUnreadNotifCount() {
  return s_unreadCount;
}

ActiveNotification& ClockManager::getLatestNotification() {
  return s_currentNotif;
}

bool ClockManager::isNavigationActive() {
  Navigation nav = s_watch.getNavigation();
  return nav.active && (nav.duration != "Disabled") && (nav.duration != "Inactive") && (nav.directions != "Start navigation on Google maps");
}

Navigation ClockManager::getNavigation() {
  return s_watch.getNavigation();
}

void ClockManager::onConnectionChange(bool connected) {
  s_bleConnected = connected;
  Serial.printf("[Chronos] BLE Connection status changed: %s\n", connected ? "CONNECTED" : "DISCONNECTED");
}

void ClockManager::onConfigChange(Config config, uint32_t a, uint32_t b) {
  if (config == CF_TIME || config == CF_SYNCED) {
    s_synced = true;
    Serial.printf("[Chronos] Time synchronized via Chronos App: %s %s\n",
                  s_watch.getTime("%H:%M:%S").c_str(),
                  s_watch.getTime("%d/%m/%Y").c_str());
    
    // Save to NVS
    time_t now = time(nullptr);
    s_clockPrefs.putUInt("saved_epoch", (uint32_t)now);
    s_clockPrefs.putUInt("saved_millis", (uint32_t)millis());
  } else if (config == CF_PBAT) {
    s_phoneCharging = (bool)a;
    s_phoneBattery = (uint8_t)b;
    Serial.printf("[Chronos] Phone Battery: %d%% (Charging: %d)\n", s_phoneBattery, s_phoneCharging);
  } else if (config == CF_WEATHER) {
    if (s_watch.getWeatherCount() > 0) {
      s_weatherTemp = s_watch.getWeatherAt(0).temp;
      s_hasWeather = true;
      Serial.printf("[Chronos] Weather received: %d C\n", s_weatherTemp);
    }
  } else if (config == CF_NAV_DATA) {
    Navigation nav = s_watch.getNavigation();
    bool isRealNav = (a == 1) && (nav.duration != "Disabled") && (nav.duration != "Inactive") && (nav.directions != "Start navigation on Google maps");
    s_navActive = isRealNav;
    Serial.printf("[Chronos] Navigation packet received: a=%u, title='%s', dur='%s', dist='%s'\n",
                  a, nav.title.c_str(), nav.duration.c_str(), nav.distance.c_str());
    if (isRealNav) {
      Serial.printf("[Chronos] Nav start: %s - %s (%s)\n", nav.title.c_str(), nav.directions.c_str(), nav.distance.c_str());
      Engine::enterNavigation();
    }
  } else if (config == CF_NAV_ICON) {
    Serial.printf("[Chronos] Nav icon received pos=%u, crc=0x%04X\n", a, b);
  } else if (config == CF_MUSIC) {
    MusicInfo info = s_watch.getMusicInfo();
    Serial.printf("[Chronos] Music packet: sub=%u, state=%u, title='%s', artist='%s', app='%s'\n",
                  a, b, info.title.c_str(), info.artist.c_str(), info.appName.c_str());
    MusicManager::onBleMusicReceived(info.title, info.artist, info.appName, (uint8_t)b);
  }
}

void ClockManager::sendMusicControl(uint16_t command) {
  s_watch.musicControl((Control)command);
}

void ClockManager::begin() {
  s_clockPrefs.begin("clock", false);
  uint32_t savedEpoch = s_clockPrefs.getUInt("saved_epoch", 0);
  uint32_t savedMillis = s_clockPrefs.getUInt("saved_millis", 0);

  if (savedEpoch > 1700000000) {
    unsigned long elapsedSec = (millis() - savedMillis) / 1000;
    time_t restored = savedEpoch + elapsedSec;
    s_watch.setTime(restored);
    s_synced = true;
    Serial.printf("[Clock] Restored time from NVS: epoch=%lu\n", (unsigned long)restored);
  } else {
    // Default fallback: 2026-01-01 12:00:00
    s_watch.setTime(0, 0, 12, 1, 1, 2026);
    s_synced = false;
  }

  // Initialize Chronos Smartwatch BLE Server
  Serial.println("[Chronos] Initializing Chronos ESP32 BLE Server...");
  s_watch.setName("Chronos ESP32");
  s_watch.setScreen(CF_ESP32_240x240);
  s_watch.set24Hour(true);
  s_watch.setBattery(100);
  s_watch.setConnectionCallback(onChronosConnection);
  s_watch.setConfigurationCallback(onChronosConfig);
  s_watch.setNotificationCallback(onChronosNotification);
  s_watch.setRawDataCallback(onChronosRawData);
  s_watch.begin();

  // Boost BLE Radio TX Power to maximum (+9 dBm) on ESP32-C3
  NimBLEDevice::setPower(9);

  // Retrieve MAC address
  s_bleMac = s_watch.getAddress();
  if (s_bleMac.length() == 0) {
    s_bleMac = NimBLEDevice::getAddress().toString().c_str();
  }

  Serial.println("\n==========================================");
  Serial.printf("[Chronos] BLE Name: %s\n", "Chronos ESP32");
  Serial.printf("[Chronos] BLE MAC : %s\n", s_bleMac.c_str());
  Serial.printf("[Chronos] Advertising Active: %d\n", NimBLEDevice::getAdvertising()->isAdvertising());
  Serial.println("[Chronos] Cara Konek via Aplikasi Chronos:");
  Serial.println("  1. Buka aplikasi Chronos di HP");
  Serial.println("  2. Pastikan GPS/Lokasi HP & Izin Bluetooth aktif");
  Serial.println("  3. Tab Watch -> Watches -> Pair New Device -> Search");
  Serial.println("  4. Pilih 'Chronos ESP32' ATAU pilih 'Manual' & masukkan MAC di atas!");
  Serial.println("==========================================\n");
}

void ClockManager::update() {
  s_watch.loop();
  bool conn = s_watch.isConnected();
  if (conn != s_bleConnected) {
    s_bleConnected = conn;
  }

  // Periodic check: restart advertising if not connected and advertising stopped
  static unsigned long lastAdvCheck = 0;
  if (!s_bleConnected && (millis() - lastAdvCheck >= 2000)) {
    lastAdvCheck = millis();
    if (!NimBLEDevice::getAdvertising()->isAdvertising()) {
      Serial.println("[Chronos] Advertising re-triggered...");
      NimBLEDevice::getAdvertising()->start();
    }
  }

  if (s_bleConnected) {
    uint8_t bat = s_watch.getPhoneBattery();
    if (bat > 0 && bat <= 100) {
      s_phoneBattery = bat;
      s_phoneCharging = s_watch.isPhoneCharging();
    }
    if (s_watch.getWeatherCount() > 0) {
      s_weatherTemp = s_watch.getWeatherAt(0).temp;
      s_hasWeather = true;
    }

    // Auto-detect navigation updates directly from library
    Navigation nav = s_watch.getNavigation();
    bool isRealNav = nav.active && (nav.duration != "Disabled") && (nav.duration != "Inactive") && (nav.directions != "Start navigation on Google maps");
    if (isRealNav && !s_navActive) {
      s_navActive = true;
      Serial.printf("[Chronos] Navigation ACTIVE -> Switching to HUD: %s - %s (%s)\n",
                    nav.title.c_str(), nav.directions.c_str(), nav.distance.c_str());
      Engine::enterNavigation();
    } else if (!isRealNav && s_navActive) {
      s_navActive = false;
      Serial.println("[Chronos] Navigation ended on phone.");
    }
  }

  // Periodic NVS save
  if (s_synced && (millis() - s_lastTimeSave >= 60000)) {
    s_lastTimeSave = millis();
    time_t now = time(nullptr);
    s_clockPrefs.putUInt("saved_epoch", (uint32_t)now);
    s_clockPrefs.putUInt("saved_millis", (uint32_t)millis());
  }
}

const char* ClockManager::getBleAddress() {
  return s_bleMac.c_str();
}

const char* ClockManager::getBleName() {
  return "Chronos ESP32";
}

bool ClockManager::isSynced() {
  return s_synced;
}

bool ClockManager::isBleConnected() {
  return s_bleConnected;
}

uint8_t ClockManager::getPhoneBattery() {
  return s_phoneBattery;
}

bool ClockManager::isPhoneCharging() {
  return s_phoneCharging;
}

bool ClockManager::hasWeather() {
  return s_hasWeather;
}

int ClockManager::getWeatherTemp() {
  return s_weatherTemp;
}

void ClockManager::getTimeString(char* buf, size_t len) {
  if (!buf || len == 0) return;
  String t = s_watch.getTime("%H:%M:%S");
  snprintf(buf, len, "%s", t.c_str());
}

void ClockManager::getDateString(char* buf, size_t len) {
  if (!buf || len == 0) return;
  static const char* months[] = {
    "JAN", "FEB", "MAR", "APR", "MEI", "JUN",
    "JUL", "AGU", "SEP", "OKT", "NOV", "DES"
  };
  int m = s_watch.getMonth(); // 0-11 in ESP32Time
  if (m < 0 || m > 11) m = 0;
  snprintf(buf, len, "%02d %s %04d", s_watch.getDay(), months[m], s_watch.getYear());
}

void ClockManager::getDayString(char* buf, size_t len) {
  if (!buf || len == 0) return;
  static const char* days[] = {
    "MINGGU", "SENIN", "SELASA", "RABU", "KAMIS", "JUMAT", "SABTU"
  };
  int w = s_watch.getDayofWeek(); // 0=Sunday
  if (w < 0 || w > 6) w = 0;
  snprintf(buf, len, "%s", days[w]);
}

void ClockManager::renderWatchFace() {
  u8g2.clearBuffer();

  uint8_t style = SettingsManager::getWatchStyle();

  char timeBuf[16];
  getTimeString(timeBuf, sizeof(timeBuf));

  char dayBuf[16];
  char dateBuf[20];
  getDayString(dayBuf, sizeof(dayBuf));
  getDateString(dateBuf, sizeof(dateBuf));

  if (style == 1) {
    // =========================================================================
    // MINIMAL WATCH FACE
    // =========================================================================
    u8g2.setFont(u8g2_font_logisoso20_tn);
    char hhmm[8];
    hhmm[0] = timeBuf[0];
    hhmm[1] = timeBuf[1];
    hhmm[2] = ((millis() / 500) % 2 == 0) ? ':' : ' ';
    hhmm[3] = timeBuf[3];
    hhmm[4] = timeBuf[4];
    hhmm[5] = '\0';
    int16_t timeW = u8g2.getStrWidth(hhmm);
    int16_t startX = (SCREEN_WIDTH - timeW - 20) / 2;
    u8g2.drawStr(startX, 36, hhmm);

    char ss[6];
    snprintf(ss, sizeof(ss), ":%c%c", timeBuf[6], timeBuf[7]);
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawStr(startX + timeW + 4, 30, ss);

    char fullDate[36];
    snprintf(fullDate, sizeof(fullDate), "%s, %s", dayBuf, dateBuf);
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(48, fullDate);

    // Indicator
    if (s_bleConnected) {
      u8g2.drawDisc(SCREEN_WIDTH - 6, 6, 2);
    } else {
      u8g2.drawCircle(SCREEN_WIDTH - 6, 6, 2);
      u8g2.setFont(u8g2_font_4x6_tf);
      if (((millis() / 2500) % 2) == 1 && s_bleMac.length() > 0) {
        char macLine[32];
        snprintf(macLine, sizeof(macLine), "MAC: %s", s_bleMac.c_str());
        Display::drawCenteredStr(58, macLine);
      } else {
        Display::drawCenteredStr(58, "Chronos ESP32");
      }
    }

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // RETRO CHRONOS WATCH FACE (DEFAULT)
  // =========================================================================
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  u8g2.drawHLine(2, 13, SCREEN_WIDTH - 4);
  u8g2.drawHLine(2, 51, SCREEN_WIDTH - 4);

  // Top Status Bar: Chronos App status & phone battery
  u8g2.setFont(u8g2_font_4x6_tf);
  if (s_bleConnected) {
    u8g2.drawStr(5, 9, "CHRONOS: CONNECTED");
    
    // Show phone battery or weather on top right
    if (s_phoneBattery > 0) {
      char batBuf[16];
      snprintf(batBuf, sizeof(batBuf), "%s%d%%", s_phoneCharging ? "CHG:" : "HP:", s_phoneBattery);
      Display::drawRightAlignedStr(SCREEN_WIDTH - 5, 9, batBuf);
    } else if (s_hasWeather) {
      char wBuf[16];
      snprintf(wBuf, sizeof(wBuf), "%d C", s_weatherTemp);
      Display::drawRightAlignedStr(SCREEN_WIDTH - 5, 9, wBuf);
    } else {
      Display::drawRightAlignedStr(SCREEN_WIDTH - 5, 9, "SYNC OK");
    }
  } else {
    bool blink = (millis() / 500) % 2;
    if (blink) {
      u8g2.drawDisc(6, 7, 2);
    } else {
      u8g2.drawCircle(6, 7, 2);
    }
    u8g2.drawStr(12, 9, "CHRONOS: SEARCHING");

    if (s_synced) {
      Display::drawRightAlignedStr(SCREEN_WIDTH - 5, 9, "SYNC OK");
    } else {
      Display::drawRightAlignedStr(SCREEN_WIDTH - 5, 9, "UNSYNCED");
    }
  }

  // Center: Big Bold Time Display
  u8g2.setFont(u8g2_font_logisoso20_tn);
  char hhmm[8];
  hhmm[0] = timeBuf[0];
  hhmm[1] = timeBuf[1];
  hhmm[2] = ((millis() / 500) % 2 == 0) ? ':' : ' ';
  hhmm[3] = timeBuf[3];
  hhmm[4] = timeBuf[4];
  hhmm[5] = '\0';

  char ss[6];
  snprintf(ss, sizeof(ss), ":%c%c", timeBuf[6], timeBuf[7]);

  int16_t timeW = u8g2.getStrWidth(hhmm);
  int16_t startX = 18;
  u8g2.drawStr(startX, 39, hhmm);

  // Seconds beside HH:MM
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(startX + timeW + 4, 32, ss);

  // Bottom Center: Day of Week & Date
  char fullDate[36];
  if (s_hasWeather && s_bleConnected) {
    snprintf(fullDate, sizeof(fullDate), "%s, %s [%d C]", dayBuf, dateBuf, s_weatherTemp);
  } else {
    snprintf(fullDate, sizeof(fullDate), "%s, %s", dayBuf, dateBuf);
  }
  u8g2.setFont(u8g2_font_5x7_tf);
  Display::drawCenteredStr(48, fullDate);

  // Footer: Subtle ticker / retro prompt
  u8g2.setFont(u8g2_font_4x6_tf);
  if (s_navActive && s_watch.getNavigation().active) {
    Display::drawCenteredStr(59, "NAV ACTIVE - OK: VIEW HUD");
  } else if (MusicManager::hasActiveTrack()) {
    char musicLine[36];
    String titleCut = MusicManager::getTitle();
    if (titleCut.length() > 14) titleCut = titleCut.substring(0, 14);
    snprintf(musicLine, sizeof(musicLine), "%s: %s - OK: HUD",
             MusicManager::isPlaying() ? "♫ PLAY" : "♫ PAUSE",
             titleCut.c_str());
    Display::drawCenteredStr(59, musicLine);
  } else if (!s_bleConnected) {
    if (((millis() / 2500) % 2) == 1 && s_bleMac.length() > 0) {
      char macPrompt[32];
      snprintf(macPrompt, sizeof(macPrompt), "MAC: %s", s_bleMac.c_str());
      Display::drawCenteredStr(59, macPrompt);
    } else {
      Display::drawCenteredStr(59, "DEV: Chronos ESP32");
    }
  } else {
    Display::drawCenteredStr(59, "PRESS ANY BUTTON TO PLAY");
  }

  u8g2.sendBuffer();
}

static void drawWrappedMessage(int16_t startX, int16_t startY, const char* text, uint8_t maxLines) {
  if (!text) return;
  int len = strlen(text);
  int offset = 0;
  for (uint8_t line = 0; line < maxLines && offset < len; line++) {
    char lineBuf[28];
    int charCount = 0;
    while (offset < len && text[offset] == ' ') offset++;
    while (offset < len && charCount < 22) {
      lineBuf[charCount++] = text[offset++];
    }
    lineBuf[charCount] = '\0';
    if (offset < len && line < maxLines - 1 && text[offset] != ' ') {
      int lastSpace = -1;
      for (int i = charCount - 1; i >= 0; i--) {
        if (lineBuf[i] == ' ') { lastSpace = i; break; }
      }
      if (lastSpace > 4) {
        offset -= (charCount - 1 - lastSpace);
        lineBuf[lastSpace] = '\0';
      }
    } else if (offset < len && line == maxLines - 1) {
      if (charCount >= 3) {
        lineBuf[charCount - 3] = '.';
        lineBuf[charCount - 2] = '.';
        lineBuf[charCount - 1] = '.';
      }
    }
    u8g2.drawStr(startX, startY + (line * 10), lineBuf);
  }
}

void ClockManager::renderNotificationPopup() {
  u8g2.clearBuffer();

  // Outer border with rounded corners
  u8g2.drawRFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 3);

  // Top header bar: White inverted box
  u8g2.drawBox(1, 1, SCREEN_WIDTH - 2, 13);
  u8g2.setDrawColor(0); // Black text on white box
  u8g2.setFont(u8g2_font_5x7_tf);

  char appTag[24];
  String appName = s_currentNotif.app;
  if (appName.length() == 0) appName = "NOTIF";
  appName.toUpperCase();
  snprintf(appTag, sizeof(appTag), "[%s]", appName.substring(0, 12).c_str());
  u8g2.drawStr(4, 10, appTag);

  if (s_currentNotif.time.length() > 0) {
    Display::drawRightAlignedStr(SCREEN_WIDTH - 4, 10, s_currentNotif.time.c_str());
  }

  // Restore normal white draw color
  u8g2.setDrawColor(1);

  // Sender / Contact title
  u8g2.setFont(u8g2_font_6x10_tf);
  String sender = s_currentNotif.title;
  if (sender.length() == 0) sender = "Pesan Baru";
  if (sender.length() > 18) sender = sender.substring(0, 18) + "...";
  u8g2.drawStr(5, 24, sender.c_str());

  // Message body with auto word-wrap
  u8g2.setFont(u8g2_font_5x7_tf);
  drawWrappedMessage(5, 35, s_currentNotif.message.c_str(), 2);

  // Bottom prompt bar
  u8g2.drawHLine(2, 53, SCREEN_WIDTH - 4);
  u8g2.setFont(u8g2_font_4x6_tf);
  Display::drawCenteredStr(61, "PRESS ANY BUTTON TO CLOSE");

  u8g2.sendBuffer();
}

void ClockManager::renderNavigationScreen() {
  u8g2.clearBuffer();

  Navigation nav = s_watch.getNavigation();

  // If navigation is inactive, disabled in Chronos settings, or waiting for Google Maps
  if (!nav.active || nav.duration == "Disabled" || nav.directions == "Start navigation on Google maps") {
    u8g2.drawRFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 2);
    u8g2.setFont(u8g2_font_6x10_tf);
    Display::drawCenteredStr(12, "GMAPS NAVIGATION");
    u8g2.drawHLine(4, 15, SCREEN_WIDTH - 8);

    u8g2.setFont(u8g2_font_5x7_tf);
    if (!s_bleConnected) {
      Display::drawCenteredStr(28, "STATUS: DISCONNECTED");
      Display::drawCenteredStr(40, "Hubungkan ke Chronos HP");
    } else if (nav.duration == "Disabled") {
      Display::drawCenteredStr(28, "STATUS: NAV DISABLED");
      Display::drawCenteredStr(39, "Chronos > ESP32 Mgr");
      Display::drawCenteredStr(49, "Nyalakan 'Navigation' ON");
    } else {
      Display::drawCenteredStr(26, "STATUS: WAITING GMAPS");
      Display::drawCenteredStr(37, "1. Buka Google Maps di HP");
      Display::drawCenteredStr(48, "2. Klik 'Mulai / Start'");
    }

    u8g2.setFont(u8g2_font_4x6_tf);
    Display::drawCenteredStr(60, "OK: KEMBALI KE MENU");
    u8g2.sendBuffer();
    return;
  }

  // Top Header Bar
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(2, 6, "GMAPS NAVIGATION");

  // Right-aligned clock
  char timeBuf[16];
  getTimeString(timeBuf, sizeof(timeBuf));
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 6, timeBuf);
  u8g2.drawHLine(0, 8, SCREEN_WIDTH);

  // Left Column: 48x48 Turn Icon Box
  u8g2.drawRFrame(0, 10, 50, 52, 2);

  if (nav.hasIcon) {
    for (int y = 0; y < 48; y++) {
      for (int x = 0; x < 48; x++) {
        int byte_index = (y * 48 + x) / 8;
        int bit_pos = 7 - (x % 8);
        bool px = (nav.icon[byte_index] >> bit_pos) & 0x01;
        if (px) {
          u8g2.drawPixel(1 + x, 12 + y);
        }
      }
    }
  } else {
    // Fallback Arrow graphic
    u8g2.setFont(u8g2_font_logisoso16_tf);
    Display::drawCenteredStr(34, "^");
    u8g2.setFont(u8g2_font_4x6_tf);
    Display::drawCenteredStr(48, "TURN");
  }

  // Right Column: Distance, Title, Street, Duration/ETA
  // Large bold distance: e.g. "250 m"
  u8g2.setFont(u8g2_font_7x14B_tf);
  const char* dist = (nav.distance.length() > 0) ? nav.distance.c_str() : "-- m";
  u8g2.drawStr(53, 23, dist);

  // Turn Title: e.g. "Turn right"
  u8g2.setFont(u8g2_font_5x7_tf);
  String title = nav.title;
  if (title.length() > 14) title = title.substring(0, 14);
  u8g2.drawStr(53, 34, title.c_str());

  // Street / Direction: e.g. "Jl. Sudirman"
  String dir = nav.directions;
  if (dir.length() > 14) dir = dir.substring(0, 14);
  u8g2.drawStr(53, 45, dir.c_str());

  // Remaining duration & ETA: e.g. "15 min | 14:35"
  u8g2.setFont(u8g2_font_4x6_tf);
  char etaLine[32];
  if (nav.duration.length() > 0 && nav.eta.length() > 0) {
    snprintf(etaLine, sizeof(etaLine), "%s | %s", nav.duration.c_str(), nav.eta.c_str());
  } else if (nav.duration.length() > 0) {
    snprintf(etaLine, sizeof(etaLine), "%s", nav.duration.c_str());
  } else {
    snprintf(etaLine, sizeof(etaLine), "TRACKING GPS...");
  }
  u8g2.drawStr(53, 56, etaLine);

  u8g2.sendBuffer();
}
