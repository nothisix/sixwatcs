#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "Config.h"

// High-Reliability Full-Buffer Hardware I2C Driver for SSD1306 (SDA=GPIO 4, SCL=GPIO 5)
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;

namespace Display {
  void init();
  void clear();
  void render();

  // Helper drawing functions
  void drawCenteredStr(int16_t y, const char* str);
  void drawRightAlignedStr(int16_t x, int16_t y, const char* str);
  void drawProgressBar(int16_t x, int16_t y, uint8_t w, uint8_t h, float progress);
  void drawDashedHLine(int16_t x, int16_t y, int16_t w, uint8_t dash = 3, uint8_t space = 2);

  // UTF-8 & Korean Hangul Support
  bool hasHangul(const String& str);
  String truncateUTF8(const String& str, int16_t maxWidth);
}
