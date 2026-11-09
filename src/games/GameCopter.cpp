#include "GameCopter.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameCopter {

static constexpr uint8_t COPTER_X = 20;
static constexpr float GRAVITY = 0.22f;
static constexpr float LIFT_POWER = 0.42f;

static float s_copterY = 28.0f;
static float s_copterVy = 0.0f;
static uint32_t s_distance = 0;
static bool s_isGameOver = false;
static uint16_t s_holdFrames = 0;

// Cave terrain profile (18 slices, each 8px wide = 144px width)
static constexpr uint8_t NUM_SLICES = 18;
static uint8_t s_ceilY[NUM_SLICES];
static uint8_t s_floorY[NUM_SLICES];
static float s_scrollOffset = 0.0f;

// Floating cavern stalactite / obstacle pillar
static float s_pillarX = -20.0f;
static uint8_t s_pillarY = 25;
static uint8_t s_pillarH = 14;

void init() {
  s_copterY = 28.0f;
  s_copterVy = 0.0f;
  s_distance = 0;
  s_isGameOver = false;
  s_holdFrames = 0;
  s_scrollOffset = 0.0f;

  for (uint8_t i = 0; i < NUM_SLICES; i++) {
    s_ceilY[i] = 8;
    s_floorY[i] = 56;
  }

  s_pillarX = 160.0f;
  s_pillarY = 24;
  s_pillarH = 16;
}

void update(ButtonEvent event) {
  // =========================================================================
  // GAME OVER SCREEN
  // =========================================================================
  if (s_isGameOver) {
    if (event == BTN_CLICK) {
      init();
      return;
    } else if (event == BTN_LONG_PRESS) {
      Engine::returnToMenu();
      return;
    }

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    Display::drawCenteredStr(16, "COPTER DOWN!");

    char buf[24];
    snprintf(buf, sizeof(buf), "DIST: %lu m", (unsigned long)s_distance);
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(30, buf);

    uint32_t hi = Engine::getHighScore("CAVE COPTER");
    snprintf(buf, sizeof(buf), "BEST: %lu m", (unsigned long)hi);
    Display::drawCenteredStr(42, buf);

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawHLine(0, 52, SCREEN_WIDTH);
    u8g2.drawStr(6, 61, "CLICK: RETRY");
    Display::drawRightAlignedStr(SCREEN_WIDTH - 6, 61, "HOLD: MENU");

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // ACTIVE COPTER PHYSICS & CONTROLS
  // =========================================================================
  // Hold mid-flight for > 2.5s aborts to menu
  if (event == BTN_HOLDING || event == BTN_LONG_PRESS) {
    s_holdFrames++;
    if (s_holdFrames >= 100) {
      Engine::returnToMenu();
      return;
    }
  } else {
    s_holdFrames = 0;
  }

  // Lift vs Gravity
  if (event == BTN_HOLDING || event == BTN_CLICK || event == BTN_LONG_PRESS) {
    s_copterVy -= LIFT_POWER;
  }
  s_copterVy += GRAVITY;

  // Velocity clamp
  if (s_copterVy > 2.2f) s_copterVy = 2.2f;
  if (s_copterVy < -2.2f) s_copterVy = -2.2f;

  s_copterY += s_copterVy;

  // Terrain generation & scroll
  float speed = 2.0f + (s_distance > 500 ? 0.6f : (s_distance * 0.0012f));
  s_scrollOffset += speed;
  s_distance++;

  while (s_scrollOffset >= 8.0f) {
    s_scrollOffset -= 8.0f;

    // Shift terrain left
    for (uint8_t i = 0; i < NUM_SLICES - 1; i++) {
      s_ceilY[i] = s_ceilY[i + 1];
      s_floorY[i] = s_floorY[i + 1];
    }

    // Generate new random slice on right
    int8_t cDelta = (int8_t)((millis() % 7) - 3);
    int8_t fDelta = (int8_t)(((millis() / 3) % 7) - 3);

    int16_t newCeil = (int16_t)s_ceilY[NUM_SLICES - 2] + cDelta;
    if (newCeil < 4) newCeil = 4;
    if (newCeil > 20) newCeil = 20;
    s_ceilY[NUM_SLICES - 1] = (uint8_t)newCeil;

    int16_t newFloor = (int16_t)s_floorY[NUM_SLICES - 2] + fDelta;
    if (newFloor < 44) newFloor = 44;
    if (newFloor > 60) newFloor = 60;
    s_floorY[NUM_SLICES - 1] = (uint8_t)newFloor;
  }

  // Move Pillar Obstacle
  s_pillarX -= speed;
  if (s_pillarX < -15.0f) {
    s_pillarX = SCREEN_WIDTH + 30.0f + (millis() % 80);
    s_pillarY = 18 + (millis() % 16);
    s_pillarH = 14 + (millis() % 10);
  }

  // =========================================================================
  // COLLISION DETECTION
  // =========================================================================
  uint8_t sliceIdx = (COPTER_X + (uint8_t)s_scrollOffset) / 8;
  if (sliceIdx >= NUM_SLICES) sliceIdx = NUM_SLICES - 1;

  uint8_t currentCeil = s_ceilY[sliceIdx];
  uint8_t currentFloor = s_floorY[sliceIdx];

  // Check ceiling and floor crash
  if (s_copterY <= currentCeil || (s_copterY + BITMAP_COPTER_HEIGHT) >= currentFloor) {
    s_isGameOver = true;
    Engine::saveHighScore("CAVE COPTER", s_distance);
  }

  // Check pillar collision
  int16_t pX = (int16_t)s_pillarX;
  if (pX < (COPTER_X + BITMAP_COPTER_WIDTH) && (pX + 6) > COPTER_X) {
    if (s_copterY < (s_pillarY + s_pillarH) && (s_copterY + BITMAP_COPTER_HEIGHT) > s_pillarY) {
      s_isGameOver = true;
      Engine::saveHighScore("CAVE COPTER", s_distance);
    }
  }

  // =========================================================================
  // RENDER GRAPHICS
  // =========================================================================
  u8g2.clearBuffer();

  // Draw Cave Ceiling & Floor
  int16_t baseShift = (int16_t)s_scrollOffset;
  for (uint8_t i = 0; i < NUM_SLICES - 1; i++) {
    int16_t x1 = (i * 8) - baseShift;
    int16_t x2 = ((i + 1) * 8) - baseShift;

    // Draw ceiling line
    u8g2.drawLine(x1, s_ceilY[i], x2, s_ceilY[i + 1]);
    // Draw floor line
    u8g2.drawLine(x1, s_floorY[i], x2, s_floorY[i + 1]);

    // Fill solid rock outside cave
    if (s_ceilY[i] > 1) {
      u8g2.drawVLine(x1, 0, s_ceilY[i]);
    }
    if (s_floorY[i] < SCREEN_HEIGHT - 1) {
      u8g2.drawVLine(x1, s_floorY[i], SCREEN_HEIGHT - s_floorY[i]);
    }
  }

  // Draw Floating Cavern Pillar
  if (pX >= -10 && pX <= SCREEN_WIDTH) {
    u8g2.drawBox(pX, s_pillarY, 6, s_pillarH);
    u8g2.drawFrame(pX - 1, s_pillarY - 1, 8, s_pillarH + 2);
  }

  // Draw Smoke Particles / Engine Flame behind Copter
  if (COPTER_X > 6) {
    int16_t flameY = (int16_t)s_copterY + 6;
    if ((s_distance % 2) == 0) {
      u8g2.drawPixel(COPTER_X - 2, flameY);
      u8g2.drawPixel(COPTER_X - 5, flameY + 1);
    } else {
      u8g2.drawPixel(COPTER_X - 3, flameY);
      u8g2.drawPixel(COPTER_X - 6, flameY - 1);
    }
  }

  // Draw Copter Sprite
  int16_t drawY = (int16_t)s_copterY;
  if (drawY < 0) drawY = 0;
  if (drawY > SCREEN_HEIGHT - BITMAP_COPTER_HEIGHT) drawY = SCREEN_HEIGHT - BITMAP_COPTER_HEIGHT;
  u8g2.drawXBMP(COPTER_X, drawY, BITMAP_COPTER_WIDTH, BITMAP_COPTER_HEIGHT, BITMAP_COPTER);

  // HUD: Distance at Top-Right
  u8g2.setFont(u8g2_font_4x6_tf);
  char distBuf[16];
  snprintf(distBuf, sizeof(distBuf), "%lum", (unsigned long)s_distance);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 6, distBuf);

  u8g2.sendBuffer();
}

Minigame game = {
  "CAVE COPTER",
  init,
  update,
  BITMAP_COPTER,
  BITMAP_COPTER_WIDTH,
  BITMAP_COPTER_HEIGHT,
  "HOLD TO FLY"
};

} // namespace GameCopter
