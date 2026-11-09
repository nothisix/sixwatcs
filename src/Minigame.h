#pragma once

#include "ButtonHandler.h"

// =============================================================================
// MODULAR MINIGAME INTERFACE
// =============================================================================
struct Minigame {
  const char* name;
  void (*init)();
  void (*update)(ButtonEvent event); // Updates physics/logic and renders via U8g2
  
  // Optional metadata for carousel display
  const uint8_t* icon;
  uint8_t iconWidth;
  uint8_t iconHeight;
  const char* instruction; // e.g. "CLICK TO FLAP", "HOLD THRUSTER"
};
