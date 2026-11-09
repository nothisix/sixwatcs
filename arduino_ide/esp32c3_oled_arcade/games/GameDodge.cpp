#include "GameDodge.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameDodge {

struct FallingHazard {
  float x;
  float y;
  float speed;
  bool isCore; // true = bonus core, false = lethal meteor
  bool active;
};

static constexpr uint8_t SHIP_Y = 52;
static constexpr uint8_t NUM_HAZARDS = 4;

static float s_playerX = 58.0f;
static float s_playerDir = 2.2f;
static FallingHazard s_hazards[NUM_HAZARDS];

static uint32_t s_score = 0;
static uint16_t s_ticks = 0;
static bool s_isGameOver = false;

static void spawnHazard(uint8_t index) {
  s_hazards[index].x = 6.0f + (millis() % (SCREEN_WIDTH - 16));
  s_hazards[index].y = -(float)(millis() % 30);
  s_hazards[index].speed = 1.4f + ((millis() % 10) * 0.15f);
  s_hazards[index].isCore = ((millis() % 5) == 0); // 20% chance of collectible core
  s_hazards[index].active = true;
}

void init() {
  s_playerX = 58.0f;
  s_playerDir = 2.2f;
  s_score = 0;
  s_ticks = 0;
  s_isGameOver = false;

  for (uint8_t i = 0; i < NUM_HAZARDS; i++) {
    spawnHazard(i);
    s_hazards[i].y -= (i * 18.0f); // Stagger initial drop times
  }
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
    Display::drawCenteredStr(16, "SHIP ANNIHILATED!");

    char buf[24];
    snprintf(buf, sizeof(buf), "SURVIVAL: %lu", (unsigned long)s_score);
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(30, buf);

    uint32_t hi = Engine::getHighScore("METEOR STORM");
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
  // 3-BUTTON MOVEMENT CONTROLS
  // =========================================================================
  if (event == BTN_LONG_PRESS) {
    Engine::returnToMenu();
    return;
  }

  // 3-button manual dodge with auto-patrol fallback
  if (event == BTN_LEFT_CLICK || event == BTN_LEFT_HOLD) {
    s_playerX -= 3.0f;
  } else if (event == BTN_RIGHT_CLICK || event == BTN_RIGHT_HOLD) {
    s_playerX += 3.0f;
  } else if (event == BTN_CLICK) {
    // OK Click flips movement direction (1-button compat)
    s_playerDir = -s_playerDir;
    s_playerX += s_playerDir;
  } else {
    // Auto-patrol slide
    s_playerX += s_playerDir;
  }
  if (s_playerX < 2.0f) {
    s_playerX = 2.0f;
    s_playerDir = -s_playerDir;
  } else if (s_playerX > SCREEN_WIDTH - BITMAP_SHOOTER_WIDTH - 2.0f) {
    s_playerX = SCREEN_WIDTH - BITMAP_SHOOTER_WIDTH - 2.0f;
    s_playerDir = -s_playerDir;
  }

  // Score ticking
  s_ticks++;
  if (s_ticks % 10 == 0) {
    s_score++;
    Engine::saveHighScore("METEOR STORM", s_score);
  }

  // Update Hazards
  for (uint8_t i = 0; i < NUM_HAZARDS; i++) {
    s_hazards[i].y += s_hazards[i].speed;

    // Reset hazard when off bottom
    if (s_hazards[i].y > SCREEN_HEIGHT + 4.0f) {
      spawnHazard(i);
    }

    // Collision detection with player ship
    if (s_hazards[i].y >= SHIP_Y - 4.0f && s_hazards[i].y <= SHIP_Y + BITMAP_SHOOTER_HEIGHT) {
      float hx = s_hazards[i].x;
      if (hx + 4.0f >= s_playerX && hx <= s_playerX + BITMAP_SHOOTER_WIDTH) {
        if (s_hazards[i].isCore) {
          // Collected bonus energy core!
          s_score += 25;
          Engine::saveHighScore("METEOR STORM", s_score);
          spawnHazard(i);
        } else {
          // Lethal Meteor Crash!
          s_isGameOver = true;
          Engine::saveHighScore("METEOR STORM", s_score);
          return;
        }
      }
    }
  }

  // =========================================================================
  // RENDER GRAPHICS
  // =========================================================================
  u8g2.clearBuffer();

  // Draw Ground Line
  u8g2.drawHLine(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH);

  // Draw Warning indicators at top if hazard is approaching
  for (uint8_t i = 0; i < NUM_HAZARDS; i++) {
    if (s_hazards[i].y < 4.0f) {
      int16_t wx = (int16_t)s_hazards[i].x;
      if (s_hazards[i].isCore) {
        u8g2.drawPixel(wx, 1);
        u8g2.drawPixel(wx, 3);
      } else {
        u8g2.drawVLine(wx, 0, 3);
        u8g2.drawPixel(wx, 4);
      }
    }
  }

  // Draw Falling Hazards
  for (uint8_t i = 0; i < NUM_HAZARDS; i++) {
    if (s_hazards[i].y >= 0.0f && s_hazards[i].y <= SCREEN_HEIGHT) {
      int16_t hx = (int16_t)s_hazards[i].x;
      int16_t hy = (int16_t)s_hazards[i].y;

      if (s_hazards[i].isCore) {
        // Glowing Energy Crystal Diamond (collectible)
        u8g2.drawFrame(hx - 2, hy - 2, 5, 5);
        u8g2.drawPixel(hx, hy);
      } else {
        // Flaming Meteor
        u8g2.drawDisc(hx, hy, 2);
        u8g2.drawLine(hx, hy - 4, hx, hy - 2);
      }
    }
  }

  // Draw Player Ship
  u8g2.drawXBMP((int16_t)s_playerX, SHIP_Y, BITMAP_SHOOTER_WIDTH, BITMAP_SHOOTER_HEIGHT, BITMAP_SHOOTER);

  // HUD Score
  u8g2.setFont(u8g2_font_4x6_tf);
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)s_score);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 7, buf);

  u8g2.sendBuffer();
}

Minigame game = {
  "METEOR STORM",
  init,
  update,
  BITMAP_METEOR,
  BITMAP_METEOR_WIDTH,
  BITMAP_METEOR_HEIGHT,
  "CLICK TO FLIP DIRECTION"
};

} // namespace GameDodge
