#include "GameStacker.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameStacker {

struct StackRow {
  int16_t x;
  int16_t width;
};

static constexpr uint8_t MAX_ROWS = 9;
static constexpr uint8_t ROW_HEIGHT = 5;
static constexpr uint8_t BASE_Y = 54;

static StackRow s_tower[MAX_ROWS];
static uint8_t s_currentRow = 0;
static float s_movingX = 10.0f;
static float s_movingSpeed = 2.2f;
static int16_t s_movingWidth = 36;
static bool s_isGameOver = false;
static bool s_isWon = false;

void init() {
  s_currentRow = 0;
  s_movingX = 20.0f;
  s_movingSpeed = 2.2f;
  s_movingWidth = 36;
  s_isGameOver = false;
  s_isWon = false;

  for (uint8_t i = 0; i < MAX_ROWS; i++) {
    s_tower[i].x = 0;
    s_tower[i].width = 0;
  }
}

void update(ButtonEvent event) {
  // =========================================================================
  // GAME OVER / VICTORY SCREEN
  // =========================================================================
  if (s_isGameOver || s_isWon) {
    if (event == BTN_CLICK) {
      init();
      return;
    } else if (event == BTN_LONG_PRESS) {
      Engine::returnToMenu();
      return;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    if (s_isWon) {
      Display::drawCenteredStr(16, "JACKPOT WINNER!");
    } else {
      Display::drawCenteredStr(16, "TOWER COLLAPSED!");
    }

    char buf[24];
    uint32_t score = s_currentRow * 10;
    snprintf(buf, sizeof(buf), "LEVEL: %u / %u", s_currentRow, MAX_ROWS);
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(30, buf);

    uint32_t hi = Engine::getHighScore("STACKER");
    snprintf(buf, sizeof(buf), "RECORD: %lu", (unsigned long)hi);
    Display::drawCenteredStr(42, buf);

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawHLine(0, 52, SCREEN_WIDTH);
    u8g2.drawStr(6, 61, "CLICK: RETRY");
    Display::drawRightAlignedStr(SCREEN_WIDTH - 6, 61, "HOLD: MENU");

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // ACTIVE STACKER MECHANICS
  // =========================================================================
  if (event == BTN_LONG_PRESS) {
    Engine::returnToMenu();
    return;
  }

  // Move active oscillating block
  s_movingX += s_movingSpeed;
  if (s_movingX < 4.0f) {
    s_movingX = 4.0f;
    s_movingSpeed = -s_movingSpeed;
  } else if (s_movingX > SCREEN_WIDTH - s_movingWidth - 4.0f) {
    s_movingX = SCREEN_WIDTH - s_movingWidth - 4.0f;
    s_movingSpeed = -s_movingSpeed;
  }

  // Button Click places current block
  if (event == BTN_CLICK) {
    int16_t curX = (int16_t)s_movingX;

    if (s_currentRow == 0) {
      // First base block always lands securely
      s_tower[0].x = curX;
      s_tower[0].width = s_movingWidth;
      s_currentRow++;
      s_movingSpeed = (s_movingSpeed > 0 ? 1 : -1) * (2.4f + s_currentRow * 0.35f);
    } else {
      int16_t prevX = s_tower[s_currentRow - 1].x;
      int16_t prevW = s_tower[s_currentRow - 1].width;

      int16_t overlapL = (curX > prevX) ? curX : prevX;
      int16_t overlapR = ((curX + s_movingWidth) < (prevX + prevW)) ? (curX + s_movingWidth) : (prevX + prevW);
      int16_t overlapW = overlapR - overlapL;

      if (overlapW <= 0) {
        // Complete miss!
        s_isGameOver = true;
        Engine::saveHighScore("STACKER", s_currentRow * 10);
      } else {
        // Successfully stacked with trimmed overhang
        s_tower[s_currentRow].x = overlapL;
        s_tower[s_currentRow].width = overlapW;
        s_movingWidth = overlapW;
        s_currentRow++;

        Engine::saveHighScore("STACKER", s_currentRow * 10);

        if (s_currentRow >= MAX_ROWS) {
          s_isWon = true;
        } else {
          // Increase speed for next tier
          s_movingSpeed = (s_movingSpeed > 0 ? 1 : -1) * (2.4f + s_currentRow * 0.35f);
        }
      }
    }
  }

  // =========================================================================
  // RENDER GRAPHICS
  // =========================================================================
  u8g2.clearBuffer();

  // Draw Ground Line
  u8g2.drawHLine(0, BASE_Y + ROW_HEIGHT + 1, SCREEN_WIDTH);

  // Draw Stacked Placed Rows
  for (uint8_t r = 0; r < s_currentRow; r++) {
    int16_t y = BASE_Y - (r * (ROW_HEIGHT + 1));
    u8g2.drawBox(s_tower[r].x, y, s_tower[r].width, ROW_HEIGHT);
    u8g2.drawFrame(s_tower[r].x - 1, y - 1, s_tower[r].width + 2, ROW_HEIGHT + 2);
  }

  // Draw Currently Moving Block
  if (!s_isGameOver && !s_isWon && s_currentRow < MAX_ROWS) {
    int16_t y = BASE_Y - (s_currentRow * (ROW_HEIGHT + 1));
    u8g2.drawBox((int16_t)s_movingX, y, s_movingWidth, ROW_HEIGHT);
  }

  // HUD: Floor Height
  u8g2.setFont(u8g2_font_4x6_tf);
  char flBuf[16];
  snprintf(flBuf, sizeof(flBuf), "LVL %u/%u", s_currentRow, MAX_ROWS);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 6, flBuf);

  u8g2.sendBuffer();
}

Minigame game = {
  "STACKER",
  init,
  update,
  BITMAP_STACKER,
  BITMAP_STACKER_WIDTH,
  BITMAP_STACKER_HEIGHT,
  "CLICK TO PLACE"
};

} // namespace GameStacker
