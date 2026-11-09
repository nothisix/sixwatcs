#include "GameFrogger.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameFrogger {

struct Vehicle {
  float x;
  uint8_t width;
  float speed;
};

static constexpr uint8_t NUM_LANES = 4;
static constexpr uint8_t LANE_Y[NUM_LANES] = {42, 32, 22, 12};
static Vehicle s_cars[NUM_LANES][2]; // 2 cars per lane

static int8_t s_frogLane = 0; // 0 = start curb (Y=52), 1..4 = road lanes, 5 = goal lilypad (Y=2)
static uint32_t s_score = 0;
static uint16_t s_crossings = 0;
static bool s_isGameOver = false;

static void resetTraffic(float speedMult) {
  for (uint8_t l = 0; l < NUM_LANES; l++) {
    float dir = (l % 2 == 0) ? -1.0f : 1.0f;
    float baseSpeed = (1.2f + (l * 0.3f)) * speedMult * dir;

    s_cars[l][0].width = (l == 2) ? 16 : 10;
    s_cars[l][0].speed = baseSpeed;
    s_cars[l][0].x = (dir > 0) ? 0.0f : 120.0f;

    s_cars[l][1].width = (l == 2) ? 16 : 10;
    s_cars[l][1].speed = baseSpeed;
    s_cars[l][1].x = s_cars[l][0].x + (dir > 0 ? 65.0f : -65.0f);
  }
}

void init() {
  s_frogLane = 0;
  s_score = 0;
  s_crossings = 0;
  s_isGameOver = false;

  resetTraffic(1.0f);
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
    Display::drawCenteredStr(16, "FROG SQUASHED!");

    char buf[24];
    snprintf(buf, sizeof(buf), "CROSSED: %u TIMES", s_crossings);
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(30, buf);

    uint32_t hi = Engine::getHighScore("ROAD HOPPER");
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
  // 1-BUTTON HOPPING MECHANICS
  // =========================================================================
  if (event == BTN_LONG_PRESS) {
    Engine::returnToMenu();
    return;
  }

  // Click hops forward 1 lane
  if (event == BTN_CLICK) {
    s_frogLane++;
    s_score += 10;
    Engine::saveHighScore("ROAD HOPPER", s_score);

    // Reached top goal!
    if (s_frogLane >= 5) {
      s_crossings++;
      s_score += 50;
      Engine::saveHighScore("ROAD HOPPER", s_score);
      s_frogLane = 0; // Return to bottom curb for next crossing
      resetTraffic(1.0f + (s_crossings * 0.15f));
    }
  }

  // Update Traffic
  for (uint8_t l = 0; l < NUM_LANES; l++) {
    for (uint8_t c = 0; c < 2; c++) {
      s_cars[l][c].x += s_cars[l][c].speed;

      // Wrap around screen edges
      if (s_cars[l][c].speed > 0 && s_cars[l][c].x > SCREEN_WIDTH + 10.0f) {
        s_cars[l][c].x = -s_cars[l][c].width - 10.0f;
      } else if (s_cars[l][c].speed < 0 && s_cars[l][c].x < -s_cars[l][c].width - 10.0f) {
        s_cars[l][c].x = SCREEN_WIDTH + 10.0f;
      }
    }
  }

  // Collision Detection with Frog
  if (s_frogLane >= 1 && s_frogLane <= 4) {
    uint8_t currentLaneIdx = s_frogLane - 1; // 0..3
    int16_t frogLeft = 58;
    int16_t frogRight = 58 + BITMAP_FROG_WIDTH;

    for (uint8_t c = 0; c < 2; c++) {
      int16_t carLeft = (int16_t)s_cars[currentLaneIdx][c].x;
      int16_t carRight = carLeft + s_cars[currentLaneIdx][c].width;

      if (frogRight > carLeft && frogLeft < carRight) {
        s_isGameOver = true;
        Engine::saveHighScore("ROAD HOPPER", s_score);
        return;
      }
    }
  }

  // =========================================================================
  // RENDER GRAPHICS
  // =========================================================================
  u8g2.clearBuffer();

  // Top Goal Bank / Lilypad
  u8g2.drawBox(0, 0, SCREEN_WIDTH, 6);
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.setDrawColor(0);
  u8g2.drawStr(10, 5, "GOAL ZONE");
  u8g2.setDrawColor(1);

  // Road Lane Dividers (dashed lines)
  for (uint8_t l = 0; l < NUM_LANES; l++) {
    int16_t divY = LANE_Y[l] + 8;
    for (int x = 0; x < SCREEN_WIDTH; x += 12) {
      u8g2.drawHLine(x, divY, 6);
    }
  }

  // Bottom Safe Sidewalk
  u8g2.drawHLine(0, 50, SCREEN_WIDTH);

  // Draw Vehicles
  for (uint8_t l = 0; l < NUM_LANES; l++) {
    int16_t carY = LANE_Y[l];
    for (uint8_t c = 0; c < 2; c++) {
      int16_t cx = (int16_t)s_cars[l][c].x;
      if (cx > -20 && cx < SCREEN_WIDTH + 20) {
        u8g2.drawBox(cx, carY, s_cars[l][c].width, 7);
        u8g2.drawPixel(cx + 2, carY - 1);
        u8g2.drawPixel(cx + s_cars[l][c].width - 3, carY - 1);
      }
    }
  }

  // Draw Frog
  int16_t frogY = 52;
  if (s_frogLane >= 1 && s_frogLane <= 4) {
    frogY = LANE_Y[s_frogLane - 1] - 1;
  } else if (s_frogLane >= 5) {
    frogY = 1;
  }
  u8g2.drawXBMP(58, frogY, BITMAP_FROG_WIDTH, BITMAP_FROG_HEIGHT, BITMAP_FROG);

  // HUD Score
  u8g2.setFont(u8g2_font_4x6_tf);
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)s_score);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 58, buf);

  u8g2.sendBuffer();
}

Minigame game = {
  "ROAD HOPPER",
  init,
  update,
  BITMAP_FROG,
  BITMAP_FROG_WIDTH,
  BITMAP_FROG_HEIGHT,
  "CLICK TO HOP FORWARD"
};

} // namespace GameFrogger
