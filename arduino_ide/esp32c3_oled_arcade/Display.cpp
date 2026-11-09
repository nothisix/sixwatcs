#include "Display.h"

// Hardware I2C instance for SSD1306 (Wire is configured in Display::init with Wire.begin(4, 5))
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

namespace Display {

void init() {
  Serial.println("[Display] Initializing SSD1306 Hardware I2C Display...");
  delay(100); // Allow OLED power rail to stabilize after MCU reset
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(100000); // 100kHz Standard Safe Mode for crystal-clear signal integrity
  
  u8g2.begin();
  u8g2.setPowerSave(0);
  u8g2.setContrast(140);

  // Clear startup RAM static
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  Serial.println("[Display] u8g2.begin() successfully completed with SSD1306 Hardware I2C!");
}

void clear() {
  u8g2.clearBuffer();
}

void render() {
  u8g2.sendBuffer();
}

void drawCenteredStr(int16_t y, const char* str) {
  int16_t strWidth = u8g2.getStrWidth(str);
  int16_t x = (SCREEN_WIDTH - strWidth) / 2;
  if (x < 0) x = 0;
  u8g2.drawStr(x, y, str);
}

void drawRightAlignedStr(int16_t x, int16_t y, const char* str) {
  int16_t strWidth = u8g2.getStrWidth(str);
  int16_t posX = x - strWidth;
  if (posX < 0) posX = 0;
  u8g2.drawStr(posX, y, str);
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

} // namespace Display
