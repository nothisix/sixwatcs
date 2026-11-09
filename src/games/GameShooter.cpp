#include "GameShooter.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameShooter {

struct Missile {
  float x;
  float y;
  bool active;
};

struct Invader {
  float x;
  float y;
  bool alive;
};

static constexpr uint8_t MAX_MISSILES = 4;
static constexpr uint8_t NUM_INVADERS = 6;
static constexpr uint8_t SHIP_Y = 52;

static float s_shipX = 58.0f;
static float s_shipDir = 1.6f;
static Missile s_missiles[MAX_MISSILES];
static Invader s_invaders[NUM_INVADERS];
static float s_invaderDir = 0.8f;
static float s_invaderStepDown = 0.0f;

static uint32_t s_score = 0;
static uint8_t s_wave = 1;
static bool s_isGameOver = false;

static void spawnWave() {
  for (uint8_t i = 0; i < NUM_INVADERS; i++) {
    s_invaders[i].x = 12.0f + (i * 18.0f);
    s_invaders[i].y = 12.0f;
    s_invaders[i].alive = true;
  }
  s_invaderDir = 0.7f + (s_wave * 0.15f);
}

void init() {
  s_shipX = 58.0f;
  s_shipDir = 1.6f;
  s_score = 0;
  s_wave = 1;
  s_isGameOver = false;

  for (uint8_t i = 0; i < MAX_MISSILES; i++) {
    s_missiles[i].active = false;
  }

  spawnWave();
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
    Display::drawCenteredStr(16, "EARTH CONQUERED!");

    char buf[24];
    snprintf(buf, sizeof(buf), "SCORE: %lu", (unsigned long)s_score);
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(30, buf);

    uint32_t hi = Engine::getHighScore("SPACE BLAST");
    snprintf(buf, sizeof(buf), "BEST:  %lu", (unsigned long)hi);
    Display::drawCenteredStr(42, buf);

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawHLine(0, 52, SCREEN_WIDTH);
    u8g2.drawStr(6, 61, "CLICK: RETRY");
    Display::drawRightAlignedStr(SCREEN_WIDTH - 6, 61, "HOLD: MENU");

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // SHIP & MISSILE LOGIC
  // =========================================================================
  if (event == BTN_LONG_PRESS) {
    Engine::returnToMenu();
    return;
  }

  // 3-Button Ship movement: Left/Right manual control with auto-sweep fallback
  if (event == BTN_LEFT_CLICK || event == BTN_LEFT_HOLD) {
    s_shipX -= 3.0f;
  } else if (event == BTN_RIGHT_CLICK || event == BTN_RIGHT_HOLD) {
    s_shipX += 3.0f;
  } else {
    // Auto-sweep ship horizontally (1-button compat)
    s_shipX += s_shipDir;
  }

  if (s_shipX < 4.0f) {
    s_shipX = 4.0f;
    s_shipDir = -s_shipDir;
  } else if (s_shipX > SCREEN_WIDTH - BITMAP_SHOOTER_WIDTH - 4.0f) {
    s_shipX = SCREEN_WIDTH - BITMAP_SHOOTER_WIDTH - 4.0f;
    s_shipDir = -s_shipDir;
  }

  // Button Click fires missile (OK click)
  if (event == BTN_CLICK) {
    for (uint8_t i = 0; i < MAX_MISSILES; i++) {
      if (!s_missiles[i].active) {
        s_missiles[i].x = s_shipX + (BITMAP_SHOOTER_WIDTH / 2.0f);
        s_missiles[i].y = SHIP_Y - 2.0f;
        s_missiles[i].active = true;
        break;
      }
    }
  }

  // Move Missiles upward
  for (uint8_t i = 0; i < MAX_MISSILES; i++) {
    if (s_missiles[i].active) {
      s_missiles[i].y -= 4.0f;
      if (s_missiles[i].y < 2.0f) {
        s_missiles[i].active = false;
      }
    }
  }

  // Move Invaders
  bool hitEdge = false;
  uint8_t aliveCount = 0;

  for (uint8_t i = 0; i < NUM_INVADERS; i++) {
    if (s_invaders[i].alive) {
      aliveCount++;
      s_invaders[i].x += s_invaderDir;
      if (s_invaders[i].x < 4.0f || s_invaders[i].x > SCREEN_WIDTH - 14.0f) {
        hitEdge = true;
      }

      // Check if invaders reach defense line
      if (s_invaders[i].y >= SHIP_Y - 8.0f) {
        s_isGameOver = true;
        Engine::saveHighScore("SPACE BLAST", s_score);
      }
    }
  }

  // When edge is hit, bounce and descend
  if (hitEdge) {
    s_invaderDir = -s_invaderDir;
    for (uint8_t i = 0; i < NUM_INVADERS; i++) {
      s_invaders[i].y += 3.5f;
    }
  }

  // Check if wave cleared
  if (aliveCount == 0) {
    s_wave++;
    s_score += 50;
    spawnWave();
  }

  // Collision: Missile vs Invader
  for (uint8_t m = 0; m < MAX_MISSILES; m++) {
    if (!s_missiles[m].active) continue;

    for (uint8_t inv = 0; inv < NUM_INVADERS; inv++) {
      if (!s_invaders[inv].alive) continue;

      float ix = s_invaders[inv].x;
      float iy = s_invaders[inv].y;

      // Invader bounding box (10x8)
      if (s_missiles[m].x >= ix && s_missiles[m].x <= (ix + 10.0f) &&
          s_missiles[m].y >= iy && s_missiles[m].y <= (iy + 8.0f)) {
        s_invaders[inv].alive = false;
        s_missiles[m].active = false;
        s_score += 10;
        Engine::saveHighScore("SPACE BLAST", s_score);
        break;
      }
    }
  }

  // =========================================================================
  // RENDER GRAPHICS
  // =========================================================================
  u8g2.clearBuffer();

  // Distant twinkling stars
  u8g2.drawPixel(15, 6);
  u8g2.drawPixel(45, 14);
  u8g2.drawPixel(85, 8);
  u8g2.drawPixel(112, 18);

  // Draw Defense line
  u8g2.drawHLine(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH);

  // Draw Invaders
  for (uint8_t i = 0; i < NUM_INVADERS; i++) {
    if (s_invaders[i].alive) {
      int16_t ix = (int16_t)s_invaders[i].x;
      int16_t iy = (int16_t)s_invaders[i].y;
      // Draw pixel-art mini invader (8x6)
      u8g2.drawFrame(ix, iy, 8, 6);
      u8g2.drawPixel(ix + 2, iy + 2);
      u8g2.drawPixel(ix + 5, iy + 2);
      u8g2.drawPixel(ix + 1, iy + 5);
      u8g2.drawPixel(ix + 6, iy + 5);
    }
  }

  // Draw Missiles
  for (uint8_t i = 0; i < MAX_MISSILES; i++) {
    if (s_missiles[i].active) {
      u8g2.drawVLine((int16_t)s_missiles[i].x, (int16_t)s_missiles[i].y, 4);
    }
  }

  // Draw Player Ship
  u8g2.drawXBMP((int16_t)s_shipX, SHIP_Y, BITMAP_SHOOTER_WIDTH, BITMAP_SHOOTER_HEIGHT, BITMAP_SHOOTER);

  // HUD: Wave & Score
  u8g2.setFont(u8g2_font_4x6_tf);
  char hudBuf[24];
  snprintf(hudBuf, sizeof(hudBuf), "W%u  %lu", s_wave, (unsigned long)s_score);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 6, hudBuf);

  u8g2.sendBuffer();
}

Minigame game = {
  "SPACE BLAST",
  init,
  update,
  BITMAP_SHOOTER,
  BITMAP_SHOOTER_WIDTH,
  BITMAP_SHOOTER_HEIGHT,
  "CLICK TO SHOOT"
};

} // namespace GameShooter
