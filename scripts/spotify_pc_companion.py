"""
ESP32-C3 Retro Arcade & OLED Watch - Spotify Real-time Synced Lyrics & Audio EQ
================================================================================
1. High-Performance Dedicated Worker Thread with WinSta0 Desktop Attachment.
2. Synchronized LRC lyrics from LRCLIB streaming directly to ESP32 OLED.
3. Live audio-reactive 5-band Equalizer using Windows Core Audio (pycaw).
4. Full hardware button control: Left=Prev, OK=Play/Pause, Right=Next.
"""

import sys
import os
import time
import threading
import argparse
import ctypes
from ctypes import wintypes
import random
import re
import requests
import serial
import serial.tools.list_ports

try:
    import psutil
    HAS_PSUTIL = True
except ImportError:
    HAS_PSUTIL = False

try:
    from pycaw.pycaw import AudioUtilities, IAudioMeterInformation
    HAS_PYCAW = True
except ImportError:
    HAS_PYCAW = False

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

user32 = ctypes.windll.user32

# Virtual key codes
VK_MEDIA_NEXT_TRACK = 0xB0
VK_MEDIA_PREV_TRACK = 0xB1
VK_MEDIA_PLAY_PAUSE = 0xB3
KEYEVENTF_KEYUP = 0x0002

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


class MediaCompanion:
    def __init__(self, port, baud=115200):
        self.port = port
        self.baud = baud
        self.ser = None
        self.running = True

        # Thread-shared detected media info
        self.lock = threading.Lock()
        self.detected_artist = ""
        self.detected_title = ""
        self.is_playing = False

        self.current_title = ""
        self.current_artist = ""
        self.last_state_str = ""
        self.parsed_lyrics = []
        self.lyrics_cache = {}

        self.track_elapsed = 0.0
        self.last_tick = 0.0

        self.last_lrc_time = 0
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
                print(f" [PORT TERKUNCI] Port {self.port} sedang dipakai oleh program lain!")
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
                        if cmd == "TOGGLE":
                            send_windows_media_key(VK_MEDIA_PLAY_PAUSE)
                        elif cmd == "NEXT":
                            send_windows_media_key(VK_MEDIA_NEXT_TRACK)
                        elif cmd == "PREV":
                            send_windows_media_key(VK_MEDIA_PREV_TRACK)
            except Exception:
                if self.running:
                    time.sleep(0.1)

    def window_scanner_thread(self):
        """Dedicated thread with exclusive WinSta0\\Default desktop attachment."""
        try:
            winsta = user32.OpenWindowStationW("WinSta0", False, 0x037F)
            if winsta:
                user32.SetProcessWindowStation(winsta)
            desktop = user32.OpenDesktopW("Default", 0, False, 0x01FF)
            if desktop:
                user32.SetThreadDesktop(desktop)
        except Exception:
            pass

        while self.running:
            try:
                spotify_pids = set()
                if HAS_PSUTIL:
                    for p in psutil.process_iter(['pid', 'name']):
                        if 'spotify' in p.info['name'].lower():
                            spotify_pids.add(p.info['pid'])

                found_artist = ""
                found_title = ""
                playing = False

                hwnd = user32.GetTopWindow(None)
                while hwnd:
                    pid = wintypes.DWORD()
                    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
                    
                    if pid.value in spotify_pids:
                        length = user32.GetWindowTextLengthW(hwnd)
                        if length > 0:
                            buff = ctypes.create_unicode_buffer(length + 1)
                            user32.GetWindowTextW(hwnd, buff, length + 1)
                            w_title = buff.value.strip()

                            delims = [" - ", " \u2013 ", " \u2014 "]
                            for d in delims:
                                if d in w_title:
                                    parts = w_title.split(d, 1)
                                    if len(parts) == 2 and parts[0].strip() and parts[1].strip():
                                        found_artist = parts[0].strip()
                                        found_title = parts[1].strip()
                                        playing = True
                                        break
                            if playing:
                                break
                    hwnd = user32.GetWindow(hwnd, 2)

                with self.lock:
                    self.detected_artist = found_artist
                    self.detected_title = found_title
                    self.is_playing = playing

            except Exception:
                pass

            time.sleep(0.08)

    def get_audio_peak(self):
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

        # 1. Exact match
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

        # 2. Cleaned title & artist
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

        # 3. Search query
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

        print(f"[Lyrics] No synced lyrics found for '{title}' by '{artist}'.")
        fallback = [
            (0.0, "(No synced lyrics found)"),
            (8.0, "(Enjoy the music ~)")
        ]
        self.lyrics_cache[cache_key] = fallback
        return fallback

    def run(self):
        # 1. Start Serial Listener thread
        t_serial = threading.Thread(target=self.serial_listener_thread, daemon=True)
        t_serial.start()

        # 2. Start Window Scanner thread with WinSta0 desktop access
        t_scanner = threading.Thread(target=self.window_scanner_thread, daemon=True)
        t_scanner.start()

        print("\n" + "=" * 62)
        print("   SIXWATCS - ESP32-C3 SPOTIFY SYNCED LYRICS & AUDIO EQ")
        print("=" * 62)
        print(f" Serial Port : {self.port}")
        print(" Engine      : Win32 Worker Thread (Zero Latency)")
        print(" Lyrics API  : LRCLIB (Real-time synchronized LRC)")
        print(" Audio Meter : Windows Core Audio Peak (pycaw)")
        print(" Controls    : Left=Prev | OK=Play/Pause | Right=Next")
        print(" Press Ctrl+C in this terminal to exit.")
        print("=" * 62 + "\n")

        self.last_tick = time.time()

        while self.running:
            with self.lock:
                artist = self.detected_artist
                title = self.detected_title
                is_playing = self.is_playing

            now = time.time()
            dt = max(0.0, now - self.last_tick)
            self.last_tick = now

            if is_playing and title:
                self.printed_idle = False
                state_str = "PLAY"
                track_id = f"{artist}|{title}"
                is_new_track = (track_id != f"{self.current_artist}|{self.current_title}")

                if is_new_track or (self.last_state_str != state_str):
                    self.current_title = title
                    self.current_artist = artist
                    self.last_state_str = state_str
                    self.track_elapsed = 0.0

                    safe_artist = self.current_artist.replace('|', '/')
                    safe_title = self.current_title.replace('|', '/')
                    song_pkt = f"SONG:PLAY|{safe_artist}|{safe_title}|SPOTIFY"
                    self.send_to_esp32(song_pkt)

                    if is_new_track:
                        self.send_to_esp32("LRC:Searching lyrics...||3,4,5,4,3|0|1000")
                        print(f"\n[Track Playing] {self.current_title} - {self.current_artist}")
                        self.parsed_lyrics = self.fetch_lyrics(self.current_artist, self.current_title)
                    else:
                        print(f"\n[Resumed] {self.current_title}")

                # Accumulate real-time elapsed seconds
                self.track_elapsed += dt

                # Audio-reactive peak calculation
                peak = self.get_audio_peak()
                b0 = min(8, max(1, int(peak * 8.5 + random.uniform(0, 0.4))))
                b1 = min(8, max(1, int(peak * 10.0 + random.uniform(0, 0.5))))
                b2 = min(8, max(1, int(peak * 9.0 + random.uniform(0, 0.4))))
                b3 = min(8, max(1, int(peak * 7.5 + random.uniform(0, 0.3))))
                b4 = min(8, max(1, int(peak * 6.0 + random.uniform(0, 0.3))))

                # Real-time lyrics progression
                curr_lrc, next_lrc, elapsed_ms, duration_ms = get_lyrics_at_time(self.parsed_lyrics, self.track_elapsed)

                # Send LRC every 35ms (~28 FPS)
                if now - self.last_lrc_time >= 0.035:
                    self.last_lrc_time = now
                    lrc_packet = f"LRC:{curr_lrc}|{next_lrc}|{b0},{b1},{b2},{b3},{b4}|{elapsed_ms}|{duration_ms}"
                    self.send_to_esp32(lrc_packet)

                    vu_bars = int(peak * 12)
                    vu_meter = "█" * vu_bars + "░" * (12 - vu_bars)
                    m = int(self.track_elapsed // 60)
                    s = int(self.track_elapsed % 60)
                    time_tag = f"[{m:02d}:{s:02d}]"
                    lrc_display = curr_lrc if curr_lrc else "..."
                    print(f"\r{time_tag} [{vu_meter}] ♫ {lrc_display[:38]:<38}", end="", flush=True)

            else:
                if self.current_title != "":
                    self.current_title = ""
                    self.current_artist = ""
                    self.last_state_str = "PAUSE"
                    self.parsed_lyrics = []
                    self.send_to_esp32("SONG:PAUSE|||")
                    print("\r[PAUSED] Spotify dihentikan sementara...", end="", flush=True)
                elif not self.printed_idle:
                    print("\r[IDLE] Menunggu lagu diputar di Spotify...", end="", flush=True)
                    self.printed_idle = True

            time.sleep(0.035)

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
        companion.run()
    except KeyboardInterrupt:
        print("\n[PC Companion] Dihentikan oleh user.")
    finally:
        companion.close()

if __name__ == "__main__":
    main()
