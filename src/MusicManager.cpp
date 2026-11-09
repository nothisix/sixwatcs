#include "MusicManager.h"
#include "Display.h"
#include "Config.h"
#include "Bitmaps.h"
#include "ClockManager.h"
#include "Engine.h"
#include "SettingsManager.h"

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
uint32_t MusicManager::s_packetElapsedMs = 0;
uint32_t MusicManager::s_lineDurationMs = 1000;
unsigned long MusicManager::s_packetArrivalMs = 0;

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
  s_packetElapsedMs = 0;
  s_lineDurationMs = 1000;
  s_packetArrivalMs = 0;

  s_scrollOffset = 0;
  s_lastScrollTime = 0;
  s_pauseUntil = 0;

  s_hasExternalEq = false;
  s_lastExternalEqTime = 0;
  for (int i = 0; i < 5; i++) s_eqHeights[i] = 1;

  s_serialBuffer.reserve(256);
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
    s_smoothLyricOffset = 0.0f;
    s_packetElapsedMs = 0;
    s_lineDurationMs = 1000;
    s_packetArrivalMs = 0;
    s_lyricPauseUntil = millis() + 1000;
  }

  // Auto-switch to Music HUD if song is playing and user is not inside a game or settings
  if (isPlaying && Engine::getState() != STATE_GAME_RUNNING && Engine::getState() != STATE_SETTINGS) {
    Engine::enterMusicPlayer();
  }
}

void MusicManager::setLyrics(const String& currentLine, const String& nextLine, uint8_t progressPct) {
  setLyrics(currentLine, nextLine, (uint32_t)progressPct * 10, 1000);
}

void MusicManager::setLyrics(const String& currentLine, const String& nextLine, uint32_t elapsedMs, uint32_t durationMs) {
  if (s_lyricLine != currentLine) {
    s_lyricLine = currentLine;
    s_smoothLyricOffset = 0.0f;
    s_lyricScrollOffset = 0;
    s_lyricPauseUntil = millis() + 80;
  }
  s_nextLyricLine = nextLine;
  s_packetElapsedMs = elapsedMs;
  s_lineDurationMs = (durationMs > 150) ? durationMs : 150;
  s_packetArrivalMs = millis();
  s_lyricProgress = (uint8_t)((elapsedMs * 100) / s_lineDurationMs);
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
    // Format: LRC:<current_line>|<next_line>|<b0>,<b1>,<b2>,<b3>,<b4>|<elapsed_ms>|<duration_ms>
    // Or fallback: LRC:<current_line>|<next_line>|<b0>,<b1>,<b2>,<b3>,<b4>|<prog_pct>
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
    String eqStr = "";
    if (thirdPipe != -1) {
      eqStr = line.substring(secondPipe + 1, thirdPipe);
      int fourthPipe = line.indexOf('|', thirdPipe + 1);
      if (fourthPipe != -1) {
        uint32_t elapsedMs = (uint32_t)line.substring(thirdPipe + 1, fourthPipe).toInt();
        uint32_t durationMs = (uint32_t)line.substring(fourthPipe + 1).toInt();
        setLyrics(currLine, nextLine, elapsedMs, durationMs);
      } else {
        uint8_t prog = (uint8_t)line.substring(thirdPipe + 1).toInt();
        setLyrics(currLine, nextLine, prog);
      }
    } else {
      eqStr = line.substring(secondPipe + 1);
      setLyrics(currLine, nextLine, 0);
    }

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
  } else if (SettingsManager::processSerialCommand(line)) {
    // Config command handled by SettingsManager
    return;
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

void MusicManager::drawTempoGlidingLyric(int16_t clipX, int16_t clipY, int16_t clipW, int16_t clipH, int16_t textY, uint8_t fontSizeMode) {
  if (s_lyricLine.length() == 0) return;

  bool isHangul = Display::hasHangul(s_lyricLine);
  if (fontSizeMode == 2) {
    if (isHangul) u8g2.setFont(u8g2_font_gulim12_t_korean2);
    else u8g2.setFont(u8g2_font_7x14B_tf);
  } else if (fontSizeMode == 1) {
    if (isHangul) u8g2.setFont(u8g2_font_gulim12_t_korean2);
    else u8g2.setFont(u8g2_font_6x12_tf);
  } else {
    if (isHangul) u8g2.setFont(u8g2_font_gulim11_t_korean2);
    else u8g2.setFont(u8g2_font_5x7_tf);
  }

  int16_t lineW = u8g2.getUTF8Width(s_lyricLine.c_str());

  // 1. If text fits within window, center it nicely
  if (lineW <= clipW) {
    s_smoothLyricOffset = 0.0f;
    int16_t posX = clipX + (clipW - lineW) / 2;
    u8g2.drawUTF8(posX, textY, s_lyricLine.c_str());
    return;
  }

  // 2. If text exceeds window, calculate silky tempo-adaptive gliding
  int16_t maxOffset = lineW - clipW + 8; // 8px breathing space at end

  uint32_t localElapsed = s_packetElapsedMs;
  if (s_isPlaying && millis() > s_packetArrivalMs) {
    localElapsed += (millis() - s_packetArrivalMs);
  }
  float rawProgress = (float)localElapsed / (float)s_lineDurationMs;
  if (rawProgress < 0.0f) rawProgress = 0.0f;
  if (rawProgress > 1.0f) rawProgress = 1.0f;

  // Tempo-adaptive curve: fast tempo (<1500ms) has minimal lead-in
  float leadIn = (s_lineDurationMs < 1500) ? 0.04f : 0.07f;
  float leadOut = 1.0f - leadIn;
  float curve;
  if (rawProgress <= leadIn) {
    curve = 0.0f;
  } else if (rawProgress >= leadOut) {
    curve = 1.0f;
  } else {
    float norm = (rawProgress - leadIn) / (leadOut - leadIn);
    // Hermite smoothstep for organic ease-in / ease-out
    curve = norm * norm * (3.0f - 2.0f * norm);
  }

  float targetOffset = curve * (float)maxOffset;
  s_smoothLyricOffset += (targetOffset - s_smoothLyricOffset) * 0.45f;

  u8g2.setClipWindow(clipX, clipY, clipX + clipW, clipY + clipH);
  u8g2.drawUTF8(clipX + 2 - (int16_t)s_smoothLyricOffset, textY, s_lyricLine.c_str());
  u8g2.setMaxClipWindow();
}

void MusicManager::renderMusicScreen() {
  uint8_t style = SettingsManager::getSpotifyStyle();
  bool lyricsEnabled = SettingsManager::isLyricsEnabled();

  // =========================================================================
  // STYLE 1: RETRO CASSETTE TAPE
  // =========================================================================
  if (style == 1) {
    u8g2.clearBuffer();
    // Outer Cassette Shell
    u8g2.drawRFrame(4, 2, 120, 42, 3);
    // Inner window / cutout
    u8g2.drawFrame(18, 9, 92, 28);

    // Left Spool at (36, 23)
    u8g2.drawCircle(36, 23, 7);
    u8g2.drawCircle(36, 23, 2);
    // Right Spool at (92, 23)
    u8g2.drawCircle(92, 23, 7);
    u8g2.drawCircle(92, 23, 2);

    // If playing, animate rotating spokes on both spools!
    if (s_isPlaying) {
      float a = (millis() % 800) * 0.00785f;
      int16_t dx = (int16_t)roundf(cosf(a) * 5.0f);
      int16_t dy = (int16_t)roundf(sinf(a) * 5.0f);
      u8g2.drawLine(36 - dx, 23 - dy, 36 + dx, 23 + dy);
      u8g2.drawLine(36 + dy, 23 - dx, 36 - dy, 23 + dx);

      u8g2.drawLine(92 - dx, 23 - dy, 92 + dx, 23 + dy);
      u8g2.drawLine(92 + dy, 23 - dx, 92 - dy, 23 + dx);
    } else {
      u8g2.drawLine(31, 23, 41, 23);
      u8g2.drawLine(36, 18, 36, 28);
      u8g2.drawLine(87, 23, 97, 23);
      u8g2.drawLine(92, 18, 92, 28);
    }

    // Center tape window: Mini tape counter
    u8g2.drawFrame(48, 14, 32, 18);
    u8g2.setFont(u8g2_font_4x6_tf);
    if (s_isPlaying) {
      char ctr[10];
      snprintf(ctr, sizeof(ctr), "%02d:%02d", (int)((millis() / 60000) % 60), (int)((millis() / 1000) % 60));
      u8g2.drawStr(51, 25, ctr);
    } else {
      u8g2.drawStr(52, 25, "PAUSE");
    }

    // Cassette Top Label: Song Title (marquee if long)
    u8g2.setFont(u8g2_font_4x6_tf);
    String fullTrack = hasActiveTrack() ? (s_title + " - " + s_artist) : "NO TRACK PLAYING";
    int16_t trackW = u8g2.getUTF8Width(fullTrack.c_str());
    if (trackW <= 110) {
      Display::drawCenteredStr(7, fullTrack.c_str());
    } else {
      u8g2.setClipWindow(8, 2, 120, 9);
      u8g2.drawUTF8(8 - s_scrollOffset, 7, fullTrack.c_str());
      u8g2.setMaxClipWindow();
    }

    // Bottom Area (y=46..64): Tempo-gliding Lyric Line or Artist
    u8g2.drawHLine(4, 46, SCREEN_WIDTH - 8);
    if (lyricsEnabled && s_lyricLine.length() > 0) {
      drawTempoGlidingLyric(4, 47, SCREEN_WIDTH - 8, 16, 58, 1);
    } else {
      u8g2.setFont(u8g2_font_4x6_tf);
      if (hasActiveTrack()) {
        Display::drawCenteredStr(55, s_artist.c_str());
      } else {
        Display::drawCenteredStr(55, "PUTAR LAGU DI HP / PC");
      }
      Display::drawCenteredStr(63, "L:PREV | OK:PAUSE | R:NEXT");
    }

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // STYLE 2: BIG LYRICS FOCUS (KARAOKE MODE)
  // =========================================================================
  if (style == 2) {
    u8g2.clearBuffer();
    // Slim header (y=0..10)
    u8g2.setFont(u8g2_font_4x6_tf);
    String titleCut = hasActiveTrack() ? s_title : "KARAOKE HUD";
    if (titleCut.length() > 18) titleCut = titleCut.substring(0, 18);
    u8g2.drawStr(4, 8, titleCut.c_str());

    char timeBuf[16];
    ClockManager::getTimeString(timeBuf, sizeof(timeBuf));
    timeBuf[5] = '\0';
    Display::drawRightAlignedStr(SCREEN_WIDTH - 4, 8, timeBuf);
    u8g2.drawHLine(2, 10, SCREEN_WIDTH - 4);

    // Flanking equalizer bars on left & right
    for (int i = 0; i < 3; i++) {
      uint8_t h = s_eqHeights[i];
      if (h < 1) h = 1;
      if (h > 12) h = 12;
      u8g2.drawBox(2 + (i * 4), 38 - h, 2, h);
      u8g2.drawBox(118 + (i * 4), 38 - h, 2, h);
    }

    // Main prominent lyric line (y=13..34)
    if (!hasActiveTrack()) {
      u8g2.setFont(u8g2_font_5x7_tf);
      Display::drawCenteredStr(32, "WAITING FOR MUSIC...");
    } else if (lyricsEnabled && s_lyricLine.length() > 0) {
      // Tempo-gliding large prominent lyric (Karaoke Style)
      drawTempoGlidingLyric(12, 13, 104, 20, 29, 2);

      // Next lyric preview below
      if (s_nextLyricLine.length() > 0) {
        bool isHangul = Display::hasHangul(s_nextLyricLine);
        if (isHangul) u8g2.setFont(u8g2_font_gulim11_t_korean2);
        else u8g2.setFont(u8g2_font_5x7_tf);
        String nextCut = "> " + s_nextLyricLine;
        int16_t nextW = u8g2.getUTF8Width(nextCut.c_str());
        if (nextW <= 104) {
          Display::drawCenteredStr(45, nextCut.c_str());
        } else {
          u8g2.setClipWindow(12, 36, 116, 49);
          u8g2.drawUTF8(12 - s_scrollOffset, 45, nextCut.c_str());
          u8g2.setMaxClipWindow();
        }
      }
    } else {
      u8g2.setFont(u8g2_font_7x14B_tf);
      Display::drawCenteredStr(28, s_isPlaying ? "PLAYING" : "PAUSED");
      u8g2.setFont(u8g2_font_5x7_tf);
      Display::drawCenteredStr(42, s_artist.c_str());
    }

    // Bottom progress line (y=56..63)
    u8g2.drawHLine(2, 54, SCREEN_WIDTH - 4);
    uint32_t localElapsed = s_packetElapsedMs;
    if (s_isPlaying && millis() > s_packetArrivalMs) {
      localElapsed += (millis() - s_packetArrivalMs);
    }
    float rawProg = (float)localElapsed / (float)s_lineDurationMs;
    if (rawProg < 0.0f) rawProg = 0.0f;
    if (rawProg > 1.0f) rawProg = 1.0f;
    int16_t barW = (int16_t)(rawProg * 120.0f);
    u8g2.drawFrame(4, 57, 120, 4);
    if (barW > 0) u8g2.drawBox(4, 57, barW, 4);

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // STYLE 3: COMPACT VINYL / PLAYER
  // =========================================================================
  if (style == 3) {
    u8g2.clearBuffer();
    // Left side: Vinyl Disc at (24, 32), radius 22
    u8g2.drawCircle(24, 32, 22);
    u8g2.drawCircle(24, 32, 17);
    u8g2.drawCircle(24, 32, 12);
    u8g2.drawDisc(24, 32, 7);
    u8g2.setDrawColor(0);
    u8g2.drawCircle(24, 32, 2);
    u8g2.setDrawColor(1);

    // If playing, draw rotating groove notch
    if (s_isPlaying) {
      float a = (millis() % 1200) * 0.005236f;
      int16_t gx = 24 + (int16_t)roundf(cosf(a) * 14.0f);
      int16_t gy = 32 + (int16_t)roundf(sinf(a) * 14.0f);
      u8g2.drawDisc(gx, gy, 1);
    }

    // Right Side Player (x=50..126)
    u8g2.setFont(u8g2_font_4x6_tf);
    String app = s_appName.length() > 0 ? s_appName : "SPOTIFY";
    app.toUpperCase();
    u8g2.drawStr(52, 9, app.c_str());
    Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 9, s_isPlaying ? "▶ PLAY" : "❚❚ PAUSE");

    u8g2.drawHLine(50, 12, 76);

    // Song Title (Bold, with marquee scroll if long)
    u8g2.setFont(u8g2_font_5x7_tf);
    String titleStr = hasActiveTrack() ? s_title : "NO TRACK";
    int16_t tw = u8g2.getUTF8Width(titleStr.c_str());
    if (tw <= 74) {
      u8g2.drawUTF8(52, 22, titleStr.c_str());
    } else {
      u8g2.setClipWindow(52, 14, 126, 24);
      u8g2.drawUTF8(52 - s_scrollOffset, 22, titleStr.c_str());
      u8g2.setMaxClipWindow();
    }

    // Artist
    u8g2.setFont(u8g2_font_4x6_tf);
    String artCut = hasActiveTrack() ? s_artist : "STANDBY";
    artCut = Display::truncateUTF8(artCut, 74);
    u8g2.drawStr(52, 31, artCut.c_str());

    // Progress Bar
    u8g2.drawFrame(52, 36, 72, 4);
    uint32_t localElapsed = s_packetElapsedMs;
    if (s_isPlaying && millis() > s_packetArrivalMs) {
      localElapsed += (millis() - s_packetArrivalMs);
    }
    float rawProg = (float)localElapsed / (float)s_lineDurationMs;
    if (rawProg < 0.0f) rawProg = 0.0f;
    if (rawProg > 1.0f) rawProg = 1.0f;
    int16_t progW = (int16_t)(rawProg * 72.0f);
    if (progW > 0) u8g2.drawBox(52, 36, progW, 4);

    // Bottom: Tempo-gliding Lyric Ticker
    u8g2.drawHLine(50, 44, 76);
    if (lyricsEnabled && s_lyricLine.length() > 0) {
      drawTempoGlidingLyric(52, 45, 74, 12, 54, 0);
    } else {
      u8g2.setFont(u8g2_font_4x6_tf);
      u8g2.drawStr(52, 53, "L:PREV | OK | R:NEXT");
    }

    u8g2.setFont(u8g2_font_4x6_tf);
    Display::drawRightAlignedStr(SCREEN_WIDTH - 2, 62, "VINYL MODE");

    u8g2.sendBuffer();
    return;
  }

  // =========================================================================
  // STYLE 0: CLASSIC HUD (DEFAULT)
  // =========================================================================
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
  } else if (!lyricsEnabled) {
    for (int i = 0; i < 9; i++) {
      uint8_t h = (i % 2 == 0) ? s_eqHeights[i % 5] : (s_eqHeights[(i + 2) % 5] + 2);
      if (h < 2) h = 2;
      if (h > 16) h = 16;
      u8g2.drawBox(20 + (i * 10), 44 - h, 6, h);
    }
    u8g2.setFont(u8g2_font_4x6_tf);
    Display::drawCenteredStr(49, s_isPlaying ? "VISUALIZER MODE" : "[PAUSED]");
  } else if (s_lyricLine.length() == 0) {
    // Song is active but lyric has not loaded / instrumental / pause
    u8g2.drawXBMP(58, 25, BITMAP_MUSIC_WIDTH, BITMAP_MUSIC_HEIGHT, BITMAP_MUSIC);
    u8g2.setFont(u8g2_font_5x7_tf);
    if (s_isPlaying) {
      Display::drawCenteredStr(45, "Playing Music...");
    } else {
      Display::drawCenteredStr(45, "[Playback Paused]");
    }
  } else {
    // Current Active Singing Line (Prominent & Clear, tempo-gliding!)
    drawTempoGlidingLyric(4, 24, 120, 15, 36, 1);

    // Next Line Preview (Upcoming Lyric with > indicator)
    if (s_nextLyricLine.length() > 0) {
      bool isNextHangul = Display::hasHangul(s_nextLyricLine);
      if (isNextHangul) {
        u8g2.setFont(u8g2_font_gulim11_t_korean2);
      } else {
        u8g2.setFont(u8g2_font_4x6_tf);
      }
      String nextCut = "> " + s_nextLyricLine;
      int16_t nextW = u8g2.getUTF8Width(nextCut.c_str());
      if (nextW <= 120) {
        Display::drawCenteredStr(48, nextCut.c_str());
      } else {
        u8g2.setClipWindow(4, 40, 124, 50);
        u8g2.drawUTF8(4 - s_scrollOffset, 48, nextCut.c_str());
        u8g2.setMaxClipWindow();
      }
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
