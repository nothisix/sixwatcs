#include "Display.h"

// Hardware I2C instance for SSD1306 (Wire is configured in Display::init with Wire.begin(4, 5))
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

namespace Display {

void init(uint8_t sda, uint8_t scl) {
  Serial.printf("[Display] Initializing SSD1306 Hardware I2C Display (SDA=%d, SCL=%d)...\n", sda, scl);
  delay(100); // Allow OLED power rail to stabilize after MCU reset
  Wire.end();
  Wire.begin(sda, scl);
  Wire.setClock(100000); // 100kHz Standard Safe Mode for crystal-clear signal integrity
  
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setPowerSave(0);
  u8g2.setContrast(140);

  // Clear startup RAM static
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  Serial.println("[Display] u8g2.begin() successfully completed with SSD1306 Hardware I2C!");
}

void reinit(uint8_t sda, uint8_t scl) {
  init(sda, scl);
}

void clear() {
  u8g2.clearBuffer();
}

void render() {
  u8g2.sendBuffer();
}

void drawCenteredStr(int16_t y, const char* str) {
  int16_t strWidth = u8g2.getUTF8Width(str);
  int16_t x = (SCREEN_WIDTH - strWidth) / 2;
  if (x < 0) x = 0;
  u8g2.drawUTF8(x, y, str);
}

void drawRightAlignedStr(int16_t x, int16_t y, const char* str) {
  int16_t strWidth = u8g2.getUTF8Width(str);
  int16_t posX = x - strWidth;
  if (posX < 0) posX = 0;
  u8g2.drawUTF8(posX, y, str);
}

void drawProgressBar(int16_t x, int16_t y, uint8_t w, uint8_t h, float progress) {
  if (progress < 0.0f) progress = 0.0f;
  if (progress > 1.0f) progress = 1.0f;

  u8g2.drawFrame(x, y, w, h);
  uint8_t fillWidth = (uint8_t)((w - 2) * progress);
  if (fillWidth > 0 && h > 2) {
    u8g2.drawBox(x + 1, y + 1, fillWidth, h - 2);
  }
}

void drawDashedHLine(int16_t x, int16_t y, int16_t w, uint8_t dash, uint8_t space) {
  int16_t curX = x;
  int16_t endX = x + w;
  while (curX < endX) {
    int16_t segment = (curX + dash <= endX) ? dash : (endX - curX);
    if (curX >= 0 && curX + segment <= SCREEN_WIDTH) {
      u8g2.drawHLine(curX, y, segment);
    }
    curX += dash + space;
  }
}

bool hasHangul(const String& str) {
  const uint8_t* p = (const uint8_t*)str.c_str();
  while (*p) {
    if (*p >= 0xE0 && *p <= 0xEF) {
      uint8_t b1 = *p;
      uint8_t b2 = *(p + 1);
      if (b2 == 0) break;
      // Hangul Syllables U+AC00..U+D7A3 (UTF-8: EA B0 80 .. ED 9E A3)
      if (b1 >= 0xEA && b1 <= 0xED) return true;
      // Hangul Compatibility Jamo U+3131..U+318E (UTF-8: E3 84 B1 .. E3 86 8E)
      if (b1 == 0xE3 && b2 >= 0x84 && b2 <= 0x86) return true;
      // Hangul Jamo U+1100..U+11FF (UTF-8: E1 84 80 .. E1 87 BF)
      if (b1 == 0xE1 && b2 >= 0x84 && b2 <= 0x87) return true;
    }
    p++;
  }
  return false;
}

String truncateUTF8(const String& str, int16_t maxWidth) {
  if (u8g2.getUTF8Width(str.c_str()) <= maxWidth) return str;
  String res = str;
  while (res.length() > 0 && (u8g2.getUTF8Width((res + "...").c_str()) > maxWidth)) {
    while (res.length() > 0) {
      uint8_t c = (uint8_t)res.charAt(res.length() - 1);
      res.remove(res.length() - 1);
      if ((c & 0xC0) != 0x80) {
        break; // Removed the leading byte of a multi-byte char or a 1-byte ASCII
      }
    }
  }
  return res + "...";
}

} // namespace Display
