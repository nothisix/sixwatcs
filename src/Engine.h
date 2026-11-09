#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "ButtonHandler.h"
#include "Minigame.h"
#include "Display.h"

enum EngineState {
  STATE_CAROUSEL_MENU,
  STATE_GAME_RUNNING,
  STATE_WATCH,
  STATE_SETTINGS,
  STATE_NAVIGATION,
  STATE_MUSIC
};

class Engine {
public:
  Engine();

  void begin();
  bool registerGame(const Minigame& game);
  void update(ButtonEvent event);

  // Global navigation to return to the carousel menu
  static void returnToMenu();
  static void enterWatchMode();
  static void enterSettings();
  static void enterNavigation();
  static void enterMusicPlayer();

  // Activity registration for AFK detection
  void reportActivity();

  // NVS High Score Storage helpers
  static uint32_t getHighScore(const char* gameKey);
  static void saveHighScore(const char* gameKey, uint32_t score);

  static EngineState getState() { return s_state; }

private:
  void updateMenu(ButtonEvent event);
  void updateWatch(ButtonEvent event);
  void updateSettings(ButtonEvent event);
  void updateNavigation(ButtonEvent event);
  void updateMusic(ButtonEvent event);
  void launchGame(int8_t index);
  void drawMenuCard(int8_t index, int16_t xOffset);

  static constexpr uint8_t MAX_GAMES = 16;
  Minigame _games[MAX_GAMES];
  uint8_t _gameCount;
  int8_t _currentIndex;
  int8_t _previousIndex;

  // Smooth Carousel Animation physics
  int16_t _animOffset;
  int16_t _animVelocity;
  bool _animating;

  unsigned long _lastActivityTime;

  static EngineState s_state;
  static Engine* s_instance;
  static Preferences s_prefs;
};
