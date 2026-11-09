#include "SettingsManager.h"
#include "Engine.h"
#include "ClockManager.h"

Preferences SettingsManager::s_prefs;
int8_t SettingsManager::s_selectedIndex = 0;
uint8_t SettingsManager::s_afkIndex = 3;       // Default 5 Min
uint8_t SettingsManager::s_contrastIndex = 1;  // Default MED (128)
bool SettingsManager::s_inverted = false;      // Default Normal
uint8_t SettingsManager::s_watchStyle = 0;     // Default Retro
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

static const char* STYLE_LABELS[] = { "RETRO", "MINIMAL" };

void SettingsManager::begin() {
  s_prefs.begin("settings", false);
  s_afkIndex = s_prefs.getUChar("afk_idx", 3);
  if (s_afkIndex > 5) s_afkIndex = 3;

  s_contrastIndex = s_prefs.getUChar("contrast", 1);
  if (s_contrastIndex > 2) s_contrastIndex = 1;

  s_inverted = s_prefs.getBool("invert", false);
  s_watchStyle = s_prefs.getUChar("style", 0);
  if (s_watchStyle > 1) s_watchStyle = 0;

  applyDisplaySettings();
  Serial.printf("[Settings] Loaded: AFK=%s, Contrast=%s, Invert=%d, Style=%s\n",
                AFK_LABELS[s_afkIndex],
                CONTRAST_LABELS[s_contrastIndex],
                s_inverted,
                STYLE_LABELS[s_watchStyle]);
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

void SettingsManager::saveSettings() {
  s_prefs.putUChar("afk_idx", s_afkIndex);
  s_prefs.putUChar("contrast", s_contrastIndex);
  s_prefs.putBool("invert", s_inverted);
  s_prefs.putUChar("style", s_watchStyle);
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
        s_watchStyle = (s_watchStyle + 1) % 2;
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
        snprintf(valBuf, sizeof(valBuf), "< %s >", STYLE_LABELS[s_watchStyle]);
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
