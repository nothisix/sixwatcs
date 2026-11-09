#include "SettingsManager.h"
#include "Engine.h"
#include "ClockManager.h"

extern InputManager inputManager;

Preferences SettingsManager::s_prefs;
int8_t SettingsManager::s_selectedIndex = 0;
uint8_t SettingsManager::s_afkIndex = 3;       // Default 5 Min
uint8_t SettingsManager::s_contrastIndex = 1;  // Default MED (128)
bool SettingsManager::s_inverted = false;      // Default Normal
uint8_t SettingsManager::s_watchStyle = 0;     // Default Retro
uint8_t SettingsManager::s_spotifyStyle = 0;   // Default Classic HUD
bool SettingsManager::s_lyricsEnabled = true;  // Default Enabled
uint8_t SettingsManager::s_sdaPin = PIN_I2C_SDA;
uint8_t SettingsManager::s_sclPin = PIN_I2C_SCL;
uint8_t SettingsManager::s_btnLeftPin = PIN_BTN_LEFT;
uint8_t SettingsManager::s_btnOkPin = PIN_BTN_OK;
uint8_t SettingsManager::s_btnRightPin = PIN_BTN_RIGHT;

bool SettingsManager::s_scoreResetNotice = false;
unsigned long SettingsManager::s_noticeTimer = 0;

static const unsigned long AFK_VALUES[] = {
  60000UL,    // 1 Min
  120000UL,   // 2 Min
  180000UL,   // 3 Min
  300000UL,   // 5 Min
  600000UL,   // 10 Min
  0UL         // OFF
};

static const char* AFK_LABELS[] = {
  "1 MIN", "2 MIN", "3 MIN", "5 MIN", "10 MIN", "OFF"
};

static const uint8_t CONTRAST_VALUES[] = { 30, 128, 255 };
static const char* CONTRAST_LABELS[] = { "LOW", "MED", "MAX" };

static const char* WATCH_STYLE_LABELS[] = { "RETRO", "MINIMAL", "ANALOG", "CYBER", "BIG" };
static const char* SPOTIFY_STYLE_LABELS[] = { "CLASSIC", "CASSETTE", "LYRICS", "PLAYER" };

void SettingsManager::begin() {
  s_prefs.begin("settings", false);
  s_afkIndex = s_prefs.getUChar("afk_idx", 3);
  if (s_afkIndex > 5) s_afkIndex = 3;

  s_contrastIndex = s_prefs.getUChar("contrast", 1);
  if (s_contrastIndex > 2) s_contrastIndex = 1;

  s_inverted = s_prefs.getBool("invert", false);

  s_watchStyle = s_prefs.getUChar("style", 0);
  if (s_watchStyle > 4) s_watchStyle = 0;

  s_spotifyStyle = s_prefs.getUChar("spot_style", 0);
  if (s_spotifyStyle > 3) s_spotifyStyle = 0;

  s_lyricsEnabled = s_prefs.getBool("lyr_en", true);

  s_sdaPin = s_prefs.getUChar("sda_pin", PIN_I2C_SDA);
  s_sclPin = s_prefs.getUChar("scl_pin", PIN_I2C_SCL);
  s_btnLeftPin = s_prefs.getUChar("btn_l", PIN_BTN_LEFT);
  s_btnOkPin = s_prefs.getUChar("btn_ok", PIN_BTN_OK);
  s_btnRightPin = s_prefs.getUChar("btn_r", PIN_BTN_RIGHT);

  applyDisplaySettings();
  Serial.printf("[Settings] Loaded: WatchStyle=%s, SpotStyle=%s, Lyrics=%d, SDA=%d, SCL=%d, BTN=[%d,%d,%d]\n",
                WATCH_STYLE_LABELS[s_watchStyle],
                SPOTIFY_STYLE_LABELS[s_spotifyStyle],
                s_lyricsEnabled,
                s_sdaPin, s_sclPin,
                s_btnLeftPin, s_btnOkPin, s_btnRightPin);
}

void SettingsManager::applyDisplaySettings() {
  u8g2.setContrast(CONTRAST_VALUES[s_contrastIndex]);
  if (s_inverted) {
    u8g2.sendF("c", 0xa7); // Invert display
  } else {
    u8g2.sendF("c", 0xa6); // Normal display
  }
}

unsigned long SettingsManager::getAfkTimeoutMs() {
  if (s_afkIndex > 5) return 300000UL;
  return AFK_VALUES[s_afkIndex];
}

uint8_t SettingsManager::getContrast() {
  return CONTRAST_VALUES[s_contrastIndex];
}

bool SettingsManager::isInverted() {
  return s_inverted;
}

uint8_t SettingsManager::getWatchStyle() {
  return s_watchStyle;
}

uint8_t SettingsManager::getSpotifyStyle() {
  return s_spotifyStyle;
}

bool SettingsManager::isLyricsEnabled() {
  return s_lyricsEnabled;
}

uint8_t SettingsManager::getSdaPin() {
  return s_sdaPin;
}

uint8_t SettingsManager::getSclPin() {
  return s_sclPin;
}

uint8_t SettingsManager::getBtnLeftPin() {
  return s_btnLeftPin;
}

uint8_t SettingsManager::getBtnOkPin() {
  return s_btnOkPin;
}

uint8_t SettingsManager::getBtnRightPin() {
  return s_btnRightPin;
}

void SettingsManager::setWatchStyle(uint8_t style) {
  if (style <= 4) s_watchStyle = style;
}

void SettingsManager::setSpotifyStyle(uint8_t style) {
  if (style <= 3) s_spotifyStyle = style;
}

void SettingsManager::setLyricsEnabled(bool en) {
  s_lyricsEnabled = en;
}

void SettingsManager::setPins(uint8_t sda, uint8_t scl, uint8_t btnL, uint8_t btnOk, uint8_t btnR) {
  s_sdaPin = sda;
  s_sclPin = scl;
  s_btnLeftPin = btnL;
  s_btnOkPin = btnOk;
  s_btnRightPin = btnR;
}

void SettingsManager::setDisplayContrast(uint8_t idx) {
  if (idx <= 2) {
    s_contrastIndex = idx;
    applyDisplaySettings();
  }
}

void SettingsManager::setInverted(bool inv) {
  s_inverted = inv;
  applyDisplaySettings();
}

void SettingsManager::saveSettings() {
  s_prefs.putUChar("afk_idx", s_afkIndex);
  s_prefs.putUChar("contrast", s_contrastIndex);
  s_prefs.putBool("invert", s_inverted);
  s_prefs.putUChar("style", s_watchStyle);
  s_prefs.putUChar("spot_style", s_spotifyStyle);
  s_prefs.putBool("lyr_en", s_lyricsEnabled);
  s_prefs.putUChar("sda_pin", s_sdaPin);
  s_prefs.putUChar("scl_pin", s_sclPin);
  s_prefs.putUChar("btn_l", s_btnLeftPin);
  s_prefs.putUChar("btn_ok", s_btnOkPin);
  s_prefs.putUChar("btn_r", s_btnRightPin);
  Serial.println("[Settings] All settings successfully saved to NVS!");
}

void SettingsManager::resetHighScores() {
  Preferences arcadePrefs;
  arcadePrefs.begin("arcade", false);
  arcadePrefs.clear();
  arcadePrefs.end();
  s_scoreResetNotice = true;
  s_noticeTimer = millis();
  Serial.println("[Settings] All Arcade high scores wiped from NVS!");
}

void SettingsManager::sendCurrentConfig() {
  Serial.printf("{\"status\":\"ok\",\"type\":\"config\",\"sda\":%d,\"scl\":%d,\"btn_l\":%d,\"btn_ok\":%d,\"btn_r\":%d,\"clock_style\":%d,\"spotify_style\":%d,\"lyrics_enable\":%d,\"contrast\":%d,\"invert\":%d,\"afk\":%d}\n",
                s_sdaPin, s_sclPin, s_btnLeftPin, s_btnOkPin, s_btnRightPin,
                s_watchStyle, s_spotifyStyle, s_lyricsEnabled ? 1 : 0,
                s_contrastIndex, s_inverted ? 1 : 0, s_afkIndex);
}

bool SettingsManager::processSerialCommand(const String& cmd) {
  String trimmed = cmd;
  trimmed.trim();

  if (trimmed.equalsIgnoreCase("GET_CONFIG") || 
      trimmed.equalsIgnoreCase("CONFIG:GET") ||
      trimmed.indexOf("\"cmd\":\"get_config\"") != -1 ||
      trimmed.indexOf("\"cmd\": \"get_config\"") != -1) {
    sendCurrentConfig();
    return true;
  }

  if (trimmed.startsWith("{\"cmd\":\"set_config\"") || 
      trimmed.startsWith("{\"cmd\": \"set_config\"") ||
      trimmed.startsWith("SET_CONFIG:") ||
      trimmed.startsWith("CFG:")) {

    auto getIntVal = [&](const char* key, int defaultVal) -> int {
      int idx = trimmed.indexOf(key);
      if (idx == -1) return defaultVal;
      idx += strlen(key);
      while (idx < trimmed.length() && (trimmed[idx] == ' ' || trimmed[idx] == ':' || trimmed[idx] == '=' || trimmed[idx] == '\"')) {
        idx++;
      }
      int endIdx = idx;
      while (endIdx < trimmed.length() && (isDigit(trimmed[endIdx]) || trimmed[endIdx] == '-')) {
        endIdx++;
      }
      if (endIdx > idx) {
        return trimmed.substring(idx, endIdx).toInt();
      }
      return defaultVal;
    };

    uint8_t oldSda = s_sdaPin;
    uint8_t oldScl = s_sclPin;
    uint8_t oldBtnL = s_btnLeftPin;
    uint8_t oldBtnOk = s_btnOkPin;
    uint8_t oldBtnR = s_btnRightPin;

    s_sdaPin = (uint8_t)getIntVal("sda", s_sdaPin);
    s_sclPin = (uint8_t)getIntVal("scl", s_sclPin);
    s_btnLeftPin = (uint8_t)getIntVal("btn_l", s_btnLeftPin);
    s_btnOkPin = (uint8_t)getIntVal("btn_ok", s_btnOkPin);
    s_btnRightPin = (uint8_t)getIntVal("btn_r", s_btnRightPin);

    int cs = getIntVal("clock_style", -1);
    if (cs == -1) cs = getIntVal("clock", -1);
    if (cs >= 0 && cs <= 4) s_watchStyle = (uint8_t)cs;

    int ss = getIntVal("spotify_style", -1);
    if (ss == -1) ss = getIntVal("spotify", -1);
    if (ss >= 0 && ss <= 3) s_spotifyStyle = (uint8_t)ss;

    int le = getIntVal("lyrics_enable", -1);
    if (le == -1) le = getIntVal("lyrics", -1);
    if (le >= 0) s_lyricsEnabled = (le != 0);

    int ct = getIntVal("contrast", -1);
    if (ct >= 0 && ct <= 2) s_contrastIndex = (uint8_t)ct;

    int inv = getIntVal("invert", -1);
    if (inv >= 0) s_inverted = (inv != 0);

    saveSettings();
    applyDisplaySettings();

    // Reconfigure button pins if changed
    if (s_btnLeftPin != oldBtnL || s_btnOkPin != oldBtnOk || s_btnRightPin != oldBtnR) {
      inputManager.reconfigure(s_btnLeftPin, s_btnOkPin, s_btnRightPin);
      Serial.printf("[Settings] Button pins updated: L=%d, OK=%d, R=%d\n", s_btnLeftPin, s_btnOkPin, s_btnRightPin);
    }

    // Reconfigure I2C display if pins changed
    if (s_sdaPin != oldSda || s_sclPin != oldScl) {
      Display::reinit(s_sdaPin, s_sclPin);
      Serial.printf("[Settings] I2C pins updated: SDA=%d, SCL=%d\n", s_sdaPin, s_sclPin);
    }

    Serial.println("{\"status\":\"saved\",\"msg\":\"Config applied successfully!\"}");
    return true;
  }

  return false;
}

void SettingsManager::update(ButtonEvent event) {
  // Clear reset notice after 2 seconds
  if (s_scoreResetNotice && (millis() - s_noticeTimer >= 2000)) {
    s_scoreResetNotice = false;
  }

  // HOLD OK: Quick Save & Exit from anywhere in Settings
  if (event == BTN_LONG_PRESS) {
    saveSettings();
    Engine::returnToMenu();
    return;
  }

  // LEFT Button: Move UP
  if (event == BTN_LEFT_CLICK) {
    s_selectedIndex = (s_selectedIndex - 1 + ITEM_COUNT) % ITEM_COUNT;
  }
  // RIGHT Button: Move DOWN
  else if (event == BTN_RIGHT_CLICK) {
    s_selectedIndex = (s_selectedIndex + 1) % ITEM_COUNT;
  }
  // OK Button: Toggle / Cycle value or Execute
  else if (event == BTN_CLICK) {
    switch (s_selectedIndex) {
      case ITEM_AFK:
        s_afkIndex = (s_afkIndex + 1) % 6;
        break;

      case ITEM_CONTRAST:
        s_contrastIndex = (s_contrastIndex + 1) % 3;
        applyDisplaySettings();
        break;

      case ITEM_INVERT:
        s_inverted = !s_inverted;
        applyDisplaySettings();
        break;

      case ITEM_WATCH_STYLE:
        s_watchStyle = (s_watchStyle + 1) % 5;
        break;

      case ITEM_SPOTIFY_STYLE:
        s_spotifyStyle = (s_spotifyStyle + 1) % 4;
        break;

      case ITEM_LYRICS_MODE:
        s_lyricsEnabled = !s_lyricsEnabled;
        break;

      case ITEM_RESET_SCORES:
        resetHighScores();
        break;

      case ITEM_BLE_INFO:
        // Informational, no action
        break;

      case ITEM_SAVE_EXIT:
        saveSettings();
        Engine::returnToMenu();
        return;
    }
  }

  render();
}

void SettingsManager::render() {
  u8g2.clearBuffer();

  // Top Title Bar
  u8g2.setFont(u8g2_font_6x10_tf);
  Display::drawCenteredStr(10, "- SETTINGS -");
  u8g2.drawHLine(0, 12, SCREEN_WIDTH);

  // Scroll window: 3 items visible at a time
  int8_t topIndex = s_selectedIndex - 1;
  if (topIndex < 0) topIndex = 0;
  if (topIndex > ITEM_COUNT - 3) topIndex = ITEM_COUNT - 3;

  for (uint8_t i = 0; i < 3; i++) {
    int8_t itemIdx = topIndex + i;
    if (itemIdx >= ITEM_COUNT) break;

    int16_t rowY = 14 + (i * 12);
    bool isSelected = (itemIdx == s_selectedIndex);

    if (isSelected) {
      u8g2.drawRBox(2, rowY, SCREEN_WIDTH - 4, 11, 2);
      u8g2.setDrawColor(0); // Invert text color
    } else {
      u8g2.setDrawColor(1);
    }

    u8g2.setFont(u8g2_font_5x7_tf);
    int16_t textY = rowY + 8;

    char labelBuf[24];
    char valBuf[24];

    switch (itemIdx) {
      case ITEM_AFK:
        snprintf(labelBuf, sizeof(labelBuf), "AFK TIMEOUT");
        snprintf(valBuf, sizeof(valBuf), "< %s >", AFK_LABELS[s_afkIndex]);
        break;

      case ITEM_CONTRAST:
        snprintf(labelBuf, sizeof(labelBuf), "BRIGHTNESS");
        snprintf(valBuf, sizeof(valBuf), "< %s >", CONTRAST_LABELS[s_contrastIndex]);
        break;

      case ITEM_INVERT:
        snprintf(labelBuf, sizeof(labelBuf), "INVERT SCREEN");
        snprintf(valBuf, sizeof(valBuf), "< %s >", s_inverted ? "ON" : "OFF");
        break;

      case ITEM_WATCH_STYLE:
        snprintf(labelBuf, sizeof(labelBuf), "WATCH STYLE");
        snprintf(valBuf, sizeof(valBuf), "< %s >", WATCH_STYLE_LABELS[s_watchStyle]);
        break;

      case ITEM_SPOTIFY_STYLE:
        snprintf(labelBuf, sizeof(labelBuf), "SPOTIFY UI");
        snprintf(valBuf, sizeof(valBuf), "< %s >", SPOTIFY_STYLE_LABELS[s_spotifyStyle]);
        break;

      case ITEM_LYRICS_MODE:
        snprintf(labelBuf, sizeof(labelBuf), "LYRICS");
        snprintf(valBuf, sizeof(valBuf), "< %s >", s_lyricsEnabled ? "ON" : "OFF");
        break;

      case ITEM_RESET_SCORES:
        snprintf(labelBuf, sizeof(labelBuf), "RESET SCORES");
        snprintf(valBuf, sizeof(valBuf), s_scoreResetNotice ? "[CLEARED!]" : "[PRESS OK]");
        break;

      case ITEM_BLE_INFO:
        snprintf(labelBuf, sizeof(labelBuf), "BLE");
        snprintf(valBuf, sizeof(valBuf), "%s", ClockManager::getBleAddress());
        break;

      case ITEM_SAVE_EXIT:
        snprintf(labelBuf, sizeof(labelBuf), ">>> SAVE & EXIT <<<");
        valBuf[0] = '\0';
        break;
    }

    if (itemIdx == ITEM_SAVE_EXIT) {
      Display::drawCenteredStr(textY, labelBuf);
    } else {
      u8g2.drawStr(6, textY, labelBuf);
      Display::drawRightAlignedStr(SCREEN_WIDTH - 6, textY, valBuf);
    }

    u8g2.setDrawColor(1); // Restore draw color
  }

  // Bottom Instructions Bar
  u8g2.drawHLine(0, 52, SCREEN_WIDTH);
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(4, 61, "L/R: UP/DN");
  Display::drawCenteredStr(61, "OK: EDIT");
  Display::drawRightAlignedStr(SCREEN_WIDTH - 4, 61, "HOLD: SAVE");

  u8g2.sendBuffer();
}
