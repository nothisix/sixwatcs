"""
ESP32-C3 Retro Arcade & OLED Watch - Spotify Real-time Synced Lyrics & Audio EQ
================================================================================
1. Streams real-time synchronized LRC lyrics from LRCLIB to the ESP32 OLED display.
2. Captures Windows system audio volume (pycaw) to drive audio-reactive equalizer bars.
3. Listens for arcade hardware buttons (Left=Prev, OK=Play/Pause, Right=Next) to control Spotify.
"""

import sys
import time
import datetime
import asyncio
import threading
import argparse
import ctypes
import random
import requests
import serial
import serial.tools.list_ports

try:
    from winsdk.windows.media.control import (
        GlobalSystemMediaTransportControlsSessionManager as MediaManager,
        GlobalSystemMediaTransportControlsSessionPlaybackStatus as PlaybackStatus
    )
    HAS_WINSDK = True
except ImportError:
    HAS_WINSDK = False

try:
    from pycaw.pycaw import AudioUtilities, IAudioMeterInformation
    HAS_PYCAW = True
except ImportError:
    HAS_PYCAW = False

# Windows Virtual-Key Codes for Media Control fallback
VK_MEDIA_NEXT_TRACK = 0xB0
VK_MEDIA_PREV_TRACK = 0xB1
VK_MEDIA_PLAY_PAUSE = 0xB3
KEYEVENTF_KEYUP = 0x0002

def send_windows_media_key(vk_code):
    user32 = ctypes.windll.user32
    user32.keybd_event(vk_code, 0, 0, 0)
    user32.keybd_event(vk_code, 0, KEYEVENTF_KEYUP, 0)

def find_esp32_port():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        return None
    for p in ports:
        desc = (p.description or "").lower()
        if "esp" in desc or "usb jtag" in desc or "ch340" in desc or "cp210" in desc:
            return p.device
    if len(ports) == 1:
        return ports[0].device
    for p in ports:
        if p.device.upper() != "COM1":
            return p.device
    return ports[0].device

def parse_lrc(text):
    """Parses LRC timestamped lyrics into clean (seconds, text) tuples."""
    if not text:
        return []
    result = []
    for line in text.splitlines():
        line = line.strip()
        if line.startswith('[') and ']' in line:
            end_bracket = line.index(']')
            time_part = line[1:end_bracket]
            lyric_part = line[end_bracket + 1:].strip()
            parts = time_part.split(':')
            if len(parts) == 2:
                try:
                    m = float(parts[0])
                    s = float(parts[1])
                    if lyric_part:  # Keep only actual lyric lines for strict sync
                        result.append((m * 60.0 + s, lyric_part))
                except ValueError:
                    pass
    result.sort(key=lambda x: x[0])
    return result

def get_lyrics_at_time(parsed_lrc, pos_sec):
    """Finds current active line, next upcoming line, and tempo-synced progress percentage (0-100%)."""
    if not parsed_lrc:
        return "", "", 0

    curr_idx = -1
    for i, (t, text) in enumerate(parsed_lrc):
        if t <= pos_sec:
            curr_idx = i
        else:
            break

    if curr_idx == -1:
        # Song is playing intro before vocals begin
        first_line = parsed_lrc[0][1] if parsed_lrc else ""
        first_t = parsed_lrc[0][0] if parsed_lrc else 1.0
        prog = min(100, max(0, int((pos_sec / first_t) * 100))) if first_t > 0 else 0
        return "♫ (Intro)", first_line, prog

    t_start, curr_line = parsed_lrc[curr_idx]

    if curr_idx + 1 < len(parsed_lrc):
        t_end, next_line = parsed_lrc[curr_idx + 1]
    else:
        t_end = t_start + 5.0
        next_line = "♫ (Outro)"

    duration = max(0.4, t_end - t_start)
    elapsed = max(0.0, pos_sec - t_start)
    progress_ratio = min(1.0, max(0.0, elapsed / duration))
    prog_pct = int(progress_ratio * 100)

    return curr_line, next_line, prog_pct

class MediaCompanion:
    def __init__(self, port, baud=115200):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = True

        self.current_session = None
        self.current_title = ""
        self.current_artist = ""
        self.parsed_lyrics = []
        self.lyrics_cache = {}

        self.last_song_packet = ""
        self.last_lrc_sent = ""
        self.last_lrc_time = 0

        self.audio_meter = None
        self.last_meter_refresh = 0

    def connect(self):
        print(f"[PC Companion] Connecting to ESP32 on {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
            time.sleep(1.0)
            print(f"[PC Companion] Connected successfully to {self.port}!")
            return True
        except Exception as e:
            print(f"[Error] Failed to connect to {self.port}: {e}")
            return False

    def send_to_esp32(self, line):
        if self.ser and self.ser.is_open:
            try:
                msg = line.strip() + "\n"
                self.ser.write(msg.encode('utf-8'))
                self.ser.flush()
            except Exception as e:
                print(f"[Serial Write Error] {e}")

    def serial_listener_thread(self):
        while self.running and self.ser and self.ser.is_open:
            try:
                raw = self.ser.readline()
                if raw:
                    line = raw.decode('utf-8', errors='ignore').strip()
                    if line.startswith("CMD:"):
                        cmd = line[4:].strip().upper()
                        print(f"\n[ESP32 Button Action] {cmd}")
                        self.handle_esp32_command(cmd)
            except Exception:
                if self.running:
                    time.sleep(0.1)

    def handle_esp32_command(self, cmd):
        if cmd == "TOGGLE":
            if self.current_session:
                try:
                    asyncio.run(self.current_session.try_toggle_play_pause_async())
                    return
                except Exception:
                    pass
            send_windows_media_key(VK_MEDIA_PLAY_PAUSE)

        elif cmd == "NEXT":
            if self.current_session:
                try:
                    asyncio.run(self.current_session.try_skip_next_async())
                    return
                except Exception:
                    pass
            send_windows_media_key(VK_MEDIA_NEXT_TRACK)

        elif cmd == "PREV":
            if self.current_session:
                try:
                    asyncio.run(self.current_session.try_skip_previous_async())
                    return
                except Exception:
                    pass
            send_windows_media_key(VK_MEDIA_PREV_TRACK)

    def get_audio_peak(self):
        """Gets real-time audio volume peak from Windows Core Audio."""
        if not HAS_PYCAW:
            return 0.0
        now = time.time()
        if not self.audio_meter or (now - self.last_meter_refresh > 5.0):
            self.last_meter_refresh = now
            try:
                sessions = AudioUtilities.GetAllSessions()
                for s in sessions:
                    if s.Process and "spotify" in s.Process.name().lower():
                        self.audio_meter = s._ctl.QueryInterface(IAudioMeterInformation)
                        break
            except Exception:
                pass

        if self.audio_meter:
            try:
                return float(self.audio_meter.GetPeakValue())
            except Exception:
                self.audio_meter = None
        return 0.0

    def fetch_lyrics(self, artist, title):
        cache_key = f"{artist}|{title}".lower()
        if cache_key in self.lyrics_cache:
            return self.lyrics_cache[cache_key]

        print(f"\n[Lyrics] Searching synced lyrics for '{title}' by '{artist}'...")
        try:
            # 1. Exact match endpoint
            url = "https://lrclib.net/api/get"
            r = requests.get(url, params={"artist_name": artist, "track_name": title}, timeout=4)
            if r.status_code == 200:
                data = r.json()
                synced = data.get("syncedLyrics")
                if synced:
                    parsed = parse_lrc(synced)
                    print(f"[Lyrics] Found {len(parsed)} synced lyric lines!")
                    self.lyrics_cache[cache_key] = parsed
                    return parsed

            # 2. Search fallback
            search_url = "https://lrclib.net/api/search"
            r = requests.get(search_url, params={"q": f"{artist} {title}"}, timeout=4)
            if r.status_code == 200:
                items = r.json()
                for item in items:
                    if item.get("syncedLyrics"):
                        parsed = parse_lrc(item["syncedLyrics"])
                        print(f"[Lyrics] Found {len(parsed)} synced lyric lines via search!")
                        self.lyrics_cache[cache_key] = parsed
                        return parsed

        except Exception as e:
            print(f"[Lyrics Warning] Could not fetch lyrics: {e}")

        self.lyrics_cache[cache_key] = []
        return []

    async def get_current_media_info(self):
        if not HAS_WINSDK:
            return None
        try:
            mgr = await MediaManager.request_async()
            session = mgr.get_current_session()
            if not session:
                sessions = mgr.get_sessions()
                if sessions:
                    session = sessions[0]
            if not session:
                return None

            self.current_session = session
            props = await session.try_get_media_properties_async()
            playback = session.get_playback_info()
            timeline = session.get_timeline_properties()
            app_id = (session.source_app_user_model_id or "").lower()

            app_name = "SPOTIFY" if "spotify" in app_id else "MEDIA"
            is_playing = (playback.playback_status == PlaybackStatus.PLAYING)
            title = props.title.strip() if props.title else ""
            artist = props.artist.strip() if props.artist else ""
            pos_sec = timeline.position.total_seconds() if timeline and timeline.position else 0.0

            # Precise Real-time Timestamp Extrapolation:
            # Windows Media Session updates `last_updated_time` periodically.
            # Adding elapsed time since last_updated_time removes the 1-2s delay!
            if is_playing and timeline and timeline.last_updated_time:
                now_utc = datetime.datetime.now(datetime.timezone.utc)
                elapsed = (now_utc - timeline.last_updated_time).total_seconds()
                if 0.0 <= elapsed < 15.0:
                    pos_sec += elapsed

            # Add +0.20s lead-time compensation for serial transmission & frame rendering
            if is_playing:
                pos_sec += 0.20

            if not title:
                return None

            return {
                "title": title,
                "artist": artist,
                "app": app_name,
                "is_playing": is_playing,
                "pos_sec": pos_sec
            }
        except Exception:
            return None

    async def run(self):
        t = threading.Thread(target=self.serial_listener_thread, daemon=True)
        t.start()

        print("\n" + "=" * 60)
        print("   ESP32-C3 SPOTIFY SYNCED LYRICS & AUDIO-REACTIVE EQ")
        print("=" * 60)
        print(f" Serial Port : {self.port}")
        print(" Lyrics API  : LRCLIB (Real-time synchronized LRC)")
        print(" Audio Meter : Windows Core Audio Peak (pycaw)")
        print(" Controls    : Left=Prev | OK=Play/Pause | Right=Next")
        print(" Press Ctrl+C in this terminal to exit.")
        print("=" * 60 + "\n")

        while self.running:
            media = await self.get_current_media_info()
            now = time.time()

            if media:
                state_str = "PLAY" if media["is_playing"] else "PAUSE"
                track_id = f"{media['artist']}|{media['title']}"

                # New song detected
                if track_id != f"{self.current_artist}|{self.current_title}":
                    self.current_title = media["title"]
                    self.current_artist = media["artist"]
                    self.parsed_lyrics = self.fetch_lyrics(self.current_artist, self.current_title)

                    # Send SONG packet
                    song_pkt = f"SONG:{state_str}|{self.current_artist}|{self.current_title}|{media['app']}"
                    self.send_to_esp32(song_pkt)
                    self.last_song_packet = song_pkt
                    print(f"\n[Track Change] {self.current_title} - {self.current_artist}")

                # Audio-reactive peak calculation
                peak = self.get_audio_peak() if media["is_playing"] else 0.0
                # Scale 5 EQ bars between 1 and 8
                b0 = min(8, max(1, int(peak * 8.5 + random.uniform(0, 0.4))))
                b1 = min(8, max(1, int(peak * 10.0 + random.uniform(0, 0.5))))
                b2 = min(8, max(1, int(peak * 9.0 + random.uniform(0, 0.4))))
                b3 = min(8, max(1, int(peak * 7.5 + random.uniform(0, 0.3))))
                b4 = min(8, max(1, int(peak * 6.0 + random.uniform(0, 0.3))))

                # Get synchronized lyric line and tempo progress (0-100%)
                curr_lrc, next_lrc, prog_pct = get_lyrics_at_time(self.parsed_lyrics, media["pos_sec"])

                # Send LRC, EQ stream, and tempo progress packet every 90ms for silky fluid sync
                if now - self.last_lrc_time >= 0.09:
                    self.last_lrc_time = now
                    # Format: LRC:<current_line>|<next_line>|<b0>,<b1>,<b2>,<b3>,<b4>|<prog_pct>
                    lrc_packet = f"LRC:{curr_lrc}|{next_lrc}|{b0},{b1},{b2},{b3},{b4}|{prog_pct}"
                    self.send_to_esp32(lrc_packet)

                    # Live terminal display
                    vu_bars = int(peak * 12)
                    vu_meter = "█" * vu_bars + "░" * (12 - vu_bars)
                    m = int(media["pos_sec"] // 60)
                    s = int(media["pos_sec"] % 60)
                    time_tag = f"[{m:02d}:{s:02d}]"
                    lrc_display = curr_lrc if curr_lrc else "..."
                    print(f"\r{time_tag} [{vu_meter}] ♫ {lrc_display[:38]:<38}", end="", flush=True)

            else:
                if self.current_title != "":
                    self.current_title = ""
                    self.current_artist = ""
                    self.parsed_lyrics = []
                    self.send_to_esp32("SONG:STOP|||")
                    print("\r[IDLE] Waiting for Spotify or media playback...", end="", flush=True)

            await asyncio.sleep(0.08)

    def close(self):
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        print("\n[PC Companion] Disconnected.")

def main():
    parser = argparse.ArgumentParser(description="ESP32-C3 Spotify Synced Lyrics & Audio EQ Companion")
    parser.add_argument("--port", type=str, default=None, help="COM port (e.g. COM5)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    args = parser.parse_args()

    port = args.port or find_esp32_port()
    if not port:
        print("[Error] No COM port found. Please plug in your ESP32-C3 SuperMini via USB!")
        sys.exit(1)

    companion = MediaCompanion(port, args.baud)
    if not companion.connect():
        sys.exit(1)

    try:
        asyncio.run(companion.run())
    except KeyboardInterrupt:
        print("\n[PC Companion] Stopped by user.")
    finally:
        companion.close()

if __name__ == "__main__":
    main()
