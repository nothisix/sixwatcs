#pragma once

#include "Minigame.h"

namespace GameRunner {
  extern Minigame game;
  void init();
  void update(ButtonEvent event);
}
