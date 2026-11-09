#include "GameSnake.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameSnake {

struct Point {
  int8_t x;
  int8_t y;
};

static constexpr uint8_t GRID_W = 24;
static constexpr uint8_t GRID_H = 11;
static constexpr uint8_t CELL_SIZE = 4;
static constexpr uint8_t OFFSET_X = 16;
static constexpr uint8_t OFFSET_Y = 14;
static constexpr uint8_t MAX_SNAKE_LEN = 64;

static Point s_snake[MAX_SNAKE_LEN];
static uint8_t s_length = 4;
static uint8_t s_dir = 0; // 0=RIGHT, 1=DOWN, 2=LEFT, 3=UP
static Point s_food = {12, 5};

static uint16_t s_score = 0;
static bool s_isGameOver = false;
static uint8_t s_frameCounter = 0;
static uint8_t s_stepInterval = 4;

static void spawnFood() {
  bool valid = false;
  while (!valid) {
    s_food.x = millis() % GRID_W;
    s_food.y = (millis() / 7) % GRID_H;
    valid = true;
    for (uint8_t i = 0; i < s_length; i++) {
      if (s_snake[i].x == s_food.x && s_snake[i].y == s_food.y) {
        valid = false;
        break;
      }
    }
  }
}

void init() {
  s_length = 4;
  s_dir = 0; // Facing Right
  s_score = 0;
  s_isGameOver = false;
  s_frameCounter = 0;
  s_stepInterval = 4;

  s_snake[0] = {6, 5};
  s_snake[1] = {5, 5};
  s_snake[2] = {4, 5};
  s_snake[3] = {3, 5};

  spawnFood();
}

void update(ButtonEvent event) {
  // =========================================================================
  // GAME OVER SCREEN
  // =========================================================================
  if (s_isGameOver) {
    if (event == BTN_CLICK || event == BTN_LEFT_CLICK || event == BTN_RIGHT_CLICK) {
      init();
      return;
    } else if (event == BTN_LONG_PRESS) {
      Engine::returnToMenu();
      return;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    Display::drawCenteredStr(16, "SNAKE CRASHED!");

    char buf[24];
    snprintf(buf, sizeof(buf), "LENGTH: %u", s_length);
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(30, buf);

    uint32_t hi = Engine::getHighScore("RETRO SNAKE");
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
  // 3-BUTTON SNAKE DIRECTION CONTROL
  // =========================================================================
  if (event == BTN_LONG_PRESS) {
    Engine::returnToMenu();
    return;
  }

  // Left button turns Left (Counter-Clockwise), Right button turns Right (Clockwise)
  if (event == BTN_LEFT_CLICK) {
    s_dir = (s_dir + 3) % 4;
  } else if (event == BTN_RIGHT_CLICK) {
    s_dir = (s_dir + 1) % 4;
  } else if (event == BTN_CLICK) {
    s_dir = (s_dir + 1) % 4; // OK click turns 90 degrees CW (1-button compat)
  }

  // Paced grid step
  s_frameCounter++;
  if (s_frameCounter < s_stepInterval) {
    // Render current frame and wait for step frame
  } else {
    s_frameCounter = 0;

    // Calculate new head position
    Point newHead = s_snake[0];
    if (s_dir == 0) newHead.x++;      // RIGHT
    else if (s_dir == 1) newHead.y++; // DOWN
    else if (s_dir == 2) newHead.x--; // LEFT
    else if (s_dir == 3) newHead.y--; // UP

    // Wall collision
    if (newHead.x < 0 || newHead.x >= GRID_W || newHead.y < 0 || newHead.y >= GRID_H) {
      s_isGameOver = true;
      Engine::saveHighScore("RETRO SNAKE", s_score);
      return;
    }

    // Self collision
    for (uint8_t i = 0; i < s_length; i++) {
      if (s_snake[i].x == newHead.x && s_snake[i].y == newHead.y) {
        s_isGameOver = true;
        Engine::saveHighScore("RETRO SNAKE", s_score);
        return;
      }
    }

    // Check food eaten
    bool ateFood = (newHead.x == s_food.x && newHead.y == s_food.y);

    // Shift body
    uint8_t limit = ateFood ? (s_length < MAX_SNAKE_LEN - 1 ? s_length + 1 : s_length) : s_length;
    for (uint8_t i = limit; i > 0; i--) {
      s_snake[i] = s_snake[i - 1];
    }
    s_snake[0] = newHead;

    if (ateFood) {
      if (s_length < MAX_SNAKE_LEN - 1) {
        s_length++;
      }
      s_score += 10;
      Engine::saveHighScore("RETRO SNAKE", s_score);
      spawnFood();
      // Increase speed slightly
      if (s_length % 5 == 0 && s_stepInterval > 2) {
        s_stepInterval--;
      }
    }
  }

  // =========================================================================
  // RENDER GRAPHICS
  // =========================================================================
  u8g2.clearBuffer();

  // Draw Arena Border
  u8g2.drawFrame(OFFSET_X - 2, OFFSET_Y - 2, (GRID_W * CELL_SIZE) + 4, (GRID_H * CELL_SIZE) + 4);

  // Draw Food (blinking diamond)
  int16_t fx = OFFSET_X + (s_food.x * CELL_SIZE);
  int16_t fy = OFFSET_Y + (s_food.y * CELL_SIZE);
  u8g2.drawBox(fx + 1, fy, 2, 4);
  u8g2.drawBox(fx, fy + 1, 4, 2);

  // Draw Snake Body
  for (uint8_t i = 0; i < s_length; i++) {
    int16_t sx = OFFSET_X + (s_snake[i].x * CELL_SIZE);
    int16_t sy = OFFSET_Y + (s_snake[i].y * CELL_SIZE);
    if (i == 0) {
      // Head
      u8g2.drawBox(sx, sy, CELL_SIZE, CELL_SIZE);
    } else {
      // Body segment with inner hole for retro look
      u8g2.drawFrame(sx, sy, CELL_SIZE, CELL_SIZE);
    }
  }

  // HUD: Length / Score
  u8g2.setFont(u8g2_font_4x6_tf);
  char scBuf[16];
  snprintf(scBuf, sizeof(scBuf), "LEN:%u  %u", s_length, s_score);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 8, scBuf);

  u8g2.sendBuffer();
}

Minigame game = {
  "RETRO SNAKE",
  init,
  update,
  BITMAP_SNAKE,
  BITMAP_SNAKE_WIDTH,
  BITMAP_SNAKE_HEIGHT,
  "CLICK TO TURN 90 DEG"
};

} // namespace GameSnake
