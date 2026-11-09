#include "Engine.h"
#include "Bitmaps.h"
#include "ClockManager.h"
#include "SettingsManager.h"
#include "MusicManager.h"

EngineState Engine::s_state = STATE_CAROUSEL_MENU;
Engine* Engine::s_instance = nullptr;
Preferences Engine::s_prefs;

Engine::Engine()
    : _gameCount(0),
      _currentIndex(0),
      _previousIndex(0),
      _animOffset(0),
      _animVelocity(0),
      _animating(false),
      _lastActivityTime(0) {
  s_instance = this;
}

void Engine::begin() {
  s_prefs.begin("arcade", false);
  SettingsManager::begin();
  s_state = STATE_CAROUSEL_MENU;
  _lastActivityTime = millis();
}

bool Engine::registerGame(const Minigame& game) {
  if (_gameCount < MAX_GAMES) {
    _games[_gameCount++] = game;
    return true;
  }
  return false;
}

void Engine::returnToMenu() {
  s_state = STATE_CAROUSEL_MENU;
  if (s_instance) {
    s_instance->reportActivity();
  }
}

void Engine::enterWatchMode() {
  s_state = STATE_WATCH;
}

void Engine::enterSettings() {
  s_state = STATE_SETTINGS;
}

void Engine::enterNavigation() {
  s_state = STATE_NAVIGATION;
}

void Engine::enterMusicPlayer() {
  s_state = STATE_MUSIC;
}

void Engine::reportActivity() {
  _lastActivityTime = millis();
}

uint32_t Engine::getHighScore(const char* gameKey) {
  return s_prefs.getUInt(gameKey, 0);
}

void Engine::saveHighScore(const char* gameKey, uint32_t score) {
  uint32_t current = s_prefs.getUInt(gameKey, 0);
  if (score > current) {
    s_prefs.putUInt(gameKey, score);
  }
}

void Engine::launchGame(int8_t index) {
  if (index < 0 || index >= _gameCount) return;

  // Special case: If user selects "RETRO WATCH" from carousel
  if (strcmp(_games[index].name, "RETRO WATCH") == 0) {
    s_state = STATE_WATCH;
    return;
  }
  // Special case: If user selects "GMAPS NAV" from carousel
  if (strcmp(_games[index].name, "GMAPS NAV") == 0) {
    s_state = STATE_NAVIGATION;
    return;
  }
  // Special case: If user selects "NOW PLAYING" from carousel
  if (strcmp(_games[index].name, "NOW PLAYING") == 0) {
    s_state = STATE_MUSIC;
    return;
  }

  s_state = STATE_GAME_RUNNING;
  if (_games[index].init != nullptr) {
    _games[index].init();
  }
  if (_games[index].update != nullptr) {
    _games[index].update(BTN_NONE);
  }
}

void Engine::update(ButtonEvent event) {
  // 1. High-priority Notification Pop-up Overlay (WhatsApp, Messages, etc.)
  if (ClockManager::hasActiveNotification()) {
    if (event != BTN_NONE) {
      ClockManager::dismissNotification();
      reportActivity();
      return;
    }
    ClockManager::renderNotificationPopup();
    return;
  }

  static unsigned long lastEngineLog = 0;
  if (millis() - lastEngineLog >= 2000) {
    lastEngineLog = millis();
    Serial.printf("[Engine] state=%d, gameIdx=%d, event=%d, afkElapsed=%lu ms\n", 
                  s_state, _currentIndex, event, (millis() - _lastActivityTime));
  }

  // If any button event was triggered, refresh activity timer
  if (event != BTN_NONE) {
    reportActivity();
  }

  // Check AFK Timeout (Configurable via Settings)
  // When idle for specified minutes in Menu or Game, auto-switch to Watch mode
  if (s_state != STATE_WATCH && s_state != STATE_SETTINGS && s_state != STATE_NAVIGATION && s_state != STATE_MUSIC) {
    unsigned long afkTimeout = SettingsManager::getAfkTimeoutMs();
    if (afkTimeout > 0 && (millis() - _lastActivityTime >= afkTimeout)) {
      Serial.println("[Engine] AFK timeout reached -> Switching to Watch Mode");
      enterWatchMode();
    }
  }

  if (s_state == STATE_CAROUSEL_MENU) {
    updateMenu(event);
  } else if (s_state == STATE_GAME_RUNNING) {
    if (_currentIndex < _gameCount && _games[_currentIndex].update != nullptr) {
      _games[_currentIndex].update(event);
    }
  } else if (s_state == STATE_WATCH) {
    updateWatch(event);
  } else if (s_state == STATE_SETTINGS) {
    updateSettings(event);
  } else if (s_state == STATE_NAVIGATION) {
    updateNavigation(event);
  } else if (s_state == STATE_MUSIC) {
    updateMusic(event);
  }
}

void Engine::updateWatch(ButtonEvent event) {
  // If user presses ANY button (Left, OK, Right), wake up to Carousel Menu!
  if (event != BTN_NONE) {
    // If navigation is active in Google Maps, OK button takes user to Navigation HUD
    if (ClockManager::isNavigationActive() && (event == BTN_CLICK)) {
      Serial.println("[Watch] Navigation is active -> Opening Navigation Screen");
      enterNavigation();
      return;
    }
    // If music is playing or active, OK button takes user to Music HUD
    if (MusicManager::hasActiveTrack() && (event == BTN_CLICK)) {
      Serial.println("[Watch] Music is active -> Opening Music HUD");
      enterMusicPlayer();
      return;
    }

    Serial.println("[Watch] Button pressed -> Waking up to Arcade Menu");
    reportActivity();
    returnToMenu();
    return;
  }

  // Render the Watch Face
  ClockManager::renderWatchFace();
}

void Engine::updateMusic(ButtonEvent event) {
  MusicManager::updateUI(event);
}

void Engine::updateNavigation(ButtonEvent event) {
  // If user holds OK (long press), return to Arcade Menu
  if (event == BTN_LONG_PRESS) {
    Serial.println("[Navigation] Hold OK -> Returning to Arcade Menu");
    returnToMenu();
    return;
  }

  // If user clicks OK, toggle to Watch Face or Menu
  if (event == BTN_CLICK) {
    Serial.println("[Navigation] Click OK -> Returning to Arcade Menu");
    returnToMenu();
    return;
  }

  ClockManager::renderNavigationScreen();
}

void Engine::updateSettings(ButtonEvent event) {
  SettingsManager::update(event);
}

void Engine::updateMenu(ButtonEvent event) {
  if (_gameCount == 0) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    Display::drawCenteredStr(32, "NO GAMES LOADED");
    u8g2.sendBuffer();
    return;
  }

  // 3-Button Menu Navigation:
  // LEFT Button: Open Settings!
  if (event == BTN_LEFT_CLICK && !_animating) {
    enterSettings();
    return;
  }
  // RIGHT Button: Next Game
  else if (event == BTN_RIGHT_CLICK && !_animating) {
    _previousIndex = _currentIndex;
    _currentIndex = (_currentIndex + 1) % _gameCount;
    _animOffset = 128; // Slide in from right
    _animating = true;
  }
  // OK Button Click or Long Press: Launch Game (or 1-button compat)
  else if ((event == BTN_CLICK || event == BTN_LONG_PRESS) && !_animating) {
    launchGame(_currentIndex);
    return;
  }

  // Smooth slide animation easing
  if (_animating) {
    if (_animOffset > 0) {
      int16_t step = (_animOffset / 3) + 2;
      _animOffset -= step;
      if (_animOffset <= 0) {
        _animOffset = 0;
        _animating = false;
      }
    } else if (_animOffset < 0) {
      int16_t step = (-_animOffset / 3) + 2;
      _animOffset += step;
      if (_animOffset >= 0) {
        _animOffset = 0;
        _animating = false;
      }
    }
  }

  // RENDER CAROUSEL MENU
  u8g2.clearBuffer();

  // Top Title Bar
  u8g2.setFont(u8g2_font_6x10_tf);
  Display::drawCenteredStr(10, "RETRO ARCADE C3");
  u8g2.drawHLine(0, 12, SCREEN_WIDTH);

  // Left & Right Navigation Arrows
  u8g2.drawXBMP(4, 30, BITMAP_ARROW_L_WIDTH, BITMAP_ARROW_L_HEIGHT, BITMAP_ARROW_LEFT);
  u8g2.drawXBMP(SCREEN_WIDTH - 9, 30, BITMAP_ARROW_R_WIDTH, BITMAP_ARROW_R_HEIGHT, BITMAP_ARROW_RIGHT);

  // Render active card
  drawMenuCard(_currentIndex, _animOffset);

  // Bottom Instructions Bar (LEFT: SETTINGS | OK: PLAY | RIGHT: NEXT)
  u8g2.drawHLine(0, 53, SCREEN_WIDTH);
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(4, 61, "LEFT: SET");
  Display::drawCenteredStr(61, "OK: PLAY");
  Display::drawRightAlignedStr(SCREEN_WIDTH - 4, 61, "RGHT: NEXT");

  u8g2.sendBuffer();
}

void Engine::drawMenuCard(int8_t index, int16_t xOffset) {
  if (index < 0 || index >= _gameCount) return;

  const Minigame& game = _games[index];

  int16_t cardX = xOffset + 14;
  int16_t cardY = 15;
  int16_t cardW = 100;
  int16_t cardH = 36;

  if (cardX + cardW <= 0 || cardX >= SCREEN_WIDTH) return;

  // Render frame
  u8g2.drawRFrame(cardX, cardY, cardW, cardH, 3);

  // Draw Game Icon if space permits
  if (game.icon != nullptr && (cardX + 24) < SCREEN_WIDTH && cardX >= 0) {
    int16_t iconX = cardX + 6;
    int16_t iconY = cardY + (cardH - game.iconHeight) / 2;
    u8g2.drawXBMP(iconX, iconY, game.iconWidth, game.iconHeight, game.icon);
  }

  // Draw Game Title
  int16_t textX = cardX + (game.icon != nullptr ? game.iconWidth + 12 : 10);
  if (textX + 20 < SCREEN_WIDTH && textX >= 0) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(textX, cardY + 14, game.name);

    if (strcmp(game.name, "RETRO WATCH") == 0) {
      char timeBuf[16];
      ClockManager::getTimeString(timeBuf, sizeof(timeBuf));
      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawStr(textX, cardY + 28, timeBuf);
    } else {
      char hiBuf[16];
      uint32_t hiScore = getHighScore(game.name);
      snprintf(hiBuf, sizeof(hiBuf), "HI: %lu", (unsigned long)hiScore);

      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawXBMP(textX, cardY + 20, BITMAP_TROPHY_WIDTH, BITMAP_TROPHY_HEIGHT, BITMAP_TROPHY);
      u8g2.drawStr(textX + 13, cardY + 28, hiBuf);
    }
  }
}
