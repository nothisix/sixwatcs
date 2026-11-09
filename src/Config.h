#pragma once

#include <Arduino.h>

// =============================================================================
// HARDWARE PIN DEFINITIONS (ESP32-C3 SuperMini)
// =============================================================================

// 3 Arcade Push Buttons (Active LOW, internal pull-up enabled, wired directly to GND)
// Clean layout on ESP32-C3 SuperMini left header (Next to GND, no boot-strapping conflicts):
constexpr uint8_t PIN_BTN_LEFT  = 0;  // Button Kiri  : GPIO 0 (Clean GPIO / ADC1_CH0) <-> GND
constexpr uint8_t PIN_BTN_OK    = 3;  // Button OK    : GPIO 3 (Existing primary button) <-> GND
constexpr uint8_t PIN_BTN_RIGHT = 1;  // Button Kanan : GPIO 1 (Clean GPIO / ADC1_CH1) <-> GND

// Backward compatibility alias for single-button references
constexpr uint8_t PIN_BUTTON = PIN_BTN_OK;

// I2C OLED Display (0.96" SSD1306 128x64) - Dedicated clean GPIOs (Free from strapping & blue LED)
constexpr uint8_t PIN_I2C_SDA = 4;
constexpr uint8_t PIN_I2C_SCL = 5;

// Note on reserved pins:
// GPIO 18: Native USB D- (DO NOT USE)
// GPIO 19: Native USB D+ (DO NOT USE)
// GPIO 8:  Onboard Blue LED (Active LOW) & Boot Strapping Pin
// GPIO 9:  Boot Button / Strapping Pin

// =============================================================================
// TIMING & INPUT THRESHOLDS (Non-Blocking Debounce & Multiplexing)
// =============================================================================

// Mechanical switch debounce threshold (filters contact bounce/chatter)
constexpr unsigned long DEBOUNCE_TIME_MS = 30;

// Maximum duration of a press to register as a BTN_CLICK upon release
constexpr unsigned long CLICK_MAX_TIME_MS = 450;

// Minimum continuous hold time to trigger BTN_LONG_PRESS
constexpr unsigned long LONG_PRESS_TIME_MS = 600;

// AFK Screensaver Timeout: 5 minutes = 300,000 ms
// Automatically transitions to Watch Mode if no button is touched for 5 minutes
constexpr unsigned long AFK_TIMEOUT_MS = 300000;

// =============================================================================
// DISPLAY & PERFORMANCE CONFIGURATION
// =============================================================================

constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;

// Target Frame Rate: 25 FPS -> ~40ms per frame (smooth retro gameplay & rock-solid I2C power)
constexpr unsigned long TARGET_FRAME_TIME_MS = 40;
