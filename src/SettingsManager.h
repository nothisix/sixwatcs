#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "ButtonHandler.h"
#include "Display.h"

class SettingsManager {
public:
  static void begin();
  static void update(ButtonEvent event);
  static void render();

  // Settings getters
  static unsigned long getAfkTimeoutMs();
  static uint8_t getContrast();
  static bool isInverted();
  static uint8_t getWatchStyle();

  // Actions
  static void saveSettings();
  static void resetHighScores();
  static void applyDisplaySettings();

private:
  static Preferences s_prefs;

  // Setting indices
  enum SettingItem {
    ITEM_AFK = 0,
    ITEM_CONTRAST,
    ITEM_INVERT,
    ITEM_WATCH_STYLE,
    ITEM_RESET_SCORES,
    ITEM_BLE_INFO,
    ITEM_SAVE_EXIT,
    ITEM_COUNT
  };

  static int8_t s_selectedIndex;
  static uint8_t s_afkIndex;       // 0=1m, 1=2m, 2=3m, 3=5m, 4=10m, 5=OFF
  static uint8_t s_contrastIndex;  // 0=LOW(30), 1=MED(128), 2=HIGH(255)
  static bool s_inverted;          // false=Normal, true=Inverted
  static uint8_t s_watchStyle;     // 0=Classic, 1=Minimal, 2=Cyber
  static bool s_scoreResetNotice;
  static unsigned long s_noticeTimer;
};
