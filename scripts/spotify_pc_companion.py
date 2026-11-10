"""
ESP32-C3 Retro Arcade & OLED Watch - Spotify Real-time Synced Lyrics & Audio EQ
================================================================================
1. Streams real-time synchronized LRC lyrics from LRCLIB to the ESP32 OLED display.
2. Captures Windows system audio volume (pycaw) to drive audio-reactive equalizer bars.
3. Listens for arcade hardware buttons (Left=Prev, OK=Play/Pause, Right=Next) to control Spotify.
4. Resilient Dual-Engine: WinRT Media Session + Win32 Spotify Window Title fallback.
"""

import sys
import os
import time
import datetime
import asyncio
import threading
import argparse
import ctypes
from ctypes import wintypes
import random
import re
import requests
import serial
import serial.tools.list_ports

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

# WinRT Windows Media Session
try:
    from winsdk.windows.media.control import (
        GlobalSystemMediaTransportControlsSessionManager as MediaManager,
        GlobalSystemMediaTransportControlsSessionPlaybackStatus as PlaybackStatus
    )
    HAS_WINSDK = True
except ImportError:
    HAS_WINSDK = False

# Windows Core Audio Peak Metering
try:
    from pycaw.pycaw import AudioUtilities, IAudioMeterInformation
    HAS_PYCAW = True
except ImportError:
    HAS_PYCAW = False

# Psutil for process checking
try:
    import psutil
    HAS_PSUTIL = True
except ImportError:
    HAS_PSUTIL = False

# Windows Virtual-Key Codes for Media Control fallback
VK_MEDIA_NEXT_TRACK = 0xB0
VK_MEDIA_PREV_TRACK = 0xB1
VK_MEDIA_PLAY_PAUSE = 0xB3
KEYEVENTF_KEYUP = 0x0002

user32 = ctypes.windll.user32

LRCLIB_HEADERS = {
    "User-Agent": "SIXWATCS-Companion/2.0 (ESP32-C3 Arcade Watch; https://github.com/nothisix/sixwatcs)"
}

def send_windows_media_key(vk_code):
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

def normalize_text(text):
    if not text:
        return ""
    t = text
    t = t.replace('|', '/')
    t = t.replace('’', "'").replace('‘', "'").replace('`', "'")
    t = t.replace('“', '"').replace('”', '"')
    t = t.replace('—', '-').replace('–', '-')
    t = t.replace('…', '...')
    t = t.replace('♪', '~').replace('♫', '~')
    return t.strip()

def clean_title(title):
    if not title:
        return ""
    t = title.replace('|', '/')
    t = re.sub(r'\(feat\.[^)]*\)', '', t, flags=re.I)
    t = re.sub(r'\(with[^)]*\)', '', t, flags=re.I)
    t = re.sub(r'\[feat\.[^\]]*\]', '', t, flags=re.I)
    t = re.sub(r'\[with[^\]]*\]', '', t, flags=re.I)
    t = re.sub(r' - [0-9]{4} Remaster.*', '', t, flags=re.I)
    t = re.sub(r' - Remaster.*', '', t, flags=re.I)
    t = re.sub(r'\(.*?remaster.*?\)', '', t, flags=re.I)
    t = re.sub(r'\[.*?remaster.*?\]', '', t, flags=re.I)
    t = re.sub(r'\(.*?deluxe.*?\)', '', t, flags=re.I)
    t = re.sub(r'\(.*?version.*?\)', '', t, flags=re.I)
    t = re.sub(r' - Live.*', '', t, flags=re.I)
    t = re.sub(r' - Bonus Track.*', '', t, flags=re.I)
    t = re.sub(r' - Single Version.*', '', t, flags=re.I)
    t = re.sub(r' - Radio Edit.*', '', t, flags=re.I)
    t = re.sub(r' - Explicit.*', '', t, flags=re.I)
    t = re.sub(r' - Clean.*', '', t, flags=re.I)
    t = re.sub(r' - Original.*', '', t, flags=re.I)
    if ' - ' in t:
        parts = t.split(' - ')
        if len(parts[0].strip()) >= 2:
            t = parts[0]
    return t.strip()

def clean_artist(artist):
    if not artist:
        return ""
    a = artist.replace('|', '/')
    a = a.split(',')[0]
    a = a.split('&')[0]
    a = re.sub(r'\s+(feat\.|ft\.|with|x)\s+.*', '', a, flags=re.I)
    return a.strip()

def parse_lrc(text):
    """Parses LRC timestamped lyrics into clean (seconds, text) tuples."""
    if not text:
        return []
    result = []
    lines = text.splitlines()
    for line in lines:
        line = line.strip()
        if line.startswith('[') and ']' in line:
            end_bracket = line.index(']')
            time_part = line[1:end_bracket]
            raw_lyric = line[end_bracket + 1:].strip()
            lyric_part = normalize_text(raw_lyric)
            parts = time_part.split(':')
            if len(parts) == 2:
                try:
                    m = float(parts[0])
                    s = float(parts[1])
                    t_sec = m * 60.0 + s
                    if lyric_part:
                        result.append((t_sec, lyric_part))
                    else:
                        result.append((t_sec, "~ (Instrumental) ~"))
                except ValueError:
                    pass
    result.sort(key=lambda x: x[0])
    return result

def get_lyrics_at_time(parsed_lrc, pos_sec):
    """Finds current active line, next upcoming line, elapsed_ms, and duration_ms."""
    if not parsed_lrc:
        return "(No synced lyrics found)", "(Enjoy the music ~)", 0, 1000

    curr_idx = -1
    for i, (t, text) in enumerate(parsed_lrc):
        if t <= pos_sec:
            curr_idx = i
        else:
            break

    if curr_idx == -1:
        first_line = parsed_lrc[0][1] if parsed_lrc else ""
        first_t = parsed_lrc[0][0] if parsed_lrc else 1.0
        elapsed_ms = int(max(0.0, pos_sec) * 1000)
        duration_ms = int(max(0.3, first_t) * 1000)
        return "~ (Intro) ~", first_line, elapsed_ms, duration_ms

    t_start, curr_line = parsed_lrc[curr_idx]

    if curr_idx + 1 < len(parsed_lrc):
        t_end, next_line = parsed_lrc[curr_idx + 1]
    else:
        t_end = t_start + 4.5
        next_line = "~ (Outro) ~"

    duration = max(0.25, t_end - t_start)
    elapsed = max(0.0, pos_sec - t_start)
    elapsed_ms = int(elapsed * 1000)
    duration_ms = int(duration * 1000)

    return curr_line, next_line, elapsed_ms, duration_ms

# Win32 Window Fallback for Spotify
def get_spotify_track_from_win32():
    """Inspects Spotify desktop window title directly as fallback."""
    found_info = None

    def enum_cb(hwnd, lparam):
        nonlocal found_info
        length = user32.GetWindowTextLengthW(hwnd)
        if length > 0:
            buff = ctypes.create_unicode_buffer(length + 1)
            user32.GetWindowTextW(hwnd, buff, length + 1)
            title = buff.value
            
            cls_buff = ctypes.create_unicode_buffer(256)
            user32.GetClassNameW(hwnd, cls_buff, 256)
            cls_name = cls_buff.value

            if cls_name == "Chrome_WidgetWin_0":
                delims = [" - ", " \u2013 ", " \u2014 "]
                for d in delims:
                    if d in title:
                        parts = title.split(d, 1)
                        if len(parts) == 2 and len(parts[0].strip()) > 0 and len(parts[1].strip()) > 0:
                            found_info = {
                                "artist": parts[0].strip(),
                                "title": parts[1].strip(),
                                "is_playing": True,
                                "app": "SPOTIFY",
                                "source": "win32"
                            }
                            return 0
                if title.strip().lower() in ["spotify", "spotify free", "spotify premium"]:
                    if found_info is None:
                        found_info = {
                            "artist": "",
                            "title": "",
                            "is_playing": False,
                            "app": "SPOTIFY",
                            "source": "win32"
                        }
        return 1

    try:
        WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        cb = WNDENUMPROC(enum_cb)
        user32.EnumWindows(cb, 0)
    except Exception:
        pass

    return found_info

class MediaCompanion:
    def __init__(self, port, baud=115200):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = True

        self.media_manager = None
        self.current_session = None
        self.current_title = ""
        self.current_artist = ""
        self.last_state_str = ""
        self.parsed_lyrics = []
        self.lyrics_cache = {}

        self.last_song_packet = ""
        self.last_lrc_sent = ""
        self.last_lrc_time = 0

        # Estimated position tracking for win32 fallback
        self.estimated_pos = 0.0
        self.last_pos_update = 0

        self.audio_meter = None
        self.last_meter_refresh = 0
        self.printed_idle = False

    def connect(self):
        print(f"[PC Companion] Connecting to ESP32 on {self.port} at {self.baud} baud...")
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=0.1)
            time.sleep(0.8)
            print(f"[PC Companion] Connected successfully to {self.port}!")
            return True
        except serial.SerialException as e:
            err_msg = str(e)
            print(f"\n[Error] Gagal membuka port {self.port}: {e}")
            if "PermissionError" in err_msg or "Access is denied" in err_msg or "5" in err_msg:
                print("=" * 65)
                print(f" [PORT TERKUNCI] Port {self.port} sedang dipakai aplikasi lain!")
                print(" --> Jika web SIXWATCS terbuka di browser:")
                print("     Buka tab 'Device Configurator (Web Serial)' lalu klik 'Disconnect'!")
                print("     Browser mengunci USB secara eksklusif sehingga script tidak bisa masuk.")
                print("=" * 65 + "\n")
            return False
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
        if not self.audio_meter or (now - self.last_meter_refresh > 4.0):
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

        c_art = clean_artist(artist)
        c_tit = clean_title(title)
        print(f"\n[Lyrics] Searching synced lyrics for '{title}' by '{artist}'...")

        # 1. Exact match with original artist & title
        try:
            r = requests.get(
                "https://lrclib.net/api/get",
                params={"artist_name": artist, "track_name": title},
                headers=LRCLIB_HEADERS,
                timeout=3.0
            )
            if r.status_code == 200:
                data = r.json()
                if data.get("syncedLyrics"):
                    parsed = parse_lrc(data["syncedLyrics"])
                    if parsed:
                        print(f"[Lyrics] Found {len(parsed)} synced lyric lines (Exact Match)!")
                        self.lyrics_cache[cache_key] = parsed
                        return parsed
        except Exception:
            pass

        # 2. Exact match with cleaned artist & title
        if (c_art != artist) or (c_tit != title):
            try:
                r = requests.get(
                    "https://lrclib.net/api/get",
                    params={"artist_name": c_art, "track_name": c_tit},
                    headers=LRCLIB_HEADERS,
                    timeout=3.0
                )
                if r.status_code == 200:
                    data = r.json()
                    if data.get("syncedLyrics"):
                        parsed = parse_lrc(data["syncedLyrics"])
                        if parsed:
                            print(f"[Lyrics] Found {len(parsed)} synced lyric lines (Cleaned: '{c_tit}' by '{c_art}')!")
                            self.lyrics_cache[cache_key] = parsed
                            return parsed
            except Exception:
                pass

        # 3. Search query with clean artist + clean title
        search_query = f"{c_art} {c_tit}".strip()
        if search_query:
            try:
                r = requests.get(
                    "https://lrclib.net/api/search",
                    params={"q": search_query},
                    headers=LRCLIB_HEADERS,
                    timeout=3.0
                )
                if r.status_code == 200:
                    items = r.json()
                    for item in items:
                        if item.get("syncedLyrics"):
                            parsed = parse_lrc(item["syncedLyrics"])
                            if parsed:
                                print(f"[Lyrics] Found {len(parsed)} synced lyric lines via search ('{search_query}')!")
                                self.lyrics_cache[cache_key] = parsed
                                return parsed
            except Exception:
                pass

        # 4. Search query with clean title only
        if c_tit and c_tit != search_query:
            try:
                r = requests.get(
                    "https://lrclib.net/api/search",
                    params={"q": c_tit},
                    headers=LRCLIB_HEADERS,
                    timeout=3.0
                )
                if r.status_code == 200:
                    items = r.json()
                    for item in items:
                        if item.get("syncedLyrics"):
                            parsed = parse_lrc(item["syncedLyrics"])
                            if parsed:
                                print(f"[Lyrics] Found {len(parsed)} synced lyric lines via title search ('{c_tit}')!")
                                self.lyrics_cache[cache_key] = parsed
                                return parsed
            except Exception:
                pass

        print(f"[Lyrics] No synced lyrics found for '{title}' by '{artist}'.")
        fallback = [
            (0.0, "(No synced lyrics found)"),
            (8.0, "(Enjoy the music ~)")
        ]
        self.lyrics_cache[cache_key] = fallback
        return fallback

    async def get_media_manager(self):
        """Safely gets or refreshes WinRT MediaManager with timeout guard."""
        if not HAS_WINSDK:
            return None
        if self.media_manager:
            return self.media_manager
        try:
            self.media_manager = await asyncio.wait_for(MediaManager.request_async(), timeout=2.0)
            return self.media_manager
        except Exception:
            self.media_manager = None
            return None

    async def get_current_media_info(self):
        """Detects playing media using WinRT SMTC with fallback to Win32 Spotify Window Title."""
        now = time.time()

        # Engine A: WinRT SMTC
        if HAS_WINSDK:
            try:
                mgr = await self.get_media_manager()
                if mgr:
                    chosen_session = None
                    is_playing = False
                    
                    # 1. Prefer current session if playing
                    curr = mgr.get_current_session()
                    if curr:
                        pb = curr.get_playback_info()
                        if pb and pb.playback_status == PlaybackStatus.PLAYING:
                            chosen_session = curr
                            is_playing = True

                    # 2. Iterate all sessions to find Spotify or any playing media
                    if not chosen_session:
                        sessions = mgr.get_sessions()
                        for s in sessions:
                            app_id = (s.source_app_user_model_id or "").lower()
                            try:
                                pb = s.get_playback_info()
                                pb_status = pb.playback_status if pb else None
                                if "spotify" in app_id:
                                    chosen_session = s
                                    is_playing = (pb_status == PlaybackStatus.PLAYING)
                                    if is_playing:
                                        break
                                elif pb_status == PlaybackStatus.PLAYING and not chosen_session:
                                    chosen_session = s
                                    is_playing = True
                            except Exception:
                                continue

                    if chosen_session:
                        self.current_session = chosen_session
                        props = await asyncio.wait_for(chosen_session.try_get_media_properties_async(), timeout=0.8)
                        title = props.title.strip() if (props and props.title) else ""
                        artist = props.artist.strip() if (props and props.artist) else ""
                        
                        if title:
                            app_id = (chosen_session.source_app_user_model_id or "").lower()
                            app_name = "SPOTIFY" if "spotify" in app_id else "MEDIA"
                            
                            pos_sec = 0.0
                            try:
                                timeline = chosen_session.get_timeline_properties()
                                if timeline and timeline.position:
                                    pos_sec = timeline.position.total_seconds()
                                if is_playing and timeline and timeline.last_updated_time:
                                    now_utc = datetime.datetime.now(datetime.timezone.utc)
                                    elapsed = (now_utc - timeline.last_updated_time).total_seconds()
                                    if 0.0 <= elapsed < 15.0:
                                        pos_sec += elapsed
                                if is_playing:
                                    pos_sec += 0.15
                            except Exception:
                                pos_sec = self.estimated_pos

                            self.estimated_pos = pos_sec
                            self.last_pos_update = now

                            return {
                                "title": title,
                                "artist": artist,
                                "app": app_name,
                                "is_playing": is_playing,
                                "pos_sec": pos_sec,
                                "source": "winrt"
                            }
            except Exception:
                # If WinRT fails or hangs, reset manager cache to avoid persistent freeze
                self.media_manager = None

        # Engine B: Win32 Window Title Fallback
        win32_track = get_spotify_track_from_win32()
        if win32_track and win32_track.get("title"):
            track_title = win32_track["title"]
            track_artist = win32_track["artist"]

            # Update estimated position
            if (track_title != self.current_title) or (track_artist != self.current_artist):
                self.estimated_pos = 0.0
                self.last_pos_update = now
            else:
                if self.last_pos_update > 0 and win32_track["is_playing"]:
                    self.estimated_pos += (now - self.last_pos_update)
                self.last_pos_update = now

            return {
                "title": track_title,
                "artist": track_artist,
                "app": "SPOTIFY",
                "is_playing": win32_track["is_playing"],
                "pos_sec": self.estimated_pos,
                "source": "win32"
            }

        return None

    async def run(self):
        t = threading.Thread(target=self.serial_listener_thread, daemon=True)
        t.start()

        print("\n" + "=" * 62)
        print("   SIXWATCS - ESP32-C3 SPOTIFY SYNCED LYRICS & AUDIO EQ")
        print("=" * 62)
        print(f" Serial Port : {self.port}")
        print(" Lyrics API  : LRCLIB (Real-time synchronized LRC)")
        print(" Audio Meter : Windows Core Audio Peak (pycaw)")
        print(" Detection   : Dual Engine (WinRT SMTC + Win32 Fallback)")
        print(" Controls    : Left=Prev | OK=Play/Pause | Right=Next")
        print(" Press Ctrl+C in this terminal to exit.")
        print("=" * 62 + "\n")

        print("[Status] Menghubungkan ke media player / Spotify...", flush=True)

        while self.running:
            media = await self.get_current_media_info()
            now = time.time()

            if media:
                self.printed_idle = False
                state_str = "PLAY" if media["is_playing"] else "PAUSE"
                track_id = f"{media['artist']}|{media['title']}"
                is_new_track = (track_id != f"{self.current_artist}|{self.current_title}")
                state_changed = (state_str != self.last_state_str)

                # 1. New track detected OR Play/Pause state toggled
                if is_new_track or state_changed:
                    self.current_title = media["title"]
                    self.current_artist = media["artist"]
                    self.last_state_str = state_str

                    safe_artist = self.current_artist.replace('|', '/')
                    safe_title = self.current_title.replace('|', '/')
                    song_pkt = f"SONG:{state_str}|{safe_artist}|{safe_title}|{media['app']}"
                    self.send_to_esp32(song_pkt)
                    self.last_song_packet = song_pkt

                    if is_new_track:
                        # Clear old lyrics immediately and inform OLED
                        self.send_to_esp32("LRC:Searching lyrics...||3,4,5,4,3|0|1000")
                        print(f"\n[Track] {self.current_title} - {self.current_artist} [{state_str}] ({media['source']})")
                        self.parsed_lyrics = self.fetch_lyrics(self.current_artist, self.current_title)
                    else:
                        print(f"\n[State Change] {state_str} -> {self.current_title}")

                # Audio-reactive peak calculation
                peak = self.get_audio_peak() if media["is_playing"] else 0.0
                b0 = min(8, max(1, int(peak * 8.5 + random.uniform(0, 0.4))))
                b1 = min(8, max(1, int(peak * 10.0 + random.uniform(0, 0.5))))
                b2 = min(8, max(1, int(peak * 9.0 + random.uniform(0, 0.4))))
                b3 = min(8, max(1, int(peak * 7.5 + random.uniform(0, 0.3))))
                b4 = min(8, max(1, int(peak * 6.0 + random.uniform(0, 0.3))))

                # Get synchronized lyric line, next line, and continuous tempo timing
                curr_lrc, next_lrc, elapsed_ms, duration_ms = get_lyrics_at_time(self.parsed_lyrics, media["pos_sec"])

                # Send LRC, EQ stream, and timing packet every 35ms (~28 FPS)
                if now - self.last_lrc_time >= 0.035:
                    self.last_lrc_time = now
                    lrc_packet = f"LRC:{curr_lrc}|{next_lrc}|{b0},{b1},{b2},{b3},{b4}|{elapsed_ms}|{duration_ms}"
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
                    self.last_state_str = ""
                    self.parsed_lyrics = []
                    self.send_to_esp32("SONG:STOP|||")
                    print("\r[IDLE] Lagu berhenti. Menunggu lagu diputar di Spotify...", end="", flush=True)
                elif not self.printed_idle:
                    print("\r[IDLE] Menunggu lagu diputar di Spotify / Media Player...", end="", flush=True)
                    self.printed_idle = True

            await asyncio.sleep(0.08)

    def close(self):
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()
        print("\n[PC Companion] Disconnected.")

def main():
    parser = argparse.ArgumentParser(description="SIXWATCS - ESP32-C3 Spotify Synced Lyrics & Audio EQ Companion")
    parser.add_argument("--port", type=str, default=None, help="COM port (e.g. COM5)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    args = parser.parse_args()

    port = args.port or find_esp32_port()
    if not port:
        print("[Error] Tidak menemukan COM port. Colokkan kabel USB ESP32-C3 kamu!")
        sys.exit(1)

    companion = MediaCompanion(port, args.baud)
    if not companion.connect():
        sys.exit(1)

    try:
        asyncio.run(companion.run())
    except KeyboardInterrupt:
        print("\n[PC Companion] Dihentikan oleh user.")
    finally:
        companion.close()

if __name__ == "__main__":
    main()
