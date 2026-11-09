#pragma once

#include <Arduino.h>
#include "ButtonHandler.h"

enum MusicSource {
  MUSIC_SRC_NONE = 0,
  MUSIC_SRC_BLE = 1,       // Mobile Phone via Chronos App (BLE)
  MUSIC_SRC_SERIAL = 2     // Laptop / PC via USB CDC Serial
};

class MusicManager {
public:
  static void begin();
  static void update();

  // BLE event handler from ChronosESP32
  static void onBleMusicReceived(const String& title, const String& artist, const String& app, uint8_t state);

  // Track setting
  static void setTrack(MusicSource source, const String& title, const String& artist, const String& app, bool isPlaying);

  // Synced Lyrics & Audio-reactive EQ setting
  static void setLyrics(const String& currentLine, const String& nextLine, uint32_t elapsedMs, uint32_t durationMs);
  static void setLyrics(const String& currentLine, const String& nextLine, uint8_t progressPct = 0);
  static void setEqualizer(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);

  // Queries
  static bool hasActiveTrack();
  static bool isPlaying();
  static const String& getTitle();
  static const String& getArtist();
  static const String& getAppName();
  static const String& getLyricLine();
  static const String& getNextLyricLine();
  static MusicSource getSource();

  // Playback control
  static void sendPlayPause();
  static void sendNext();
  static void sendPrevious();

  // Display & UI
  static void renderMusicScreen();
  static void updateUI(ButtonEvent event);

private:
  static void parseSerialCommand(const String& line);

  static MusicSource s_source;
  static String s_title;
  static String s_artist;
  static String s_appName;
  static bool s_isPlaying;
  static unsigned long s_lastUpdate;

  // Synced Lyrics
  static String s_lyricLine;
  static String s_nextLyricLine;
  static uint8_t s_lyricProgress;
  static float s_smoothLyricOffset;
  static int16_t s_lyricScrollOffset;
  static unsigned long s_lastLyricScrollTime;
  static unsigned long s_lyricPauseUntil;
  static uint32_t s_packetElapsedMs;
  static uint32_t s_lineDurationMs;
  static unsigned long s_packetArrivalMs;

  // Compact Title marquee scrolling
  static int16_t s_scrollOffset;
  static unsigned long s_lastScrollTime;
  static unsigned long s_pauseUntil;

  // Audio-reactive Equalizer
  static uint8_t s_eqHeights[5];
  static unsigned long s_lastEqTime;
  static bool s_hasExternalEq;
  static unsigned long s_lastExternalEqTime;

  // Serial incoming buffer
  static String s_serialBuffer;
};
