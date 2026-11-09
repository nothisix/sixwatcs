#include "MusicManager.h"
#include "Display.h"
#include "Config.h"
#include "Bitmaps.h"
#include "ClockManager.h"
#include "Engine.h"

MusicSource MusicManager::s_source = MUSIC_SRC_NONE;
String MusicManager::s_title = "";
String MusicManager::s_artist = "";
String MusicManager::s_appName = "";
bool MusicManager::s_isPlaying = false;
unsigned long MusicManager::s_lastUpdate = 0;

String MusicManager::s_lyricLine = "";
String MusicManager::s_nextLyricLine = "";
uint8_t MusicManager::s_lyricProgress = 0;
float MusicManager::s_smoothLyricOffset = 0.0f;
int16_t MusicManager::s_lyricScrollOffset = 0;
unsigned long MusicManager::s_lastLyricScrollTime = 0;
unsigned long MusicManager::s_lyricPauseUntil = 0;

int16_t MusicManager::s_scrollOffset = 0;
unsigned long MusicManager::s_lastScrollTime = 0;
unsigned long MusicManager::s_pauseUntil = 0;

uint8_t MusicManager::s_eqHeights[5] = {1, 1, 1, 1, 1};
unsigned long MusicManager::s_lastEqTime = 0;
bool MusicManager::s_hasExternalEq = false;
unsigned long MusicManager::s_lastExternalEqTime = 0;

String MusicManager::s_serialBuffer = "";

void MusicManager::begin() {
  s_source = MUSIC_SRC_NONE;
  s_title = "";
  s_artist = "";
  s_appName = "MEDIA";
  s_isPlaying = false;
  s_lastUpdate = 0;

  s_lyricLine = "";
  s_nextLyricLine = "";
  s_lyricScrollOffset = 0;
  s_lastLyricScrollTime = 0;
  s_lyricPauseUntil = 0;

  s_scrollOffset = 0;
  s_lastScrollTime = 0;
  s_pauseUntil = 0;

  s_hasExternalEq = false;
  s_lastExternalEqTime = 0;
  for (int i = 0; i < 5; i++) s_eqHeights[i] = 1;

  s_serialBuffer.reserve(128);
}

void MusicManager::onBleMusicReceived(const String& title, const String& artist, const String& app, uint8_t state) {
  setTrack(MUSIC_SRC_BLE, title, artist, app, state == 1);
  Serial.printf("[Music] BLE Track: '%s' by '%s' [%s] (state=%d)\n",
                s_title.c_str(), s_artist.c_str(), s_appName.c_str(), s_isPlaying ? 1 : 0);
}

void MusicManager::setTrack(MusicSource source, const String& title, const String& artist, const String& app, bool isPlaying) {
  bool isNewSong = (s_title != title || s_artist != artist);
  s_source = source;
  s_title = title;
  s_artist = artist;
  s_appName = (app.length() > 0) ? app : "SPOTIFY";
  s_isPlaying = isPlaying;
  s_lastUpdate = millis();

  if (isNewSong) {
    s_scrollOffset = 0;
    s_pauseUntil = millis() + 1200;
    s_lyricLine = "";
    s_nextLyricLine = "";
    s_lyricScrollOffset = 0;
    s_lyricPauseUntil = millis() + 1000;
  }

  // Auto-switch to Music HUD if song is playing and user is not inside a game or settings
  if (isPlaying && Engine::getState() != STATE_GAME_RUNNING && Engine::getState() != STATE_SETTINGS) {
    Engine::enterMusicPlayer();
  }
}

void MusicManager::setLyrics(const String& currentLine, const String& nextLine, uint8_t progressPct) {
  if (s_lyricLine != currentLine) {
    s_lyricLine = currentLine;
    s_lyricScrollOffset = 0;
    s_smoothLyricOffset = 0.0f;
    s_lyricPauseUntil = millis() + 150;
  }
  s_nextLyricLine = nextLine;
  s_lyricProgress = (progressPct > 100) ? 100 : progressPct;
}

void MusicManager::setEqualizer(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
  s_eqHeights[0] = (b0 > 8) ? 8 : b0;
  s_eqHeights[1] = (b1 > 8) ? 8 : b1;
  s_eqHeights[2] = (b2 > 8) ? 8 : b2;
  s_eqHeights[3] = (b3 > 8) ? 8 : b3;
  s_eqHeights[4] = (b4 > 8) ? 8 : b4;
  s_hasExternalEq = true;
  s_lastExternalEqTime = millis();
}

bool MusicManager::hasActiveTrack() {
  return (s_title.length() > 0);
}

bool MusicManager::isPlaying() {
  return s_isPlaying;
}

const String& MusicManager::getTitle() {
  return s_title;
}

const String& MusicManager::getArtist() {
  return s_artist;
}

const String& MusicManager::getAppName() {
  return s_appName;
}

const String& MusicManager::getLyricLine() {
  return s_lyricLine;
}

const String& MusicManager::getNextLyricLine() {
  return s_nextLyricLine;
}

MusicSource MusicManager::getSource() {
  return s_source;
}

void MusicManager::parseSerialCommand(const String& line) {
  if (line.startsWith("SONG:")) {
    // Format: SONG:PLAY|Artist Name|Song Title|AppName
    // Format: SONG:PAUSE|Artist Name|Song Title|AppName
    // Format: SONG:STOP|||
    int firstPipe = line.indexOf('|', 5);
    if (firstPipe == -1) return;
    String stateStr = line.substring(5, firstPipe);

    int secondPipe = line.indexOf('|', firstPipe + 1);
    if (secondPipe == -1) return;
    String artistStr = line.substring(firstPipe + 1, secondPipe);

    int thirdPipe = line.indexOf('|', secondPipe + 1);
    String titleStr = "";
    String appStr = "SPOTIFY";
    if (thirdPipe != -1) {
      titleStr = line.substring(secondPipe + 1, thirdPipe);
      appStr = line.substring(thirdPipe + 1);
    } else {
      titleStr = line.substring(secondPipe + 1);
    }

    bool isPlay = (stateStr.equalsIgnoreCase("PLAY") || stateStr.equalsIgnoreCase("1"));
    bool isStop = stateStr.equalsIgnoreCase("STOP");

    if (isStop) {
      s_isPlaying = false;
      s_title = "";
      s_artist = "";
      s_lyricLine = "";
      s_nextLyricLine = "";
    } else {
      setTrack(MUSIC_SRC_SERIAL, titleStr, artistStr, appStr, isPlay);
    }
  } else if (line.startsWith("LRC:")) {
    // Format: LRC:<current_line>|<next_line>|<b0>,<b1>,<b2>,<b3>,<b4>|<prog_pct>
    int firstPipe = line.indexOf('|', 4);
    if (firstPipe == -1) {
      setLyrics(line.substring(4), "", 0);
      return;
    }
    String currLine = line.substring(4, firstPipe);

    int secondPipe = line.indexOf('|', firstPipe + 1);
    if (secondPipe == -1) {
      setLyrics(currLine, line.substring(firstPipe + 1), 0);
      return;
    }
    String nextLine = line.substring(firstPipe + 1, secondPipe);

    int thirdPipe = line.indexOf('|', secondPipe + 1);
    uint8_t prog = 0;
    String eqStr = "";
    if (thirdPipe != -1) {
      eqStr = line.substring(secondPipe + 1, thirdPipe);
      prog = (uint8_t)line.substring(thirdPipe + 1).toInt();
    } else {
      eqStr = line.substring(secondPipe + 1);
    }
    setLyrics(currLine, nextLine, prog);

    // Parse audio-reactive EQ values: e.g. "8,6,7,9,4"
    int c1 = eqStr.indexOf(',');
    int c2 = (c1 != -1) ? eqStr.indexOf(',', c1 + 1) : -1;
    int c3 = (c2 != -1) ? eqStr.indexOf(',', c2 + 1) : -1;
    int c4 = (c3 != -1) ? eqStr.indexOf(',', c3 + 1) : -1;
    if (c1 != -1 && c2 != -1 && c3 != -1 && c4 != -1) {
      uint8_t b0 = eqStr.substring(0, c1).toInt();
      uint8_t b1 = eqStr.substring(c1 + 1, c2).toInt();
      uint8_t b2 = eqStr.substring(c2 + 1, c3).toInt();
      uint8_t b3 = eqStr.substring(c3 + 1, c4).toInt();
      uint8_t b4 = eqStr.substring(c4 + 1).toInt();
      setEqualizer(b0, b1, b2, b3, b4);
    }
  } else if (line.startsWith("EQ:")) {
    // Format: EQ:<b0>,<b1>,<b2>,<b3>,<b4>
    String eqStr = line.substring(3);
    int c1 = eqStr.indexOf(',');
    int c2 = (c1 != -1) ? eqStr.indexOf(',', c1 + 1) : -1;
    int c3 = (c2 != -1) ? eqStr.indexOf(',', c2 + 1) : -1;
    int c4 = (c3 != -1) ? eqStr.indexOf(',', c3 + 1) : -1;
    if (c1 != -1 && c2 != -1 && c3 != -1 && c4 != -1) {
      uint8_t b0 = eqStr.substring(0, c1).toInt();
      uint8_t b1 = eqStr.substring(c1 + 1, c2).toInt();
      uint8_t b2 = eqStr.substring(c2 + 1, c3).toInt();
      uint8_t b3 = eqStr.substring(c3 + 1, c4).toInt();
      uint8_t b4 = eqStr.substring(c4 + 1).toInt();
      setEqualizer(b0, b1, b2, b3, b4);
    }
  } else if (line.equalsIgnoreCase("PING")) {
    Serial.println("[ESP32] PONG");
  }
}

void MusicManager::update() {
  // 1. Process incoming Serial data from Laptop USB
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (s_serialBuffer.length() > 0) {
        parseSerialCommand(s_serialBuffer);
        s_serialBuffer = "";
      }
    } else {
      if (s_serialBuffer.length() < 384) {
        s_serialBuffer += c;
      }
    }
  }

  // 2. Animate Equalizer Bars (Decay if no live EQ from PC)
  if (millis() - s_lastExternalEqTime > 500) {
    s_hasExternalEq = false;
  }

  if (!s_hasExternalEq) {
    if (millis() - s_lastEqTime >= 80) {
      s_lastEqTime = millis();
      if (s_isPlaying) {
        for (int i = 0; i < 5; i++) {
          s_eqHeights[i] = 1 + (uint8_t)(random(7));
        }
      } else {
        for (int i = 0; i < 5; i++) {
          if (s_eqHeights[i] > 1) s_eqHeights[i]--;
        }
      }
    }
  }

  // 3. Update Compact Title marquee scrolling
  if (s_title.length() > 0 && millis() > s_pauseUntil) {
    if (millis() - s_lastScrollTime >= 55) {
      s_lastScrollTime = millis();
      String fullTrack = s_title + " - " + s_artist;
      if (Display::hasHangul(fullTrack)) {
        u8g2.setFont(u8g2_font_gulim11_t_korean2);
      } else {
        u8g2.setFont(u8g2_font_5x7_tf);
      }
      int16_t strW = u8g2.getUTF8Width(fullTrack.c_str());
      if (strW > 120) {
        s_scrollOffset++;
        if (s_scrollOffset > (strW - 100)) {
          s_scrollOffset = 0;
          s_pauseUntil = millis() + 1500;
        }
      } else {
        s_scrollOffset = 0;
      }
    }
  }

  // 4. Update Synced Lyric Line marquee scrolling if line is extra long
  if (s_lyricLine.length() > 0 && millis() > s_lyricPauseUntil) {
    // Fast, smooth marquee scroll: 2 pixels every 28ms (~70 px/sec)
    if (millis() - s_lastLyricScrollTime >= 28) {
      s_lastLyricScrollTime = millis();
      if (Display::hasHangul(s_lyricLine)) {
        u8g2.setFont(u8g2_font_gulim12_t_korean2);
      } else {
        u8g2.setFont(u8g2_font_6x12_tf);
      }
      int16_t lineW = u8g2.getUTF8Width(s_lyricLine.c_str());
      if (lineW > 120) {
        s_lyricScrollOffset += 2;
        if (s_lyricScrollOffset > (lineW - 85)) {
          s_lyricScrollOffset = 0;
          s_lyricPauseUntil = millis() + 600;
        }
      } else {
        s_lyricScrollOffset = 0;
      }
    }
  }
}

void MusicManager::sendPlayPause() {
  s_isPlaying = !s_isPlaying;
  if (s_source == MUSIC_SRC_BLE) {
    ClockManager::sendMusicControl(0x9900); // MUSIC_TOGGLE
  } else {
    Serial.println("CMD:TOGGLE");
  }
}

void MusicManager::sendNext() {
  if (s_source == MUSIC_SRC_BLE) {
    ClockManager::sendMusicControl(0x9D03); // MUSIC_NEXT
  } else {
    Serial.println("CMD:NEXT");
  }
}

void MusicManager::sendPrevious() {
  if (s_source == MUSIC_SRC_BLE) {
    ClockManager::sendMusicControl(0x9D02); // MUSIC_PREVIOUS
  } else {
    Serial.println("CMD:PREV");
  }
}

void MusicManager::updateUI(ButtonEvent event) {
  if (event == BTN_LEFT_CLICK) {
    sendPrevious();
  } else if (event == BTN_RIGHT_CLICK) {
    sendNext();
  } else if (event == BTN_CLICK) {
    sendPlayPause();
  } else if (event == BTN_LONG_PRESS) {
    Serial.println("[Music] Hold OK -> Returning to Carousel Menu");
    Engine::returnToMenu();
    return;
  }

  renderMusicScreen();
}

void MusicManager::renderMusicScreen() {
  u8g2.clearBuffer();

  // Outer border
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  // =========================================================================
  // 1. TOP HEADER BAR (y=0..12): Source + Mini Clock + Audio-reactive Equalizer
  // =========================================================================
  u8g2.setFont(u8g2_font_4x6_tf);

  // Left: Source Tag
  const char* srcTag = "[USB]";
  if (s_source == MUSIC_SRC_BLE) srcTag = "[BLE]";
  else if (s_source == MUSIC_SRC_NONE) srcTag = "[READY]";
  u8g2.drawStr(4, 9, srcTag);

  // App Name
  String app = s_appName;
  if (app.length() == 0) app = "SPOTIFY";
  app.toUpperCase();
  if (app.length() > 7) app = app.substring(0, 7);
  u8g2.drawStr(25, 9, app.c_str());

  // Center: Mini Clock
  char timeBuf[16];
  ClockManager::getTimeString(timeBuf, sizeof(timeBuf));
  timeBuf[5] = '\0'; // HH:MM
  Display::drawCenteredStr(9, timeBuf);

  // Right: 5 Real Audio-Reactive Equalizer Bars (x=100..122, max height 8px)
  for (int i = 0; i < 5; i++) {
    uint8_t h = s_eqHeights[i];
    if (h < 1) h = 1;
    if (h > 8) h = 8;
    int16_t bx = 100 + (i * 5);
    int16_t by = 11 - h;
    u8g2.drawBox(bx, by, 3, h);
  }

  // Top Divider
  u8g2.drawHLine(2, 12, SCREEN_WIDTH - 4);

  // =========================================================================
  // 2. COMPACT TITLE & ARTIST (y=13..23) - Downsized as requested!
  // =========================================================================
  if (hasActiveTrack()) {
    String fullTrack = s_title;
    if (s_artist.length() > 0) fullTrack += " - " + s_artist;

    bool isTitleHangul = Display::hasHangul(fullTrack);
    if (isTitleHangul) {
      u8g2.setFont(u8g2_font_gulim11_t_korean2);
    } else {
      u8g2.setFont(u8g2_font_5x7_tf);
    }
    int16_t trackW = u8g2.getUTF8Width(fullTrack.c_str());
    if (trackW <= 120) {
      Display::drawCenteredStr(21, fullTrack.c_str());
    } else {
      u8g2.setClipWindow(4, 12, 124, 23);
      u8g2.drawUTF8(4 - s_scrollOffset, 21, fullTrack.c_str());
      u8g2.setMaxClipWindow();
    }
  } else {
    u8g2.setFont(u8g2_font_5x7_tf);
    Display::drawCenteredStr(20, "NOW PLAYING HUD");
  }

  // Middle Divider
  u8g2.drawHLine(4, 23, SCREEN_WIDTH - 8);

  // =========================================================================
  // 3. MAIN CENTER AREA: SYNCED LYRICS (y=24..50)
  // =========================================================================
  if (!hasActiveTrack()) {
    u8g2.drawXBMP(58, 27, BITMAP_MUSIC_WIDTH, BITMAP_MUSIC_HEIGHT, BITMAP_MUSIC);
    u8g2.setFont(u8g2_font_4x6_tf);
    Display::drawCenteredStr(46, "PUTAR LAGU DI SPOTIFY");
  } else if (s_lyricLine.length() == 0) {
    // Song is active but lyric has not loaded / instrumental / pause
    u8g2.drawXBMP(58, 26, BITMAP_MUSIC_WIDTH, BITMAP_MUSIC_HEIGHT, BITMAP_MUSIC);
    u8g2.setFont(u8g2_font_5x7_tf);
    if (s_isPlaying) {
      Display::drawCenteredStr(45, "♫ Playing Music...");
    } else {
      Display::drawCenteredStr(45, "⏸ Paused");
    }
  } else {
    // Current Active Singing Line (Prominent & Clear!)
    bool isLyricHangul = Display::hasHangul(s_lyricLine);
    if (isLyricHangul) {
      u8g2.setFont(u8g2_font_gulim12_t_korean2);
    } else {
      u8g2.setFont(u8g2_font_6x12_tf);
    }
    int16_t lyricW = u8g2.getUTF8Width(s_lyricLine.c_str());
    if (lyricW <= 120) {
      Display::drawCenteredStr(35, s_lyricLine.c_str());
    } else {
      // Dynamic tempo-synced offset:
      // Offset glides across strictly in sync with vocal tempo progress!
      int16_t maxOffset = lyricW - 90;
      float targetOffset = (float)(s_lyricProgress * maxOffset) / 100.0f;
      // Exponential lerp smoothing for silky Spotify inertia
      s_smoothLyricOffset += (targetOffset - s_smoothLyricOffset) * 0.35f;

      u8g2.setClipWindow(4, 24, 124, 37);
      u8g2.drawUTF8(4 - (int16_t)s_smoothLyricOffset, 35, s_lyricLine.c_str());
      u8g2.setMaxClipWindow();
    }

    // Spotify-style 1px micro karaoke vocal progress bar below active lyric
    if (s_isPlaying && s_lyricProgress > 0) {
      int16_t barW = (s_lyricProgress * 116) / 100;
      if (barW > 116) barW = 116;
      u8g2.drawHLine(6, 38, barW);
    }

    // Next Line Preview (Upcoming Lyric with > indicator)
    if (s_nextLyricLine.length() > 0) {
      bool isNextHangul = Display::hasHangul(s_nextLyricLine);
      if (isNextHangul) {
        u8g2.setFont(u8g2_font_gulim11_t_korean2);
      } else {
        u8g2.setFont(u8g2_font_4x6_tf);
      }
      String nextCut = "> " + s_nextLyricLine;
      nextCut = Display::truncateUTF8(nextCut, 120);
      Display::drawCenteredStr(48, nextCut.c_str());
    }
  }

  // =========================================================================
  // 4. BOTTOM FOOTER BAR (y=51..64)
  // =========================================================================
  u8g2.drawHLine(2, 51, SCREEN_WIDTH - 4);
  u8g2.setFont(u8g2_font_4x6_tf);

  bool showExitHint = ((millis() / 2500) % 2 == 1);
  if (showExitHint) {
    Display::drawCenteredStr(60, "TAHAN OK: KEMBALI KE MENU");
  } else {
    Display::drawCenteredStr(60, "L: PREV  |  OK: PAUSE  |  R: NEXT");
  }

  u8g2.sendBuffer();
}
