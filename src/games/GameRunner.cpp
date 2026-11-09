#include "GameRunner.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameRunner {

struct Obstacle {
  float x;
  uint8_t width;
  uint8_t height;
  bool isFlying;
};

static constexpr uint8_t GROUND_Y = 54;
static constexpr float GRAVITY = 0.38f;
static constexpr float JUMP_IMPULSE = -4.0f;
static constexpr int16_t RUNNER_X = 18;

static float s_runnerY = GROUND_Y - BITMAP_RUNNER_HEIGHT;
static float s_runnerVy = 0.0f;
static bool s_isGrounded = true;

static Obstacle s_obstacles[2];
static float s_worldSpeed = 2.2f;
static uint32_t s_score = 0;
static bool s_isGameOver = false;
static uint8_t s_animFrame = 0;
static uint8_t s_tick = 0;

void init() {
  s_runnerY = GROUND_Y - BITMAP_RUNNER_HEIGHT;
  s_runnerVy = 0.0f;
  s_isGrounded = true;

  s_worldSpeed = 2.2f;
  s_score = 0;
  s_isGameOver = false;
  s_animFrame = 0;
  s_tick = 0;

  s_obstacles[0].x = 130.0f;
  s_obstacles[0].width = 8;
  s_obstacles[0].height = 10;
  s_obstacles[0].isFlying = false;

  s_obstacles[1].x = 130.0f + 80.0f;
  s_obstacles[1].width = 10;
  s_obstacles[1].height = 12;
  s_obstacles[1].isFlying = false;
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
    Display::drawCenteredStr(16, "SYSTEM CRASHED");

    char buf[24];
    snprintf(buf, sizeof(buf), "DIST: %lu m", (unsigned long)s_score);
    Display::drawCenteredStr(29, buf);

    uint32_t hi = Engine::getHighScore("CYBER RUN");
    snprintf(buf, sizeof(buf), "RECORD: %lu m", (unsigned long)hi);
    Display::drawCenteredStr(41, buf);

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawHLine(0, 50, SCREEN_WIDTH);
    u8g2.drawStr(6, 60, "CLICK: RETRY");
    Display::drawRightAlignedStr(SCREEN_WIDTH - 6, 60, "HOLD: MENU");

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // ACTIVE RUNNER LOGIC
  // =========================================================================

  // Handle Jump Input
  if (event == BTN_CLICK && s_isGrounded) {
    s_runnerVy = JUMP_IMPULSE;
    s_isGrounded = false;
  } else if (event == BTN_LONG_PRESS) {
    Engine::returnToMenu();
    return;
  }

  // Physics simulation
  if (!s_isGrounded) {
    s_runnerVy += GRAVITY;
    s_runnerY += s_runnerVy;

    if (s_runnerY >= GROUND_Y - BITMAP_RUNNER_HEIGHT) {
      s_runnerY = GROUND_Y - BITMAP_RUNNER_HEIGHT;
      s_runnerVy = 0.0f;
      s_isGrounded = true;
    }
  }

  // Animate runner legs when grounded
  s_tick++;
  if (s_tick % 4 == 0) {
    s_animFrame = 1 - s_animFrame;
  }

  // Increase score and gradually increase world speed
  s_score++;
  if (s_score % 100 == 0 && s_worldSpeed < 4.5f) {
    s_worldSpeed += 0.15f;
  }

  // Runner bounding box
  int16_t rLeft = RUNNER_X + 2;
  int16_t rRight = RUNNER_X + BITMAP_RUNNER_WIDTH - 2;
  int16_t rTop = (int16_t)s_runnerY + 1;
  int16_t rBottom = (int16_t)s_runnerY + BITMAP_RUNNER_HEIGHT - 1;

  // Move & Recycle Obstacles
  for (int i = 0; i < 2; i++) {
    s_obstacles[i].x -= s_worldSpeed;

    // Respawn off-screen obstacle
    if (s_obstacles[i].x + s_obstacles[i].width < 0) {
      float otherX = (i == 0) ? s_obstacles[1].x : s_obstacles[0].x;
      s_obstacles[i].x = otherX + 70.0f + (millis() % 40);

      // 25% chance of low drone
      bool fly = ((millis() % 4) == 0) && (s_score > 150);
      s_obstacles[i].isFlying = fly;
      s_obstacles[i].width = fly ? 8 : (6 + (millis() % 6));
      s_obstacles[i].height = fly ? 6 : (8 + (millis() % 6));
    }

    // Calculate obstacle box
    int16_t oLeft = (int16_t)s_obstacles[i].x;
    int16_t oRight = oLeft + s_obstacles[i].width;
    int16_t oTop = s_obstacles[i].isFlying ? (GROUND_Y - 20) : (GROUND_Y - s_obstacles[i].height);
    int16_t oBottom = s_obstacles[i].isFlying ? (GROUND_Y - 20 + s_obstacles[i].height) : GROUND_Y;

    // Check collision AABB
    if (rRight >= oLeft && rLeft <= oRight && rBottom >= oTop && rTop <= oBottom) {
      s_isGameOver = true;
      Engine::saveHighScore("CYBER RUN", s_score);
    }
  }

  // =========================================================================
  // RENDER GRAPHICS
  // =========================================================================
  u8g2.clearBuffer();

  // 1. Distant Moon and Cyber Stars (Atmosphere in upper screen)
  u8g2.drawCircle(22, 12, 5);
  u8g2.setDrawColor(0);
  u8g2.drawDisc(25, 10, 4);
  u8g2.setDrawColor(1);
  u8g2.drawPixel(50, 8);
  u8g2.drawPixel(82, 16);
  u8g2.drawPixel(105, 6);

  // 2. Distant Cyber City Skyline Parallax (Wireframe outlines with window dots & antennas)
  int16_t bgShift = (s_score / 2) % 36;
  for (int bx = -36; bx < SCREEN_WIDTH + 36; bx += 36) {
    int16_t drawBx = bx - bgShift;

    // Building 1: Tall wireframe tower
    int16_t b1X = drawBx;
    int16_t b1W = 14;
    int16_t b1H = 22; // Y from 32 to 54
    int16_t b1Y = GROUND_Y - b1H;
    if (b1X + b1W >= 0 && b1X < SCREEN_WIDTH) {
      u8g2.drawFrame(b1X, b1Y, b1W, b1H);
      // Antenna on roof
      u8g2.drawLine(b1X + b1W / 2, b1Y - 4, b1X + b1W / 2, b1Y);
      u8g2.drawPixel(b1X + b1W / 2, b1Y - 5);
      // Windows
      u8g2.drawPixel(b1X + 3, b1Y + 5);
      u8g2.drawPixel(b1X + 7, b1Y + 5);
      u8g2.drawPixel(b1X + 10, b1Y + 5);
      u8g2.drawPixel(b1X + 3, b1Y + 11);
      u8g2.drawPixel(b1X + 7, b1Y + 11);
      u8g2.drawPixel(b1X + 10, b1Y + 11);
    }

    // Building 2: Mid-height wireframe building
    int16_t b2X = drawBx + 18;
    int16_t b2W = 12;
    int16_t b2H = 15; // Y from 39 to 54
    int16_t b2Y = GROUND_Y - b2H;
    if (b2X + b2W >= 0 && b2X < SCREEN_WIDTH) {
      u8g2.drawFrame(b2X, b2Y, b2W, b2H);
      // Windows
      u8g2.drawPixel(b2X + 3, b2Y + 4);
      u8g2.drawPixel(b2X + 8, b2Y + 4);
      u8g2.drawPixel(b2X + 3, b2Y + 8);
      u8g2.drawPixel(b2X + 8, b2Y + 8);
    }
  }

  // 3. Ground Line & Scrolling hashes
  u8g2.drawHLine(0, GROUND_Y, SCREEN_WIDTH);
  int16_t groundShift = (int16_t)(s_score * 2) % 8;
  for (int gx = 0; gx < SCREEN_WIDTH; gx += 8) {
    int16_t drawGx = gx - groundShift;
    if (drawGx >= 0 && drawGx < SCREEN_WIDTH) {
      u8g2.drawPixel(drawGx, GROUND_Y + 3);
      if (drawGx + 2 < SCREEN_WIDTH) {
        u8g2.drawPixel(drawGx + 2, GROUND_Y + 5);
      }
    }
  }

  // 4. Draw Obstacles with High-Contrast Black Halo / Cutout
  for (int i = 0; i < 2; i++) {
    int16_t ox = (int16_t)s_obstacles[i].x;
    if (ox >= -15 && ox <= SCREEN_WIDTH) {
      if (s_obstacles[i].isFlying) {
        // --- Flying Drone ---
        int16_t droneY = GROUND_Y - 20; // Y = 34
        int16_t dW = s_obstacles[i].width;

        // Clear a 2px black mask around drone so it NEVER blends with background
        u8g2.setDrawColor(0);
        u8g2.drawBox(ox - 2, droneY - 3, dW + 4, 11);
        u8g2.setDrawColor(1);

        // Drone main fuselage
        u8g2.drawBox(ox + 1, droneY + 1, dW - 2, 4);
        // Drone frame
        u8g2.drawFrame(ox, droneY, dW, 5);
        // Spinning rotor blades animation
        if ((s_tick / 2) % 2 == 0) {
          u8g2.drawHLine(ox - 1, droneY - 1, dW + 2);
        } else {
          u8g2.drawPixel(ox, droneY - 1);
          u8g2.drawPixel(ox + dW - 1, droneY - 1);
        }
        // Center sensor eye (cutout dot)
        u8g2.setDrawColor(0);
        u8g2.drawPixel(ox + dW / 2, droneY + 2);
        u8g2.setDrawColor(1);

      } else {
        // --- Ground Spikes / Hazard Barrier ---
        int16_t bH = s_obstacles[i].height;
        int16_t bW = s_obstacles[i].width;

        // Clear a 2px black mask around spike
        u8g2.setDrawColor(0);
        u8g2.drawBox(ox - 2, GROUND_Y - bH - 2, bW + 4, bH + 3);
        u8g2.setDrawColor(1);

        // Solid bright white spikes
        u8g2.drawTriangle(ox, GROUND_Y, ox + bW / 2, GROUND_Y - bH, ox + bW, GROUND_Y);
        // Base line
        u8g2.drawHLine(ox - 1, GROUND_Y, bW + 2);
      }
    }
  }

  // 5. Draw Runner Sprite with clean black mask
  u8g2.setDrawColor(0);
  u8g2.drawBox(RUNNER_X - 1, (int16_t)s_runnerY - 1, BITMAP_RUNNER_WIDTH + 2, BITMAP_RUNNER_HEIGHT + 2);
  u8g2.setDrawColor(1);

  const uint8_t* runnerBmp = (s_animFrame == 0 || !s_isGrounded) ? BITMAP_RUNNER_1 : BITMAP_RUNNER_2;
  u8g2.drawXBMP(RUNNER_X, (int16_t)s_runnerY, BITMAP_RUNNER_WIDTH, BITMAP_RUNNER_HEIGHT, runnerBmp);

  // 6. Draw HUD Distance
  u8g2.setFont(u8g2_font_5x7_tf);
  char distBuf[16];
  snprintf(distBuf, sizeof(distBuf), "%lum", (unsigned long)s_score);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 8, distBuf);

  u8g2.sendBuffer();
}

Minigame game = {
  "CYBER RUN",
  init,
  update,
  BITMAP_RUNNER_1,
  BITMAP_RUNNER_WIDTH,
  BITMAP_RUNNER_HEIGHT,
  "CLICK TO JUMP"
};

} // namespace GameRunner
