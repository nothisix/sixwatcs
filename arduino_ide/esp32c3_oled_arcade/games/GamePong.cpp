#include "GamePong.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GamePong {

static constexpr uint8_t PADDLE_Y = 56;
static constexpr uint8_t PADDLE_W = 24;
static constexpr uint8_t PADDLE_H = 4;
static constexpr uint8_t NUM_BRICKS = 16;

static float s_paddleX = 52.0f;
static float s_paddleDir = 2.0f;

static float s_ballX = 64.0f;
static float s_ballY = 40.0f;
static float s_ballVx = 1.4f;
static float s_ballVy = -1.6f;

static bool s_bricks[NUM_BRICKS];
static uint32_t s_score = 0;
static bool s_isGameOver = false;

static void resetBricks() {
  for (uint8_t i = 0; i < NUM_BRICKS; i++) {
    s_bricks[i] = true;
  }
}

void init() {
  s_paddleX = 52.0f;
  s_paddleDir = 2.0f;
  s_ballX = 64.0f;
  s_ballY = 40.0f;
  s_ballVx = 1.4f;
  s_ballVy = -1.6f;
  s_score = 0;
  s_isGameOver = false;

  resetBricks();
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
    Display::drawCenteredStr(16, "BALL DROPPED!");

    char buf[24];
    snprintf(buf, sizeof(buf), "SCORE: %lu", (unsigned long)s_score);
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(30, buf);

    uint32_t hi = Engine::getHighScore("BRICK POP");
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
  // 3-BUTTON PADDLE CONTROL
  // =========================================================================
  if (event == BTN_LONG_PRESS) {
    Engine::returnToMenu();
    return;
  }

  // 3-button direct movement
  if (event == BTN_LEFT_CLICK || event == BTN_LEFT_HOLD) {
    s_paddleX -= 3.0f;
  } else if (event == BTN_RIGHT_CLICK || event == BTN_RIGHT_HOLD) {
    s_paddleX += 3.0f;
  } else if (event == BTN_CLICK) {
    // OK Click flips paddle direction (1-button compat)
    s_paddleDir = -s_paddleDir;
    s_paddleX += s_paddleDir;
  } else {
    // Gentle auto-drift if no button pressed
    s_paddleX += s_paddleDir;
  }
  if (s_paddleX < 2.0f) {
    s_paddleX = 2.0f;
    s_paddleDir = -s_paddleDir;
  } else if (s_paddleX > SCREEN_WIDTH - PADDLE_W - 2.0f) {
    s_paddleX = SCREEN_WIDTH - PADDLE_W - 2.0f;
    s_paddleDir = -s_paddleDir;
  }

  // Move ball
  s_ballX += s_ballVx;
  s_ballY += s_ballVy;

  // Ball wall bounce (left & right)
  if (s_ballX < 2.0f) {
    s_ballX = 2.0f;
    s_ballVx = -s_ballVx;
  } else if (s_ballX > SCREEN_WIDTH - 4.0f) {
    s_ballX = SCREEN_WIDTH - 4.0f;
    s_ballVx = -s_ballVx;
  }

  // Ball ceiling bounce
  if (s_ballY < 2.0f) {
    s_ballY = 2.0f;
    s_ballVy = -s_ballVy;
  }

  // Ball paddle collision
  if (s_ballY >= PADDLE_Y - 3.0f && s_ballY <= PADDLE_Y + 2.0f) {
    if (s_ballX >= s_paddleX - 2.0f && s_ballX <= s_paddleX + PADDLE_W + 2.0f) {
      s_ballY = PADDLE_Y - 3.0f;
      s_ballVy = -fabsf(s_ballVy);

      // Angle variance based on hit location
      float hitOffset = (s_ballX - (s_paddleX + PADDLE_W / 2.0f)) / (PADDLE_W / 2.0f);
      s_ballVx = hitOffset * 2.2f;
      if (fabsf(s_ballVx) < 0.5f) s_ballVx = (hitOffset >= 0 ? 0.8f : -0.8f);
    }
  }

  // Ball floor drop (Game Over)
  if (s_ballY > SCREEN_HEIGHT + 4.0f) {
    s_isGameOver = true;
    Engine::saveHighScore("BRICK POP", s_score);
    return;
  }

  // Ball Brick Collision
  uint8_t remainingBricks = 0;
  for (uint8_t i = 0; i < NUM_BRICKS; i++) {
    if (!s_bricks[i]) continue;
    remainingBricks++;

    uint8_t row = i / 8; // 0 or 1
    uint8_t col = i % 8; // 0..7

    int16_t bx = 8 + (col * 14);
    int16_t by = 10 + (row * 6);
    int16_t bw = 12;
    int16_t bh = 4;

    if (s_ballX + 2.0f >= bx && s_ballX <= bx + bw &&
        s_ballY + 2.0f >= by && s_ballY <= by + bh) {
      s_bricks[i] = false;
      s_ballVy = -s_ballVy;
      s_score += 10;
      Engine::saveHighScore("BRICK POP", s_score);
      break;
    }
  }

  if (remainingBricks == 0) {
    s_score += 100;
    resetBricks();
  }

  // =========================================================================
  // RENDER GRAPHICS
  // =========================================================================
  u8g2.clearBuffer();

  // Draw Arena Walls
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  // Draw Bricks
  for (uint8_t i = 0; i < NUM_BRICKS; i++) {
    if (s_bricks[i]) {
      uint8_t row = i / 8;
      uint8_t col = i % 8;
      int16_t bx = 8 + (col * 14);
      int16_t by = 10 + (row * 6);
      u8g2.drawBox(bx, by, 12, 4);
    }
  }

  // Draw Paddle
  u8g2.drawBox((int16_t)s_paddleX, PADDLE_Y, PADDLE_W, PADDLE_H);
  u8g2.drawPixel((int16_t)s_paddleX + PADDLE_W / 2, PADDLE_Y + 1);

  // Draw Ball
  u8g2.drawBox((int16_t)s_ballX, (int16_t)s_ballY, 3, 3);

  // HUD Score
  u8g2.setFont(u8g2_font_4x6_tf);
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)s_score);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 4, 7, buf);

  u8g2.sendBuffer();
}

Minigame game = {
  "BRICK POP",
  init,
  update,
  BITMAP_PONG,
  BITMAP_PONG_WIDTH,
  BITMAP_PONG_HEIGHT,
  "CLICK TO FLIP PADDLE"
};

} // namespace GamePong
