#include "GameLander.h"
#include "Display.h"
#include "Bitmaps.h"
#include "Engine.h"

namespace GameLander {

enum LanderState {
  STATE_FLYING,
  STATE_LANDED,
  STATE_CRASHED
};

static constexpr float GRAVITY = 0.045f;
static constexpr float THRUST_POWER = 0.11f;
static constexpr float SAFE_VELOCITY = 1.35f; // Forgiving & realistic soft landing speed
static constexpr float MAX_FUEL = 100.0f;
static constexpr uint8_t SURFACE_Y = 54;

static float s_posX = 20.0f;
static float s_posY = 4.0f;
static float s_velX = 0.4f;
static float s_velY = 0.0f;
static float s_fuel = MAX_FUEL;
static float s_lastTouchSpeed = 0.0f;

static int16_t s_padX = 50;
static uint8_t s_padW = 34;

static uint16_t s_score = 0;
static LanderState s_state = STATE_FLYING;
static bool s_thrusterActive = false;
static const char* s_endReason = "";
static uint16_t s_continuousHoldFrames = 0;

void init() {
  s_posX = 10.0f + (millis() % 20);
  s_posY = 2.0f;
  s_velX = 0.35f;
  s_velY = 0.0f;
  s_fuel = MAX_FUEL;
  s_lastTouchSpeed = 0.0f;

  // Position landing pad in playable bounds
  s_padX = 30 + (millis() % 50);
  s_padW = 34;

  s_state = STATE_FLYING;
  s_thrusterActive = false;
  s_endReason = "";
  s_continuousHoldFrames = 0;
}

void update(ButtonEvent event) {
  // =========================================================================
  // ROUND END (CRASHED OR LANDED)
  // =========================================================================
  if (s_state != STATE_FLYING) {
    if (event == BTN_CLICK) {
      if (s_state == STATE_LANDED) {
        // Next landing attempt with carried score
        s_posX = 10.0f + (millis() % 20);
        s_posY = 2.0f;
        s_velX = 0.35f + (s_score * 0.015f);
        s_velY = 0.0f;
        s_fuel = MAX_FUEL;
        s_padX = 35 + (millis() % 55);
        s_padW = (s_padW > 18) ? s_padW - 2 : 18;
        s_state = STATE_FLYING;
        s_thrusterActive = false;
        s_continuousHoldFrames = 0;
        return;
      } else {
        // Full restart after crash
        s_score = 0;
        init();
        return;
      }
    } else if (event == BTN_LONG_PRESS) {
      // Exit cleanly to carousel menu
      Engine::returnToMenu();
      return;
    }

    // Render Round End Overlay
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    if (s_state == STATE_LANDED) {
      Display::drawCenteredStr(15, "TOUCHDOWN SUCCESS!");
    } else {
      Display::drawCenteredStr(15, "LANDER CRASHED!");
    }

    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(26, s_endReason);

    char speedBuf[24];
    snprintf(speedBuf, sizeof(speedBuf), "V:%0.1f (SAFE MAX:%0.1f)", s_lastTouchSpeed * 10.0f, SAFE_VELOCITY * 10.0f);
    Display::drawCenteredStr(36, speedBuf);

    char scoreBuf[24];
    uint32_t hi = Engine::getHighScore("MOON LANDER");
    snprintf(scoreBuf, sizeof(scoreBuf), "SCORE: %u  HI: %lu", s_score, (unsigned long)hi);
    Display::drawCenteredStr(47, scoreBuf);

    u8g2.setFont(u8g2_font_4x6_tf);
    u8g2.drawHLine(0, 52, SCREEN_WIDTH);
    u8g2.drawStr(6, 61, "CLICK: NEXT");
    Display::drawRightAlignedStr(SCREEN_WIDTH - 6, 61, "HOLD: MENU");

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // ACTIVE SIMULATION / INPUT (VIRTUAL MULTIPLEXING)
  // =========================================================================
  s_thrusterActive = false;

  // Track hold duration while flying: holding for > 2.5s (100 frames @ 40fps) aborts mid-flight
  if (event == BTN_HOLDING || event == BTN_LONG_PRESS) {
    s_continuousHoldFrames++;
    if (s_continuousHoldFrames >= 100) {
      Engine::returnToMenu();
      return;
    }
  } else {
    s_continuousHoldFrames = 0;
  }

  // BTN_HOLDING, BTN_LONG_PRESS or BTN_CLICK fires rocket thruster!
  if ((event == BTN_HOLDING || event == BTN_CLICK || event == BTN_LONG_PRESS) && s_fuel > 0.0f) {
    s_velY -= THRUST_POWER;
    s_fuel -= 0.35f;
    if (s_fuel < 0.0f) s_fuel = 0.0f;
    s_thrusterActive = true;
  }

  // Gravity & Physics
  s_velY += GRAVITY;
  s_posY += s_velY;
  s_posX += s_velX;

  // Horizontal screen bounce
  if (s_posX < 2.0f) {
    s_posX = 2.0f;
    s_velX = -s_velX * 0.6f;
  } else if (s_posX + BITMAP_LANDER_WIDTH > SCREEN_WIDTH - 2) {
    s_posX = SCREEN_WIDTH - 2 - BITMAP_LANDER_WIDTH;
    s_velX = -s_velX * 0.6f;
  }

  // Touchdown Detection on Lunar Surface
  float landerBottomY = s_posY + BITMAP_LANDER_HEIGHT;
  if (landerBottomY >= SURFACE_Y) {
    s_posY = SURFACE_Y - BITMAP_LANDER_HEIGHT;
    s_lastTouchSpeed = s_velY;

    // Check if lander is safely over the landing pad (center of lander on pad)
    float landerCenter = s_posX + (BITMAP_LANDER_WIDTH / 2.0f);
    bool onPad = (landerCenter >= s_padX - 2.0f) && (landerCenter <= (s_padX + s_padW + 2.0f));

    if (onPad) {
      if (s_velY <= SAFE_VELOCITY) {
        // Perfect Landing!
        s_state = STATE_LANDED;
        s_endReason = "PERFECT SOFT LANDING!";
        uint16_t roundPoints = 100 + (uint16_t)s_fuel;
        s_score += roundPoints;
        Engine::saveHighScore("MOON LANDER", s_score);
      } else {
        s_state = STATE_CRASHED;
        s_endReason = "CRASH: DESCENT TOO FAST";
        Engine::saveHighScore("MOON LANDER", s_score);
      }
    } else {
      s_state = STATE_CRASHED;
      s_endReason = "CRASH: MISSED LANDING PAD";
      Engine::saveHighScore("MOON LANDER", s_score);
    }
  }

  // =========================================================================
  // RENDER GRAPHICS & HUD
  // =========================================================================
  u8g2.clearBuffer();

  // Draw Fuel Gauge
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(2, 6, "FUEL");
  Display::drawProgressBar(20, 1, 36, 6, s_fuel / MAX_FUEL);

  // Draw Descent Speed Meter
  char vBuf[16];
  snprintf(vBuf, sizeof(vBuf), "V:%0.1f", s_velY * 10.0f);
  Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 6, vBuf);
  // Indicator dot: solid if safe descent speed, hollow circle if too fast
  if (s_velY <= SAFE_VELOCITY) {
    u8g2.drawDisc(SCREEN_WIDTH - 38, 3, 2);
  } else {
    u8g2.drawCircle(SCREEN_WIDTH - 38, 3, 2);
  }

  // Draw Rough Terrain
  u8g2.drawLine(0, SURFACE_Y + 4, s_padX, SURFACE_Y + 4);
  u8g2.drawLine(s_padX + s_padW, SURFACE_Y + 4, SCREEN_WIDTH, SURFACE_Y + 4);
  // Crater bumps
  u8g2.drawPixel(10, SURFACE_Y + 6);
  u8g2.drawPixel(25, SURFACE_Y + 8);
  u8g2.drawPixel(110, SURFACE_Y + 7);

  // Draw Landing Pad (thick elevated platform)
  u8g2.drawBox(s_padX, SURFACE_Y, s_padW, 3);
  u8g2.drawLine(s_padX + 2, SURFACE_Y + 3, s_padX, SURFACE_Y + 7);
  u8g2.drawLine(s_padX + s_padW - 3, SURFACE_Y + 3, s_padX + s_padW - 1, SURFACE_Y + 7);

  // Draw Lander Sprite (clamped safely within screen)
  int16_t drawX = (int16_t)s_posX;
  int16_t drawY = (int16_t)s_posY;
  if (drawX < 0) drawX = 0;
  if (drawX > SCREEN_WIDTH - BITMAP_LANDER_WIDTH) drawX = SCREEN_WIDTH - BITMAP_LANDER_WIDTH;
  if (drawY < 0) drawY = 0;
  u8g2.drawXBMP(drawX, drawY, BITMAP_LANDER_WIDTH, BITMAP_LANDER_HEIGHT, BITMAP_LANDER);

  // Draw Thruster Flame when active
  if (s_thrusterActive && drawY + BITMAP_LANDER_HEIGHT + BITMAP_FLAME_HEIGHT <= SCREEN_HEIGHT) {
    int16_t flameX = drawX + (BITMAP_LANDER_WIDTH - BITMAP_FLAME_WIDTH) / 2;
    int16_t flameY = drawY + BITMAP_LANDER_HEIGHT;
    if (flameX >= 0 && flameX + BITMAP_FLAME_WIDTH <= SCREEN_WIDTH) {
      u8g2.drawXBMP(flameX, flameY, BITMAP_FLAME_WIDTH, BITMAP_FLAME_HEIGHT, BITMAP_FLAME);
    }
  }

  u8g2.sendBuffer();
}

Minigame game = {
  "MOON LANDER",
  init,
  update,
  BITMAP_LANDER,
  BITMAP_LANDER_WIDTH,
  BITMAP_LANDER_HEIGHT,
  "HOLD THRUSTER"
};

} // namespace GameLander
