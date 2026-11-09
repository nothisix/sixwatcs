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
  static uint8_t getSpotifyStyle();
  static bool isLyricsEnabled();
  static uint8_t getSdaPin();
  static uint8_t getSclPin();
  static uint8_t getBtnLeftPin();
  static uint8_t getBtnOkPin();
  static uint8_t getBtnRightPin();

  // Setters
  static void setWatchStyle(uint8_t style);
  static void setSpotifyStyle(uint8_t style);
  static void setLyricsEnabled(bool en);
  static void setPins(uint8_t sda, uint8_t scl, uint8_t btnL, uint8_t btnOk, uint8_t btnR);
  static void setDisplayContrast(uint8_t idx);
  static void setInverted(bool inv);

  // Actions
  static void saveSettings();
  static void resetHighScores();
  static void applyDisplaySettings();

  // Web Serial Configurator Protocol
  static bool processSerialCommand(const String& cmd);
  static void sendCurrentConfig();

private:
  static Preferences s_prefs;

  // Setting indices in on-device settings menu
  enum SettingItem {
    ITEM_AFK = 0,
    ITEM_CONTRAST,
    ITEM_INVERT,
    ITEM_WATCH_STYLE,
    ITEM_SPOTIFY_STYLE,
    ITEM_LYRICS_MODE,
    ITEM_RESET_SCORES,
    ITEM_BLE_INFO,
    ITEM_SAVE_EXIT,
    ITEM_COUNT
  };

  static int8_t s_selectedIndex;
  static uint8_t s_afkIndex;       // 0=1m, 1=2m, 2=3m, 3=5m, 4=10m, 5=OFF
  static uint8_t s_contrastIndex;  // 0=LOW(30), 1=MED(128), 2=HIGH(255)
  static bool s_inverted;          // false=Normal, true=Inverted
  static uint8_t s_watchStyle;     // 0=Retro, 1=Minimal, 2=Analog, 3=Cyberpunk, 4=Big Glance
  static uint8_t s_spotifyStyle;   // 0=Classic HUD, 1=Cassette, 2=Lyrics Focus, 3=Compact Vinyl
  static bool s_lyricsEnabled;     // true=Show lyrics, false=Off
  static uint8_t s_sdaPin;
  static uint8_t s_sclPin;
  static uint8_t s_btnLeftPin;
  static uint8_t s_btnOkPin;
  static uint8_t s_btnRightPin;

  static bool s_scoreResetNotice;
  static unsigned long s_noticeTimer;
};
