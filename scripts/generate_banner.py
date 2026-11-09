import os
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageOps

def draw_pacman_sprite(draw, x, y, color=(255, 255, 255, 255)):
    # Crisp pixel Pac-Man facing right
    # 12x12 grid bitmap
    pac_bitmap = [
        "....XXXX....",
        "..XXXXXXXX..",
        ".XXXXXXXXXX.",
        "XXXXXXXXXXXX",
        "XXXXXXXXXXXX",
        "XXXXXXXX....",
        "XXXXXXXX....",
        "XXXXXXXXXXXX",
        "XXXXXXXXXXXX",
        ".XXXXXXXXXX.",
        "..XXXXXXXX..",
        "....XXXX...."
    ]
    for row_idx, row in enumerate(pac_bitmap):
        for col_idx, ch in enumerate(row):
            if ch == 'X':
                draw.rectangle([x + col_idx, y + row_idx, x + col_idx, y + row_idx], fill=color)

def draw_ghost_sprite(draw, x, y, color=(220, 225, 240, 255)):
    # 12x12 grid pixel ghost
    ghost_bitmap = [
        "....XXXX....",
        "..XXXXXXXX..",
        ".XXXXXXXXXX.",
        ".XX..XX..XX.",
        ".XX..XX..XX.",
        "XXXXXXXXXXXX",
        "XXXXXXXXXXXX",
        "XXXXXXXXXXXX",
        "XXXXXXXXXXXX",
        "XXXXXXXXXXXX",
        "X.XX.XX.XX.X",
        "X..X..X..X.X"
    ]
    for row_idx, row in enumerate(ghost_bitmap):
        for col_idx, ch in enumerate(row):
            if ch == 'X':
                draw.rectangle([x + col_idx, y + row_idx, x + col_idx, y + row_idx], fill=color)

def draw_battery_icon(draw, x, y, percent=92, color=(160, 170, 190, 255)):
    # Outer shell
    draw.rectangle([x, y, x + 24, y + 12], outline=color, width=1)
    draw.rectangle([x + 24, y + 3, x + 26, y + 9], fill=color)
    # Fill level
    fill_w = int(20 * (percent / 100.0))
    if fill_w > 0:
        draw.rectangle([x + 2, y + 2, x + 2 + fill_w, y + 10], fill=color)

def draw_ble_icon(draw, x, y, color=(160, 170, 190, 255)):
    # Stylized Bluetooth rune
    draw.line([(x + 5, y), (x + 5, y + 14)], fill=color, width=1)
    draw.line([(x + 5, y + 7), (x + 9, y + 3)], fill=color, width=1)
    draw.line([(x + 9, y + 3), (x + 5, y)], fill=color, width=1)
    draw.line([(x + 5, y + 7), (x + 9, y + 11)], fill=color, width=1)
    draw.line([(x + 9, y + 11), (x + 5, y + 14)], fill=color, width=1)
    draw.line([(x + 1, y + 4), (x + 5, y + 7)], fill=color, width=1)
    draw.line([(x + 1, y + 10), (x + 5, y + 7)], fill=color, width=1)

def create_banner():
    # Canvas Dimensions: High-Res 1600 x 640
    W, H = 1600, 640
    img = Image.new('RGBA', (W, H), (7, 8, 11, 255))
    draw = ImageDraw.Draw(img)

    # --- 1. Background Grid & Dot Matrix ---
    for x in range(36, W - 36, 28):
        for y in range(36, H - 36, 28):
            draw.rectangle([x, y, x+1, y+1], fill=(24, 27, 36, 255))

    # Subtle Circuit PCB Bus Lines in background (Dark Tech Gray)
    bus_color = (26, 32, 44, 255)
    # Top bus line with 45-degree chamfer
    draw.line([(60, 95), (420, 95), (475, 145), (1020, 145)], fill=bus_color, width=2)
    draw.ellipse([57, 92, 63, 98], fill=bus_color)
    draw.ellipse([1017, 142, 1023, 148], fill=bus_color)

    # Left PCB branch
    draw.line([(60, 95), (60, 160), (35, 185)], fill=bus_color, width=2)
    draw.ellipse([32, 182, 38, 188], fill=bus_color)

    # Bottom bus line
    draw.line([(80, 555), (680, 555), (735, 500), (1450, 500)], fill=bus_color, width=2)
    draw.ellipse([77, 552, 83, 558], fill=bus_color)
    draw.ellipse([1447, 497, 1453, 503], fill=bus_color)

    # --- 2. Tech Frame & Corner Brackets ---
    pad = 28
    border_color = (38, 44, 58, 255)
    draw.rectangle([pad, pad, W - pad, H - pad], outline=border_color, width=1)

    # Corner Crosshairs
    def draw_cross(cx, cy):
        draw.line([(cx - 8, cy), (cx + 8, cy)], fill=(210, 220, 235, 255), width=2)
        draw.line([(cx, cy - 8), (cx, cy + 8)], fill=(210, 220, 235, 255), width=2)

    draw_cross(pad, pad)
    draw_cross(W - pad, pad)
    draw_cross(pad, H - pad)
    draw_cross(W - pad, H - pad)

    # Top & Bottom accent lines
    draw.line([pad + 20, pad, pad + 90, pad], fill=(190, 200, 220, 255), width=2)
    draw.line([W - pad - 90, pad, W - pad - 20, pad], fill=(190, 200, 220, 255), width=2)
    draw.line([pad, pad + 20, pad, pad + 90], fill=(190, 200, 220, 255), width=2)
    draw.line([pad, H - pad - 90, pad, H - pad - 20], fill=(190, 200, 220, 255), width=2)

    # --- 3. Fonts ---
    font_path_bahn = 'C:/Windows/Fonts/bahnschrift.ttf'
    font_path_mono = 'C:/Windows/Fonts/consolab.ttf'

    font_header_tag = ImageFont.truetype(font_path_mono, 14)
    font_title = ImageFont.truetype(font_path_bahn, 82)
    font_subtitle = ImageFont.truetype(font_path_bahn, 22)
    font_desc = ImageFont.truetype(font_path_bahn, 15)
    font_badges = ImageFont.truetype(font_path_mono, 12)
    font_micro = ImageFont.truetype(font_path_mono, 12)
    font_oled_clock = ImageFont.truetype(font_path_bahn, 52)
    font_oled_sub = ImageFont.truetype(font_path_mono, 13)
    font_oled_lyric = ImageFont.truetype(font_path_bahn, 15)

    # Header Tag
    draw.text((pad + 30, pad + 14), '// ESP32-C3 RISC-V ARCHITECTURE // SSD1306 0.96\" OLED', fill=(130, 140, 160, 255), font=font_header_tag)
    draw.text((W - pad - 310, pad + 14), 'GITHUB: NOTHISIX / SIXWATCS', fill=(130, 140, 160, 255), font=font_header_tag)

    # --- 4. Glowing User Logo 'n6' (Left Area) ---
    logo_file = 'assets/sixwatch_logo.png' if os.path.exists('assets/sixwatch_logo.png') else 'logo_white_transparent.png'
    logo_src = Image.open(logo_file).convert('RGBA')
    logo_size = 230
    logo = logo_src.resize((logo_size, logo_size), Image.Resampling.LANCZOS)

    # Soft radial ambient glow behind logo
    glow_size = 400
    glow_img = Image.new('RGBA', (glow_size, glow_size), (0, 0, 0, 0))
    glow_draw = ImageDraw.Draw(glow_img)
    for r in range(glow_size // 2, 0, -4):
        alpha = int(45 * (1.0 - (r / (glow_size / 2)) ** 1.6))
        glow_draw.ellipse([glow_size//2 - r, glow_size//2 - r, glow_size//2 + r, glow_size//2 + r], fill=(255, 255, 255, alpha))
    glow_img = glow_img.filter(ImageFilter.GaussianBlur(15))

    logo_x = 90
    logo_y = 195
    img.paste(glow_img, (logo_x + logo_size//2 - glow_size//2, logo_y + logo_size//2 - glow_size//2), glow_img)
    img.paste(logo, (logo_x, logo_y), logo)

    # Clean Tech Frame around logo
    draw.rectangle([logo_x - 20, logo_y - 20, logo_x + logo_size + 20, logo_y + logo_size + 20], outline=(48, 54, 70, 255), width=1)
    draw.text((logo_x - 12, logo_y - 36), 'SIGNATURE LOGO : n6', fill=(120, 130, 150, 255), font=font_micro)

    # --- 5. Main Title & Typography (Center-Left) ---
    tx = 370
    ty = 175

    # System Status Pill Badge
    pill_w = 175
    draw.rounded_rectangle([tx, ty, tx + pill_w, ty + 26], radius=5, fill=(18, 22, 30, 255), outline=(55, 64, 82, 255), width=1)
    draw.ellipse([tx + 12, ty + 8, tx + 20, ty + 16], fill=(255, 255, 255, 255))
    draw.text((tx + 28, ty + 5), 'SYSTEM ONLINE  v2.4', fill=(225, 230, 245, 255), font=font_micro)

    # Big Title: SIXWATCS
    draw.text((tx, ty + 36), 'SIXWATCS', fill=(255, 255, 255, 255), font=font_title)

    # Subtitle
    draw.text((tx + 2, ty + 130), 'SMART OLED DESK COMPANION & RETRO ARCADE HUD', fill=(195, 205, 225, 255), font=font_subtitle)
    draw.text((tx + 2, ty + 165), 'Real-time Chronos BLE sync, Spotify synced lyrics with audio-reactive EQ, and mini-games.', fill=(130, 140, 160, 255), font=font_desc)

    # Feature Badges Pill Rows (2 clean rows to prevent any clipping!)
    badges_row1 = ['CHRONOS BLE CLOCK', 'SPOTIFY SYNCED LYRICS', 'AUDIO REACTIVE EQ']
    badges_row2 = ['RETRO ARCADE GAMES', 'HANGUL + LATIN ENGINE', 'OPEN SOURCE MIT']
    
    bx = tx
    by = ty + 200
    for b in badges_row1:
        bbox = draw.textbbox((0, 0), b, font=font_badges)
        bw = bbox[2] - bbox[0] + 16
        draw.rounded_rectangle([bx, by, bx + bw, by + 26], radius=4, fill=(16, 20, 28, 255), outline=(50, 58, 76, 255), width=1)
        draw.text((bx + 8, by + 5), b, fill=(215, 225, 240, 255), font=font_badges)
        bx += bw + 10

    bx = tx
    by = ty + 234
    for b in badges_row2:
        bbox = draw.textbbox((0, 0), b, font=font_badges)
        bw = bbox[2] - bbox[0] + 16
        draw.rounded_rectangle([bx, by, bx + bw, by + 26], radius=4, fill=(16, 20, 28, 255), outline=(50, 58, 76, 255), width=1)
        draw.text((bx + 8, by + 5), b, fill=(215, 225, 240, 255), font=font_badges)
        bx += bw + 10

    # --- 6. Realistic OLED Display Screen Module (Right Area) ---
    oled_x = 1040
    oled_y = 165
    oled_w = 490
    oled_h = 295

    # PCB Board Frame with mounting holes
    draw.rounded_rectangle([oled_x - 14, oled_y - 14, oled_x + oled_w + 14, oled_y + oled_h + 14], radius=12, fill=(13, 15, 20, 255), outline=(60, 68, 86, 255), width=2)
    for hx, hy in [(oled_x - 4, oled_y - 4), (oled_x + oled_w + 4, oled_y - 4), (oled_x - 4, oled_y + oled_h + 4), (oled_x + oled_w + 4, oled_y + oled_h + 4)]:
        draw.ellipse([hx - 7, hy - 7, hx + 7, hy + 7], outline=(150, 130, 90, 255), width=2)
        draw.ellipse([hx - 4, hy - 4, hx + 4, hy + 4], fill=(8, 9, 12, 255))

    # OLED Deep Noir Glass Surface
    draw.rounded_rectangle([oled_x, oled_y, oled_x + oled_w, oled_y + oled_h], radius=6, fill=(2, 2, 4, 255), outline=(95, 105, 128, 255), width=1)

    # OLED Top Header Bar
    draw.text((oled_x + 18, oled_y + 14), 'HUD v2.4', fill=(150, 160, 180, 255), font=font_oled_sub)
    draw_ble_icon(draw, oled_x + oled_w - 170, oled_y + 14)
    draw.text((oled_x + oled_w - 150, oled_y + 14), 'BLE SYNC', fill=(150, 160, 180, 255), font=font_oled_sub)
    draw_battery_icon(draw, oled_x + oled_w - 55, oled_y + 15, percent=92)
    draw.line([(oled_x + 14, oled_y + 36), (oled_x + oled_w - 14, oled_y + 36)], fill=(40, 48, 64, 255), width=1)

    # Big Digital Clock
    draw.text((oled_x + 24, oled_y + 50), '14:38:26', fill=(255, 255, 255, 255), font=font_oled_clock)
    draw.text((oled_x + 295, oled_y + 64), 'MON 09 NOV', fill=(190, 200, 220, 255), font=font_oled_sub)
    draw.text((oled_x + 295, oled_y + 84), '28 C  82% RH', fill=(140, 150, 170, 255), font=font_oled_sub)

    # Middle Divider
    draw.line([(oled_x + 14, oled_y + 128), (oled_x + oled_w - 14, oled_y + 128)], fill=(40, 48, 64, 255), width=1)

    # Active Music & Synced Lyric Preview
    draw.text((oled_x + 20, oled_y + 142), '>> SPOTIFY NOW PLAYING', fill=(130, 140, 165, 255), font=font_oled_sub)
    draw.text((oled_x + 20, oled_y + 165), '\"cherry flavoured love inside your heart\"', fill=(255, 255, 255, 255), font=font_oled_lyric)
    draw.text((oled_x + 20, oled_y + 192), 'wave to earth  //  [Hangul + Sync OK]', fill=(160, 170, 195, 255), font=font_oled_sub)

    # Bottom Audio Equalizer (14 animated bars)
    eq_bars = [6, 12, 22, 32, 40, 36, 24, 34, 42, 28, 18, 10, 6, 4]
    eq_start_x = oled_x + 20
    eq_base_y = oled_y + oled_h - 22
    for i, h_val in enumerate(eq_bars):
        bar_x = eq_start_x + i * 15
        draw.rectangle([bar_x, eq_base_y - h_val, bar_x + 9, eq_base_y], fill=(230, 240, 255, 255))
        # Peak indicator dot
        draw.rectangle([bar_x, eq_base_y - h_val - 5, bar_x + 9, eq_base_y - h_val - 3], fill=(255, 255, 255, 255))

    # Arcade Sprite animation on the right side of the EQ
    # Pacman and Ghost pixel sprites
    pac_x = oled_x + oled_w - 180
    pac_y = eq_base_y - 24
    draw_pacman_sprite(draw, pac_x, pac_y, color=(255, 255, 255, 255))
    # Pacman dots
    for dot_x in [pac_x + 22, pac_x + 36, pac_x + 50]:
        draw.rectangle([dot_x, pac_y + 4, dot_x + 3, pac_y + 7], fill=(255, 255, 255, 255))
    draw_ghost_sprite(draw, pac_x + 65, pac_y, color=(220, 225, 240, 255))
    draw.text((pac_x + 85, pac_y - 1), 'ARCADE', fill=(150, 160, 180, 255), font=font_oled_sub)

    # OLED Label under the module
    draw.text((oled_x, oled_y + oled_h + 20), 'SSD1306 0.96\" I2C OLED (128x64 MONOCHROME)', fill=(120, 130, 150, 255), font=font_micro)

    # --- 7. Bottom Hardware Specs Bar ---
    draw.line([(pad + 20, H - pad - 42), (W - pad - 20, H - pad - 42)], fill=(32, 36, 48, 255), width=1)
    specs_text = 'HARDWARE: ESP32-C3 SUPERMINI (RISC-V 160MHz)  |  FLASH: 4MB  |  DISPLAY: 128x64 OLED  |  PROTOCOL: I2C (0x3C)  |  COMPANION: PYTHON WINSDK  |  LICENSE: MIT'
    draw.text((pad + 30, H - pad - 30), specs_text, fill=(110, 120, 140, 255), font=font_micro)

    os.makedirs('assets', exist_ok=True)
    out_path = 'assets/sixwatch_banner.png'
    img.save(out_path, 'PNG')
    print(f'Banner successfully generated and saved to {out_path}!')

if __name__ == '__main__':
    create_banner()
