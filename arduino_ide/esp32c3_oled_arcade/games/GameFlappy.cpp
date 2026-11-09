#include "GameFlappy.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameFlappy {

struct Pipe {
  float x;
  int16_t gapY;
  bool passed;
};

static constexpr uint8_t PIPE_WIDTH = 12;
static constexpr uint8_t PIPE_GAP = 24;
static constexpr uint8_t GROUND_Y = 56;
static constexpr float GRAVITY = 0.28f;
static constexpr float FLAP_IMPULSE = -2.8f;
static constexpr float PIPE_SPEED = 1.6f;

static float s_birdY = 24.0f;
static float s_birdVy = 0.0f;
static uint8_t s_wingTimer = 0;

static Pipe s_pipes[2];
static uint16_t s_score = 0;
static bool s_isGameOver = false;
static uint8_t s_groundOffset = 0;

void init() {
  s_birdY = 24.0f;
  s_birdVy = 0.0f;
  s_wingTimer = 0;
  s_score = 0;
  s_isGameOver = false;
  s_groundOffset = 0;

  s_pipes[0].x = 128.0f;
  s_pipes[0].gapY = 16;
  s_pipes[0].passed = false;

  s_pipes[1].x = 128.0f + 70.0f;
  s_pipes[1].gapY = 22;
  s_pipes[1].passed = false;
}

void update(ButtonEvent event) {
  // =========================================================================
  // GAME OVER LOGIC
  // =========================================================================
  if (s_isGameOver) {
    if (event == BTN_CLICK) {
      init();
      return;
    } else if (event == BTN_LONG_PRESS) {
      Engine::returnToMenu();
      return;
    }

    // Render Game Over Overlay
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_6x10_tf);
    Display::drawCenteredStr(16, "GAME OVER");

    char buf[32];
    snprintf(buf, sizeof(buf), "SCORE: %u", s_score);
    Display::drawCenteredStr(29, buf);

    uint32_t hi = Engine::getHighScore("FLAPPY C3");
    snprintf(buf, sizeof(buf), "BEST:  %lu", (unsigned long)hi);
    Display::drawCenteredStr(41, buf);

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawHLine(0, 50, SCREEN_WIDTH);
    u8g2.drawStr(6, 60, "CLICK: RETRY");
    Display::drawRightAlignedStr(SCREEN_WIDTH - 6, 60, "HOLD: MENU");

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // ACTIVE GAMEPLAY LOGIC
  // =========================================================================

  // Handle Input
  if (event == BTN_CLICK) {
    s_birdVy = FLAP_IMPULSE;
    s_wingTimer = 6;
  } else if (event == BTN_LONG_PRESS) {
    // In-game hold to abort to menu
    Engine::returnToMenu();
    return;
  }

  // Update Bird Physics
  s_birdVy += GRAVITY;
  if (s_birdVy > 3.8f) s_birdVy = 3.8f;
  s_birdY += s_birdVy;

  if (s_wingTimer > 0) s_wingTimer--;

  // Ground collision
  if (s_birdY + BITMAP_BIRD_HEIGHT >= GROUND_Y) {
    s_birdY = GROUND_Y - BITMAP_BIRD_HEIGHT;
    s_isGameOver = true;
    Engine::saveHighScore("FLAPPY C3", s_score);
  }

  // Ceiling clamp
  if (s_birdY < 0.0f) {
    s_birdY = 0.0f;
    s_birdVy = 0.0f;
  }

  // Update Ground Scroll
  s_groundOffset = (s_groundOffset + 2) % 6;

  // Bird bounding box (tightened for fair collision)
  constexpr int16_t birdX = 22;
  int16_t bLeft = birdX + 1;
  int16_t bRight = birdX + BITMAP_BIRD_WIDTH - 2;
  int16_t bTop = (int16_t)s_birdY + 1;
  int16_t bBottom = (int16_t)s_birdY + BITMAP_BIRD_HEIGHT - 1;

  // Update Pipes & Collision
  for (int i = 0; i < 2; i++) {
    s_pipes[i].x -= PIPE_SPEED;

    // Reset pipe once it scrolls off screen
    if (s_pipes[i].x + PIPE_WIDTH < 0) {
      s_pipes[i].x = 128.0f;
      // Pseudo-random gap position between 8 and 28
      s_pipes[i].gapY = 8 + (millis() % 21);
      s_pipes[i].passed = false;
    }

    // Score point when bird passes pipe center
    if (!s_pipes[i].passed && s_pipes[i].x + PIPE_WIDTH < birdX) {
      s_pipes[i].passed = true;
      s_score++;
    }

    // Check collision against pipe
    int16_t pLeft = (int16_t)s_pipes[i].x;
    int16_t pRight = pLeft + PIPE_WIDTH;

    if (bRight >= pLeft && bLeft <= pRight) {
      // Overlapping in X, check Y against top and bottom pipe
      if (bTop < s_pipes[i].gapY || bBottom > (s_pipes[i].gapY + PIPE_GAP)) {
        s_isGameOver = true;
        Engine::saveHighScore("FLAPPY C3", s_score);
      }
    }
  }

  // =========================================================================
  // RENDER GAME FRAME
  // =========================================================================
  u8g2.clearBuffer();

  // Draw Pipes (with strict screen boundaries)
  for (int i = 0; i < 2; i++) {
    int16_t px = (int16_t)s_pipes[i].x;
    if (px >= 0 && px < SCREEN_WIDTH) {
      int16_t drawW = (px + PIPE_WIDTH > SCREEN_WIDTH) ? (SCREEN_WIDTH - px) : PIPE_WIDTH;
      // Top Pipe
      if (s_pipes[i].gapY > 0) {
        u8g2.drawFrame(px, 0, drawW, s_pipes[i].gapY);
        u8g2.drawBox(px, s_pipes[i].gapY - 3, drawW, 3);
      }
      // Bottom Pipe
      int16_t bottomPipeY = s_pipes[i].gapY + PIPE_GAP;
      if (bottomPipeY < GROUND_Y) {
        u8g2.drawFrame(px, bottomPipeY, drawW, GROUND_Y - bottomPipeY);
        u8g2.drawBox(px, bottomPipeY, drawW, 3);
      }
    }
  }

  // Draw Bird (normal or wing up)
  const uint8_t* birdBmp = (s_wingTimer > 0) ? BITMAP_BIRD_WING_UP : BITMAP_BIRD;
  int16_t birdYDraw = (int16_t)s_birdY;
  if (birdYDraw < 0) birdYDraw = 0;
  u8g2.drawXBMP(birdX, birdYDraw, BITMAP_BIRD_WIDTH, BITMAP_BIRD_HEIGHT, birdBmp);

  // Draw Ground Line & Texture (no negative coords)
  u8g2.drawHLine(0, GROUND_Y, SCREEN_WIDTH);
  for (int x = 0; x < SCREEN_WIDTH; x += 6) {
    int16_t drawX = x - s_groundOffset;
    if (drawX >= 0 && drawX + 4 < SCREEN_WIDTH) {
      u8g2.drawLine(drawX, 63, drawX + 4, GROUND_Y + 1);
    }
  }

  // Draw HUD Score
  u8g2.setFont(u8g2_font_6x10_tf);
  char scoreStr[10];
  snprintf(scoreStr, sizeof(scoreStr), "%u", s_score);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 4, 10, scoreStr);

  u8g2.sendBuffer();
}

Minigame game = {
  "FLAPPY C3",
  init,
  update,
  BITMAP_BIRD,
  BITMAP_BIRD_WIDTH,
  BITMAP_BIRD_HEIGHT,
  "CLICK TO FLAP"
};

} // namespace GameFlappy
