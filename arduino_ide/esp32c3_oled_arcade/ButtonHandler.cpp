#include "ButtonHandler.h"

// =============================================================================
// ButtonHandler Implementation
// =============================================================================

ButtonHandler::ButtonHandler(uint8_t pin)
    : _pin(pin),
      _idleState(HIGH),
      _lastRawPressed(false),
      _debouncedState(false),
      _lastDebounceTime(0),
      _pressStartTime(0),
      _longPressEmitted(false) {}

void ButtonHandler::begin() {
  pinMode(_pin, INPUT_PULLUP);
  delay(10); // Allow internal pullup voltage to stabilize at 3.3V

  _idleState = HIGH; // Unpressed is HIGH with pullup
  _lastRawPressed = (digitalRead(_pin) == LOW);
  _debouncedState = _lastRawPressed;
  _lastDebounceTime = millis();
  _pressStartTime = 0;
  _longPressEmitted = false;

  Serial.printf("[Input] Button initialized on GPIO %d (Active LOW, INPUT_PULLUP)\n", _pin);
}

ButtonEvent ButtonHandler::update() {
  unsigned long now = millis();
  
  // Active LOW: Pressed when pin is LOW
  bool rawPressed = (digitalRead(_pin) == LOW);
  ButtonEvent eventToReturn = BTN_NONE;

  // Filter mechanical switch contact chatter
  if (rawPressed != _lastRawPressed) {
    _lastDebounceTime = now;
    _lastRawPressed = rawPressed;
  }

  // Once stable beyond debounce threshold, commit state change
  if ((now - _lastDebounceTime) >= DEBOUNCE_TIME_MS) {
    if (rawPressed != _debouncedState) {
      _debouncedState = rawPressed;

      if (_debouncedState) {
        // Just pressed down
        _pressStartTime = now;
        _longPressEmitted = false;
      } else {
        // Just released
        unsigned long duration = now - _pressStartTime;
        if (!_longPressEmitted && duration >= DEBOUNCE_TIME_MS && duration <= CLICK_MAX_TIME_MS) {
          eventToReturn = BTN_CLICK;
        }
        _longPressEmitted = false;
        return eventToReturn;
      }
    }
  }

  // If button is currently being held down
  if (_debouncedState) {
    unsigned long duration = now - _pressStartTime;

    // Detect one-shot trigger when hold threshold is crossed
    if (!_longPressEmitted && duration >= LONG_PRESS_TIME_MS) {
      _longPressEmitted = true;
      return BTN_LONG_PRESS;
    }

    // Continuous holding event for thrust/charge mechanics
    return BTN_HOLDING;
  }

  return BTN_NONE;
}

unsigned long ButtonHandler::getPressDuration() const {
  if (!_debouncedState) return 0;
  return millis() - _pressStartTime;
}

// =============================================================================
// InputManager (3-Button Unified Coordinator)
// =============================================================================

InputManager::InputManager(uint8_t pinLeft, uint8_t pinOk, uint8_t pinRight)
    : _btnLeft(pinLeft),
      _btnOk(pinOk),
      _btnRight(pinRight),
      _activityDetected(false) {}

void InputManager::begin() {
  _btnLeft.begin();
  _btnOk.begin();
  _btnRight.begin();
}

ButtonEvent InputManager::update() {
  _activityDetected = false;

  ButtonEvent evLeft  = _btnLeft.update();
  ButtonEvent evOk    = _btnOk.update();
  ButtonEvent evRight = _btnRight.update();

  if (evLeft != BTN_NONE || evOk != BTN_NONE || evRight != BTN_NONE ||
      _btnLeft.isDown() || _btnOk.isDown() || _btnRight.isDown()) {
    _activityDetected = true;
  }

  // Priority 1: One-shot Click / Long-Press events (Release or Hold trigger)
  if (evOk == BTN_LONG_PRESS) return BTN_LONG_PRESS;
  if (evOk == BTN_CLICK)      return BTN_CLICK;
  if (evLeft == BTN_CLICK)    return BTN_LEFT_CLICK;
  if (evRight == BTN_CLICK)   return BTN_RIGHT_CLICK;

  // Priority 2: Continuous Holding events
  if (evOk == BTN_HOLDING)    return BTN_HOLDING;
  if (_btnLeft.isDown())      return BTN_LEFT_HOLD;
  if (_btnRight.isDown())     return BTN_RIGHT_HOLD;

  return BTN_NONE;
}

bool InputManager::isAnyDown() const {
  return _btnLeft.isDown() || _btnOk.isDown() || _btnRight.isDown();
}

bool InputManager::hasActivity() const {
  return _activityDetected;
}
