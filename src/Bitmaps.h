#pragma once

#include <Arduino.h>

// =============================================================================
// RETRO PIXEL ART BITMAPS (1-bit PROGMEM for U8g2 drawXBMP)
// =============================================================================

// Flappy Bird Sprite (12x10)
constexpr uint8_t BITMAP_BIRD_WIDTH = 12;
constexpr uint8_t BITMAP_BIRD_HEIGHT = 10;
const uint8_t BITMAP_BIRD[] PROGMEM = {
  0x70, 0x00, //   xxx
  0x88, 0x01, //  x   x  x
  0x84, 0x02, //  x    x x
  0x82, 0x07, //  x     xxx
  0xfe, 0x04, // xxxxxxx  x
  0x02, 0x04, //  x      x
  0xfa, 0x03, // xxxxx x xx
  0x06, 0x00, //  xx
  0x1c, 0x00, //   xxx
  0x00, 0x00
};

// Flappy Bird Wing Up (12x10)
const uint8_t BITMAP_BIRD_WING_UP[] PROGMEM = {
  0x70, 0x00,
  0xf8, 0x01,
  0x84, 0x02,
  0x92, 0x07,
  0xbe, 0x04,
  0x02, 0x04,
  0xfa, 0x03,
  0x06, 0x00,
  0x1c, 0x00,
  0x00, 0x00
};

// Lunar Lander Sprite (14x12)
constexpr uint8_t BITMAP_LANDER_WIDTH = 14;
constexpr uint8_t BITMAP_LANDER_HEIGHT = 12;
const uint8_t BITMAP_LANDER[] PROGMEM = {
  0x38, 0x07, //    xxx   xxx
  0x44, 0x08, //   x   x x   x
  0xfe, 0x1f, //  xxxxxxxxxxx
  0x82, 0x10, //  x         x
  0xba, 0x17, //  x xxx x xxx
  0x82, 0x10, //  x         x
  0xfe, 0x1f, //  xxxxxxxxxxx
  0x28, 0x05, //   x x   x x
  0x54, 0x0a, //  x x x x x x
  0x82, 0x10, // x           x
  0x82, 0x10, // x           x
  0xc6, 0x18  // xx         xx
};

// Rocket Flame (6x8)
constexpr uint8_t BITMAP_FLAME_WIDTH = 6;
constexpr uint8_t BITMAP_FLAME_HEIGHT = 8;
const uint8_t BITMAP_FLAME[] PROGMEM = {
  0x3c, //  xxxx
  0x3c, //  xxxx
  0x18, //   xx
  0x3c, //  xxxx
  0x18, //   xx
  0x24, //  x  x
  0x18, //   xx
  0x00
};

// Cyber Runner Sprite (Frame 1 - 10x14)
constexpr uint8_t BITMAP_RUNNER_WIDTH = 10;
constexpr uint8_t BITMAP_RUNNER_HEIGHT = 14;
const uint8_t BITMAP_RUNNER_1[] PROGMEM = {
  0x30, 0x00, //   xx
  0x30, 0x00, //   xx
  0x78, 0x00, //  xxxx
  0xfc, 0x00, // xxxxxx
  0x78, 0x00, //  xxxx
  0x30, 0x00, //   xx
  0x30, 0x00, //   xx
  0x78, 0x00, //  xxxx
  0xdc, 0x00, // xx xxx
  0x9c, 0x00, // x  xxx
  0x18, 0x00, //   xx
  0x30, 0x00, //   xx
  0x60, 0x00, //  xx
  0xc0, 0x00  // xx
};

// Cyber Runner Sprite (Frame 2 - 10x14)
const uint8_t BITMAP_RUNNER_2[] PROGMEM = {
  0x30, 0x00,
  0x30, 0x00,
  0x78, 0x00,
  0xfc, 0x00,
  0x78, 0x00,
  0x30, 0x00,
  0x30, 0x00,
  0x78, 0x00,
  0x3c, 0x00,
  0x3a, 0x00,
  0x18, 0x00,
  0x0c, 0x00,
  0x06, 0x00,
  0x03, 0x00
};

// Trophy Icon (10x10)
constexpr uint8_t BITMAP_TROPHY_WIDTH = 10;
constexpr uint8_t BITMAP_TROPHY_HEIGHT = 10;
const uint8_t BITMAP_TROPHY[] PROGMEM = {
  0xfe, 0x01, // xxxxxxxx
  0xff, 0x03, // xxxxxxxxxx
  0xd6, 0x01, // x x x x xx
  0xd6, 0x01, // x x x x xx
  0xfe, 0x01, // xxxxxxxx
  0x7c, 0x00, //  xxxxx
  0x38, 0x00, //   xxx
  0x10, 0x00, //    x
  0x38, 0x00, //   xxx
  0x7c, 0x00  //  xxxxx
};

// Arrow Left (5x7)
constexpr uint8_t BITMAP_ARROW_L_WIDTH = 5;
constexpr uint8_t BITMAP_ARROW_L_HEIGHT = 7;
const uint8_t BITMAP_ARROW_LEFT[] PROGMEM = {
  0x08, //    x
  0x0c, //   xx
  0x0e, //  xxx
  0x0f, // xxxx
  0x0e, //  xxx
  0x0c, //   xx
  0x08  //    x
};

// Arrow Right (5x7)
constexpr uint8_t BITMAP_ARROW_R_WIDTH = 5;
constexpr uint8_t BITMAP_ARROW_R_HEIGHT = 7;
const uint8_t BITMAP_ARROW_RIGHT[] PROGMEM = {
  0x02, //  x
  0x06, //  xx
  0x0e, //  xxx
  0x1e, // xxxx
  0x0e, //  xxx
  0x06, //  xx
  0x02  //  x
};

// =============================================================================
// NEW MINIGAME ICONS (1-bit PROGMEM for Carousel Menu & In-Game Sprites)
// =============================================================================

// 4. Cave Copter (14x10)
constexpr uint8_t BITMAP_COPTER_WIDTH = 14;
constexpr uint8_t BITMAP_COPTER_HEIGHT = 10;
const uint8_t BITMAP_COPTER[] PROGMEM = {
  0xf8, 0x07, // ---xxxxxxxx---
  0x40, 0x00, // ------xx------
  0x38, 0x01, // ---xxxxxx-----
  0x3f, 0x01, // xxxx--xxxx----
  0xff, 0x0f, // xxxxxxxxxxxx--
  0xff, 0x0a, // xxxxxxxxxx--x-
  0x08, 0x01, // ---x----x-----
  0xff, 0x0f, // xxxxxxxxxxxx--
  0x00, 0x00,
  0x00, 0x00
};

// 5. Space Blast Ship (12x10)
constexpr uint8_t BITMAP_SHOOTER_WIDTH = 12;
constexpr uint8_t BITMAP_SHOOTER_HEIGHT = 10;
const uint8_t BITMAP_SHOOTER[] PROGMEM = {
  0x60, 0x00, // -----xx-----
  0x70, 0x00, // ----xxxx----
  0x70, 0x00, // ----xxxx----
  0x78, 0x01, // ---xxxxxx---
  0xdb, 0x01, // --xx-xx-xx--
  0xfe, 0x03, // -xxxxxxxxxx-
  0xff, 0x0f, // xxxxxxxxxxxx
  0xcc, 0x09, // xx--xxxx--xx
  0x81, 0x08, // x----xx----x
  0x60, 0x00  // -----xx-----
};

// 6. Stacker Tower (12x10)
constexpr uint8_t BITMAP_STACKER_WIDTH = 12;
constexpr uint8_t BITMAP_STACKER_HEIGHT = 10;
const uint8_t BITMAP_STACKER[] PROGMEM = {
  0x70, 0x00, // ----xxxx----
  0x70, 0x00, // ----xxxx----
  0x00, 0x00,
  0xfc, 0x03, // --xxxxxxxx--
  0xfc, 0x03, // --xxxxxxxx--
  0x00, 0x00,
  0xff, 0x0f, // xxxxxxxxxxxx
  0xff, 0x0f, // xxxxxxxxxxxx
  0x00, 0x00,
  0xff, 0x0f  // xxxxxxxxxxxx
};

// 7. Snake & Apple (12x10)
constexpr uint8_t BITMAP_SNAKE_WIDTH = 12;
constexpr uint8_t BITMAP_SNAKE_HEIGHT = 10;
const uint8_t BITMAP_SNAKE[] PROGMEM = {
  0x80, 0x00, // -------x----
  0xc0, 0x01, // ------xxx---
  0xc0, 0x01, // ------xxx---
  0x00, 0x00,
  0x1e, 0x00, // -xxxx-------
  0x12, 0x00, // -x--x-------
  0xfe, 0x01, // -x-xxxxxxxx-
  0x92, 0x02, // -x-x------x-
  0x0f, 0x0c, // xxxx------xx
  0x00, 0x00
};

// 8. Brick Pop (12x10)
constexpr uint8_t BITMAP_PONG_WIDTH = 12;
constexpr uint8_t BITMAP_PONG_HEIGHT = 10;
const uint8_t BITMAP_PONG[] PROGMEM = {
  0xef, 0x0f, // xxxx-xxxxxxx
  0xef, 0x0f, // xxxx-xxxxxxx
  0x00, 0x00,
  0x18, 0x00, // ---xx-------
  0x18, 0x00, // ---xx-------
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0xfe, 0x03, // -xxxxxxxxxx-
  0xfe, 0x03  // -xxxxxxxxxx-
};

// 9. Road Hopper Frog (12x10)
constexpr uint8_t BITMAP_FROG_WIDTH = 12;
constexpr uint8_t BITMAP_FROG_HEIGHT = 10;
const uint8_t BITMAP_FROG[] PROGMEM = {
  0x86, 0x03, // -xx------xx-
  0xcf, 0x07, // xxxx----xxxx
  0xfc, 0x03, // --xxxxxxxx--
  0x7e, 0x02, // -x-xxxxxx-x-
  0xfe, 0x03, // -xxxxxxxxxx-
  0xfc, 0x03, // --xxxxxxxx--
  0xfc, 0x03, // --xxxxxxxx--
  0xcf, 0x07, // xxxx----xxxx
  0x89, 0x09, // x--x----x--x
  0x03, 0x0c  // xx--------xx
};

// 10. Meteor Storm (12x10)
constexpr uint8_t BITMAP_METEOR_WIDTH = 12;
constexpr uint8_t BITMAP_METEOR_HEIGHT = 10;
const uint8_t BITMAP_METEOR[] PROGMEM = {
  0x00, 0x07, // ---------xxx
  0x80, 0x07, // -------xxxx-
  0x60, 0x01, // -----xxxx---
  0x70, 0x00, // ----xxxx----
  0x78, 0x01, // ---xxxxxx---
  0xfc, 0x03, // --xxxxxxxx--
  0xfe, 0x03, // -xxxxxxxxxx-
  0xfe, 0x03, // -xxxxxxxxxx-
  0xfc, 0x03, // --xxxxxxxx--
  0x78, 0x01  // ---xxxxxx---
};

// 11. Retro Digital Watch (12x12)
constexpr uint8_t BITMAP_WATCH_WIDTH = 12;
constexpr uint8_t BITMAP_WATCH_HEIGHT = 12;
const uint8_t BITMAP_WATCH[] PROGMEM = {
  0x38, 0x00, // ---xxx------
  0x38, 0x00, // ---xxx------
  0xfe, 0x03, // -xxxxxxxxxx-
  0x02, 0x02, // -x--------x-
  0x82, 0x02, // -x---x----x-
  0xc2, 0x02, // -x---xx---x-
  0x02, 0x02, // -x--------x-
  0x02, 0x02, // -x--------x-
  0xfe, 0x03, // -xxxxxxxxxx-
  0x38, 0x00, // ---xxx------
  0x38, 0x00, // ---xxx------
  0x00, 0x00
};

// 12. Navigation Pin / Turn Arrow (12x12)
constexpr uint8_t BITMAP_NAV_WIDTH = 12;
constexpr uint8_t BITMAP_NAV_HEIGHT = 12;
const uint8_t BITMAP_NAV[] PROGMEM = {
  0x30, 0x00, // ----xx------
  0x78, 0x00, // ---xxxx-----
  0xfc, 0x00, // --xxxxxx----
  0xfe, 0x01, // -xxxxxxxx---
  0xff, 0x03, // xxxxxxxxxx--
  0x30, 0x00, // ----xx------
  0x30, 0x00, // ----xx------
  0x30, 0x00, // ----xx------
  0x30, 0x00, // ----xx------
  0x30, 0x00, // ----xx------
  0x00, 0x00,
  0x00, 0x00
};

// 13. Retro Music Note (12x12)
constexpr uint8_t BITMAP_MUSIC_WIDTH = 12;
constexpr uint8_t BITMAP_MUSIC_HEIGHT = 12;
const uint8_t BITMAP_MUSIC[] PROGMEM = {
  0xfe, 0x01, // -xxxxxxx----
  0xfe, 0x01, // -xxxxxxx----
  0x02, 0x01, // -x-----x----
  0x02, 0x01, // -x-----x----
  0x02, 0x01, // -x-----x----
  0x02, 0x01, // -x-----x----
  0x02, 0x01, // -x-----x----
  0xc7, 0x01, // xxx---xxx---
  0xc7, 0x01, // xxx---xxx---
  0x86, 0x01, // -xx---xx----
  0x00, 0x00,
  0x00, 0x00
};


