#include <Arduino.h>
#include "Config.h"
#include "ButtonHandler.h"
#include "Display.h"
#include "Engine.h"
#include "Bitmaps.h"
#include "ClockManager.h"
#include "MusicManager.h"

// Include Minigames
#include "games/GameFlappy.h"
#include "games/GameLander.h"
#include "games/GameRunner.h"
#include "games/GameCopter.h"
#include "games/GameShooter.h"
#include "games/GameStacker.h"
#include "games/GameSnake.h"
#include "games/GamePong.h"
#include "games/GameFrogger.h"
#include "games/GameDodge.h"

// Hardware instances: 3-Button Controller (Left=GPIO 0, OK=GPIO 3, Right=GPIO 1)
static InputManager inputManager(PIN_BTN_LEFT, PIN_BTN_OK, PIN_BTN_RIGHT);
static Engine engine;

// Non-blocking frame timing
static unsigned long lastFrameTime = 0;
static ButtonEvent latchedEvent = BTN_NONE;

// Retro Watch Carousel Card
static Minigame gameWatch = {
  "RETRO WATCH",
  nullptr,
  nullptr,
  BITMAP_WATCH,
  BITMAP_WATCH_WIDTH,
  BITMAP_WATCH_HEIGHT,
  "BLE TIME SYNC"
};

// Google Maps Navigation Carousel Card
static Minigame gameNav = {
  "GMAPS NAV",
  nullptr,
  nullptr,
  BITMAP_NAV,
  BITMAP_NAV_WIDTH,
  BITMAP_NAV_HEIGHT,
  "TURN-BY-TURN HUD"
};

// Spotify & YT Music Now Playing Card
static Minigame gameMusic = {
  "NOW PLAYING",
  nullptr,
  nullptr,
  BITMAP_MUSIC,
  BITMAP_MUSIC_WIDTH,
  BITMAP_MUSIC_HEIGHT,
  "SPOTIFY / YT MUSIC"
};

void showBootSplash() {
  u8g2.clearBuffer();

  // Decorative border
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  u8g2.drawFrame(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4);

  // Title
  u8g2.setFont(u8g2_font_7x14B_tf);
  Display::drawCenteredStr(18, "RETRO ARCADE C3");

  // Subtitle
  u8g2.setFont(u8g2_font_5x7_tf);
  Display::drawCenteredStr(30, "3-BUTTON + BLE WATCH");

  // Retro decorative arcade icons across bottom
  u8g2.drawXBMP(6, 42, BITMAP_BIRD_WIDTH, BITMAP_BIRD_HEIGHT, BITMAP_BIRD);
  u8g2.drawXBMP(28, 42, BITMAP_COPTER_WIDTH, BITMAP_COPTER_HEIGHT, BITMAP_COPTER);
  u8g2.drawXBMP(52, 42, BITMAP_SHOOTER_WIDTH, BITMAP_SHOOTER_HEIGHT, BITMAP_SHOOTER);
  u8g2.drawXBMP(76, 42, BITMAP_SNAKE_WIDTH, BITMAP_SNAKE_HEIGHT, BITMAP_SNAKE);
  u8g2.drawXBMP(100, 42, BITMAP_WATCH_WIDTH, BITMAP_WATCH_HEIGHT, BITMAP_WATCH);

  u8g2.sendBuffer();
  delay(1200);
}

void setup() {
  // Initialize Serial over native USB CDC
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("  ESP32-C3 3-BUTTON ARCADE & BLE WATCH");
  Serial.println("========================================");
  Serial.printf("[Pins] LEFT=GPIO %d, OK=GPIO %d, RIGHT=GPIO %d\n", PIN_BTN_LEFT, PIN_BTN_OK, PIN_BTN_RIGHT);
  Serial.printf("[Pins] OLED SDA=GPIO %d, SCL=GPIO %d\n", PIN_I2C_SDA, PIN_I2C_SCL);

  // Initialize Hardware I2C Display (SDA=GPIO 4, SCL=GPIO 5)
  Display::init();

  // Initialize Engine & Preferences (NVS High Scores)
  engine.begin();
  Serial.println("[Engine] High scores loaded from NVS.");

  // Initialize Bluetooth Low Energy & RTC Clock Manager
  ClockManager::begin();

  // Initialize Music Manager (USB Serial & BLE Media Sync)
  MusicManager::begin();

  // Register Modules: Now Playing & Watch first, followed by games
  engine.registerGame(gameMusic);
  engine.registerGame(gameWatch);
  engine.registerGame(gameNav);
  engine.registerGame(GameFlappy::game);
  engine.registerGame(GameLander::game);
  engine.registerGame(GameRunner::game);
  engine.registerGame(GameCopter::game);
  engine.registerGame(GameShooter::game);
  engine.registerGame(GameStacker::game);
  engine.registerGame(GameSnake::game);
  engine.registerGame(GamePong::game);
  engine.registerGame(GameFrogger::game);
  engine.registerGame(GameDodge::game);
  Serial.println("[Engine] 13 modules registered (Music + Watch + Nav + 10 Games)!");

  // Show Retro Splash
  Serial.println("[Splash] Showing boot splash...");
  showBootSplash();

  // Initialize physical 3-button inputs AFTER splash
  inputManager.begin();
  Serial.println("[Input] 3 Buttons initialized (LEFT=0, OK=3, RIGHT=1).");

  Serial.println("[Engine] System ready, entering main loop!");
}

static unsigned long lastHeartbeat = 0;

void loop() {
  // Update BLE, clock, and music background tasks
  ClockManager::update();
  MusicManager::update();

  if (millis() - lastHeartbeat >= 3000) {
    lastHeartbeat = millis();
    char timeStr[16];
    ClockManager::getTimeString(timeStr, sizeof(timeStr));
    Serial.printf("[Heartbeat] time=%s, L=%d, OK=%d, R=%d, state=%d, BLE_conn=%d\n",
                  timeStr,
                  inputManager.isLeftDown(),
                  inputManager.isOkDown(),
                  inputManager.isRightDown(),
                  engine.getState(),
                  ClockManager::isBleConnected());
  }

  // Poll inputs with high frequency to guarantee zero missed clicks
  ButtonEvent event = inputManager.update();
  if (event == BTN_CLICK || event == BTN_LONG_PRESS || 
      event == BTN_LEFT_CLICK || event == BTN_RIGHT_CLICK) {
    latchedEvent = event;
    engine.reportActivity();
  }

  if (inputManager.isAnyDown()) {
    engine.reportActivity();
  }

  // Frame rate regulation (25 FPS -> 40ms per frame)
  unsigned long now = millis();
  if (now - lastFrameTime >= TARGET_FRAME_TIME_MS) {
    lastFrameTime = now;

    ButtonEvent eventToDispatch = latchedEvent;
    latchedEvent = BTN_NONE;

    // Check continuous holding if no one-shot click occurred
    if (eventToDispatch == BTN_NONE) {
      if (event == BTN_HOLDING) {
        eventToDispatch = BTN_HOLDING;
      } else if (event == BTN_LEFT_HOLD) {
        eventToDispatch = BTN_LEFT_HOLD;
      } else if (event == BTN_RIGHT_HOLD) {
        eventToDispatch = BTN_RIGHT_HOLD;
      }
    }

    // Update game physics, state machine, and render OLED
    engine.update(eventToDispatch);
    delay(4); // Inter-frame relaxation delay
  }

  // Yield to allow background ESP32 BLE and USB CDC tasks
  yield();
}
