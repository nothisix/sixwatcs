#pragma once

#include "Minigame.h"

namespace GameLander {
  extern Minigame game;
  void init();
  void update(ButtonEvent event);
}
