#pragma once

#include <Arduino.h>
#include "Config.h"

// Input event types for 3-button multiplexing & arcade control
enum ButtonEvent {
  BTN_NONE = 0,
  
  // Center / OK Button Events (Primary Action)
  BTN_CLICK,        // Released within 30ms - 450ms (Jump, Shoot, Launch Game, Retry)
  BTN_LONG_PRESS,   // Held down for > 600ms (Exit Game to Menu)
  BTN_HOLDING,      // Actively depressed (Thrust, Charge mechanics)

  // Left Button Events
  BTN_LEFT_CLICK,   // Left button clicked (Previous Game, Steer Left, Paddle Up, Turn CCW)
  BTN_LEFT_HOLD,    // Left button held down (Continuous left movement)

  // Right Button Events
  BTN_RIGHT_CLICK,  // Right button clicked (Next Game, Steer Right, Paddle Down, Turn CW)
  BTN_RIGHT_HOLD,   // Right button held down (Continuous right movement)

  // Universal button activity
  BTN_ANY_PRESS
};

class ButtonHandler {
public:
  explicit ButtonHandler(uint8_t pin = PIN_BUTTON);

  void begin();
  void reconfigure(uint8_t pin);
  ButtonEvent update();

  // Helper inspection methods
  bool isDown() const { return _debouncedState; }
  unsigned long getPressDuration() const;
  bool getRawState() const { return digitalRead(_pin); }
  bool getIdleState() const { return _idleState; }
  uint8_t getPin() const { return _pin; }

private:
  uint8_t _pin;
  bool _idleState;        // Pin level when released (auto-calibrated or HIGH)
  bool _lastRawPressed;
  bool _debouncedState;
  unsigned long _lastDebounceTime;
  unsigned long _pressStartTime;
  bool _longPressEmitted;
};

// Unified 3-Button Controller (Left, OK, Right)
class InputManager {
public:
  InputManager(uint8_t pinLeft = PIN_BTN_LEFT, 
               uint8_t pinOk = PIN_BTN_OK, 
               uint8_t pinRight = PIN_BTN_RIGHT);

  void begin();
  void reconfigure(uint8_t pinLeft, uint8_t pinOk, uint8_t pinRight);
  ButtonEvent update();

  bool isAnyDown() const;
  bool hasActivity() const;

  // Direct pin inspection for debugging
  bool isLeftDown() const { return _btnLeft.isDown(); }
  bool isOkDown() const { return _btnOk.isDown(); }
  bool isRightDown() const { return _btnRight.isDown(); }

private:
  ButtonHandler _btnLeft;
  ButtonHandler _btnOk;
  ButtonHandler _btnRight;
  bool _activityDetected;
};
