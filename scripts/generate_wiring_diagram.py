import os
import math
from PIL import Image, ImageDraw, ImageFont

def create_wiring_diagram():
    W, H = 1600, 1220
    img = Image.new('RGBA', (W, H), (11, 13, 19, 255))
    draw = ImageDraw.Draw(img)

    # --- Fonts ---
    font_bahn_bold = 'C:/Windows/Fonts/bahnschrift.ttf'
    font_mono = 'C:/Windows/Fonts/consolab.ttf'
    font_segoe_bold = 'C:/Windows/Fonts/segoeuib.ttf'

    f_title = ImageFont.truetype(font_bahn_bold, 34)
    f_sub = ImageFont.truetype(font_mono, 13)
    f_header = ImageFont.truetype(font_segoe_bold, 15)
    f_label = ImageFont.truetype(font_segoe_bold, 13)
    f_small = ImageFont.truetype(font_mono, 12)
    f_tiny = ImageFont.truetype(font_mono, 11)
    f_pin = ImageFont.truetype(font_mono, 12)
    f_oled = ImageFont.truetype(font_bahn_bold, 28)

    # --- 1. Background Grid & Outer Border ---
    for x in range(30, W - 30, 25):
        for y in range(30, H - 30, 25):
            draw.rectangle([x, y, x + 1, y + 1], fill=(20, 24, 34, 255))

    # Outer Frame
    pad = 25
    draw.rectangle([pad, pad, W - pad, H - pad], outline=(36, 44, 60, 255), width=2)
    for cx, cy in [(pad, pad), (W - pad, pad), (pad, H - pad), (W - pad, H - pad)]:
        draw.line([(cx - 10, cy), (cx + 10, cy)], fill=(180, 195, 220, 255), width=3)
        draw.line([(cx, cy - 10), (cx, cy + 10)], fill=(180, 195, 220, 255), width=3)

    # --- 2. Title & Header ---
    draw.text((pad + 25, pad + 10), "SIXWATCS // ESP32-C3 HARDWARE WIRING SCHEMATIC", fill=(255, 255, 255, 255), font=f_title)
    draw.text((pad + 27, pad + 46), "SKEMA SAMBUNGAN: OLED SSD1306 + 3 TOMBOL ARCADE + CHARGER TP4056 + BATERAI 18650 + SAKLAR", fill=(130, 150, 180, 255), font=f_sub)
    draw.line([(pad + 20, pad + 66), (W - pad - 20, pad + 66)], fill=(38, 46, 64, 255), width=1)

    # Pin pad helper
    def draw_pad(px, py, name, pos="left", color=(240, 200, 80, 255)):
        draw.rectangle([px - 5, py - 5, px + 5, py + 5], fill=(180, 140, 40, 255), outline=color, width=1)
        draw.rectangle([px - 2, py - 2, px + 2, py + 2], fill=(20, 22, 28, 255))
        bbox = draw.textbbox((0, 0), name, font=f_pin)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        if pos == "left":
            draw.text((px + 10, py - th//2 - 2), name, fill=(230, 235, 245, 255), font=f_pin)
        elif pos == "right":
            draw.text((px - 10 - tw, py - th//2 - 2), name, fill=(230, 235, 245, 255), font=f_pin)
        elif pos == "top":
            draw.text((px - tw//2, py - th - 7), name, fill=(230, 235, 245, 255), font=f_pin)
        elif pos == "bottom":
            draw.text((px - tw//2, py + 7), name, fill=(230, 235, 245, 255), font=f_pin)

    # Base Y for main component row
    MOD_Y = 195

    # =========================================================================
    # COMPONENT 1: 18650 LI-ION BATTERY (Top-Left)
    # =========================================================================
    bat_x, bat_y = 65, MOD_Y
    bat_w, bat_h = 145, 280

    draw.text((bat_x, bat_y - 28), "[1] BATERAI 18650", fill=(100, 210, 255, 255), font=f_header)

    # Positive Top Cap
    draw.rounded_rectangle([bat_x + bat_w//2 - 18, bat_y - 10, bat_x + bat_w//2 + 18, bat_y], radius=3, fill=(210, 215, 225, 255), outline=(130, 140, 160, 255), width=2)
    # Battery Cylinder Body
    draw.rounded_rectangle([bat_x, bat_y, bat_x + bat_w, bat_y + bat_h], radius=16, fill=(18, 48, 72, 255), outline=(45, 120, 180, 255), width=2)

    # Battery Graphics & Text
    draw.text((bat_x + 24, bat_y + 35), "18650 Li-ion", fill=(255, 255, 255, 255), font=f_label)
    draw.text((bat_x + 30, bat_y + 58), "3.7V / 4.2V", fill=(180, 220, 255, 255), font=f_small)
    draw.text((bat_x + 22, bat_y + 80), "2600-3400mAh", fill=(130, 175, 215, 255), font=f_tiny)

    # (+)
    draw.ellipse([bat_x + bat_w//2 - 16, bat_y + 125, bat_x + bat_w//2 + 16, bat_y + 157], fill=(220, 45, 45, 255))
    draw.text((bat_x + bat_w//2 - 6, bat_y + 129), "+", fill=(255, 255, 255, 255), font=f_header)
    draw.text((bat_x + 32, bat_y + 165), "KUTUB POSITIF", fill=(255, 120, 120, 255), font=f_tiny)

    # (-)
    draw.ellipse([bat_x + bat_w//2 - 16, bat_y + 210, bat_x + bat_w//2 + 16, bat_y + 242], fill=(35, 42, 52, 255), outline=(140, 150, 170, 255), width=2)
    draw.text((bat_x + bat_w//2 - 5, bat_y + 212), "-", fill=(255, 255, 255, 255), font=f_header)
    draw.text((bat_x + 30, bat_y + 250), "KUTUB NEGATIF", fill=(180, 190, 210, 255), font=f_tiny)

    # Terminal Wire Nodes (Right edge)
    bat_pos_pt = (bat_x + bat_w, bat_y + 140)
    bat_neg_pt = (bat_x + bat_w, bat_y + 225)
    draw_pad(bat_pos_pt[0], bat_pos_pt[1], "(+) OUT", pos="right", color=(255, 70, 70, 255))
    draw_pad(bat_neg_pt[0], bat_neg_pt[1], "(-) OUT", pos="right", color=(160, 170, 190, 255))

    # =========================================================================
    # COMPONENT 2: TP4056 CHARGER MODULE + PROTECTION (Left-Center)
    # =========================================================================
    tp_x, tp_y = 300, MOD_Y
    tp_w, tp_h = 245, 280

    draw.text((tp_x, tp_y - 28), "[2] MODUL TP4056 + PROTEKSI", fill=(100, 210, 255, 255), font=f_header)
    draw.rounded_rectangle([tp_x, tp_y, tp_x + tp_w, tp_y + tp_h], radius=10, fill=(14, 35, 68, 255), outline=(42, 98, 170, 255), width=2)

    # USB-C Input Port
    draw.rounded_rectangle([tp_x + tp_w//2 - 38, tp_y - 12, tp_x + tp_w//2 + 38, tp_y + 16], radius=5, fill=(180, 185, 195, 255), outline=(100, 110, 130, 255), width=2)
    draw.rectangle([tp_x + tp_w//2 - 28, tp_y + 4, tp_x + tp_w//2 + 28, tp_y + 10], fill=(28, 30, 38, 255))
    draw.text((tp_x + tp_w//2 - 46, tp_y + 24), "USB-C CHARGER 5V", fill=(180, 210, 245, 255), font=f_tiny)

    # ICs
    draw.rectangle([tp_x + 65, tp_y + 65, tp_x + 135, tp_y + 112], fill=(20, 24, 30, 255), outline=(65, 78, 98, 255), width=1)
    draw.text((tp_x + 73, tp_y + 82), "TP4056", fill=(210, 215, 230, 255), font=f_small)
    draw.rectangle([tp_x + 155, tp_y + 70, tp_x + 205, tp_y + 110], fill=(20, 24, 30, 255), outline=(65, 78, 98, 255), width=1)
    draw.text((tp_x + 160, tp_y + 83), "DW01A", fill=(175, 185, 205, 255), font=f_tiny)

    # Status LEDs
    draw.rectangle([tp_x + 28, tp_y + 55, tp_x + 42, tp_y + 69], fill=(220, 40, 40, 255))
    draw.text((tp_x + 20, tp_y + 73), "CHRG", fill=(255, 130, 130, 255), font=f_tiny)
    draw.rectangle([tp_x + 28, tp_y + 95, tp_x + 42, tp_y + 109], fill=(40, 140, 255, 255))
    draw.text((tp_x + 20, tp_y + 113), "FULL", fill=(130, 190, 255, 255), font=f_tiny)

    # Solder Pads on TP4056
    pad_tp_bp = (tp_x + 22, tp_y + 140)
    pad_tp_bm = (tp_x + 22, tp_y + 225)
    draw_pad(pad_tp_bp[0], pad_tp_bp[1], "B+", pos="left", color=(255, 80, 80, 255))
    draw_pad(pad_tp_bm[0], pad_tp_bm[1], "B-", pos="left", color=(160, 170, 190, 255))

    pad_tp_outp = (tp_x + tp_w - 22, tp_y + 140)
    pad_tp_outm = (tp_x + tp_w - 22, tp_y + 225)
    draw_pad(pad_tp_outp[0], pad_tp_outp[1], "OUT+", pos="right", color=(255, 80, 80, 255))
    draw_pad(pad_tp_outm[0], pad_tp_outm[1], "OUT-", pos="right", color=(160, 170, 190, 255))

    # =========================================================================
    # COMPONENT 3: SPST SLIDE SWITCH (Below TP4056)
    # =========================================================================
    sw_x, sw_y = 300, 520
    sw_w, sw_h = 245, 85

    draw.text((sw_x, sw_y - 24), "[3] SAKLAR POWER (ON / OFF)", fill=(100, 210, 255, 255), font=f_header)
    draw.rounded_rectangle([sw_x, sw_y, sw_x + sw_w, sw_y + sw_h], radius=8, fill=(28, 34, 46, 255), outline=(65, 80, 105, 255), width=2)
    # Slider Knob
    draw.rounded_rectangle([sw_x + 85, sw_y + 14, sw_x + 135, sw_y + 42], radius=4, fill=(200, 210, 225, 255), outline=(120, 130, 150, 255), width=2)
    draw.text((sw_x + 145, sw_y + 18), "SPST SWITCH", fill=(255, 255, 255, 255), font=f_label)

    sw_pin1 = (sw_x + 55, sw_y + sw_h - 14)
    sw_pin2 = (sw_x + sw_w - 55, sw_y + sw_h - 14)
    draw_pad(sw_pin1[0], sw_pin1[1], "IN", pos="left", color=(255, 110, 110, 255))
    draw_pad(sw_pin2[0], sw_pin2[1], "OUT", pos="right", color=(255, 110, 110, 255))

    # =========================================================================
    # COMPONENT 4: ESP32-C3 SUPERMINI (Center)
    # =========================================================================
    esp_x, esp_y = 650, MOD_Y
    esp_w, esp_h = 320, 480

    draw.text((esp_x, esp_y - 28), "[4] ESP32-C3 SUPERMINI", fill=(100, 210, 255, 255), font=f_header)
    draw.rounded_rectangle([esp_x, esp_y, esp_x + esp_w, esp_y + esp_h], radius=14, fill=(17, 19, 26, 255), outline=(60, 70, 95, 255), width=2)

    # USB-C Port
    draw.rounded_rectangle([esp_x + esp_w//2 - 38, esp_y - 12, esp_x + esp_w//2 + 38, esp_y + 24], radius=6, fill=(175, 180, 195, 255), outline=(90, 100, 120, 255), width=2)
    draw.rectangle([esp_x + esp_w//2 - 28, esp_y + 6, esp_x + esp_w//2 + 28, esp_y + 14], fill=(28, 30, 38, 255))
    draw.text((esp_x + esp_w//2 - 38, esp_y + 32), "USB-C DATA/PC", fill=(160, 175, 200, 255), font=f_tiny)

    # SoC Chip
    draw.rounded_rectangle([esp_x + 85, esp_y + 150, esp_x + esp_w - 85, esp_y + 255], radius=8, fill=(32, 37, 46, 255), outline=(90, 105, 130, 255), width=2)
    draw.text((esp_x + 105, esp_y + 175), "ESP32-C3", fill=(255, 255, 255, 255), font=f_header)
    draw.text((esp_x + 98, esp_y + 202), "RISC-V 160MHz", fill=(180, 195, 220, 255), font=f_small)
    draw.text((esp_x + 115, esp_y + 225), "BLE 5.0", fill=(130, 150, 180, 255), font=f_tiny)

    # Antenna
    draw.line([(esp_x + 110, esp_y + 310), (esp_x + esp_w - 110, esp_y + 310)], fill=(160, 140, 70, 255), width=3)
    draw.line([(esp_x + 110, esp_y + 320), (esp_x + esp_w - 110, esp_y + 320)], fill=(160, 140, 70, 255), width=3)
    draw.text((esp_x + 115, esp_y + 335), "PCB ANTENNA", fill=(130, 120, 80, 255), font=f_tiny)

    # Left Header Pins
    left_pin_x = esp_x + 20
    pin_y_start = esp_y + 65
    pin_step = 44

    left_pins = [
        ("5V",  (255, 80, 80, 255)),     # 0: 5V Power Input
        ("GND", (160, 170, 190, 255)),   # 1: Ground
        ("3V3", (255, 160, 40, 255)),    # 2: 3.3V Output
        ("IO0", (60, 220, 120, 255)),    # 3: Tombol Kiri
        ("IO1", (80, 170, 255, 255)),    # 4: Tombol Kanan
        ("IO2", (80, 90, 110, 255)),     # 5: NC
        ("IO3", (230, 80, 230, 255)),    # 6: Tombol OK
        ("IO4", (255, 225, 60, 255)),    # 7: OLED SDA
        ("IO5", (60, 225, 240, 255)),    # 8: OLED SCL
    ]

    esp_pin_pts = {}
    for idx, (pname, pcolor) in enumerate(left_pins):
        py = pin_y_start + idx * pin_step
        esp_pin_pts[pname] = (left_pin_x, py)
        draw_pad(left_pin_x, py, pname, pos="left", color=pcolor)

    # Right Header Pins (Unused references)
    right_pin_x = esp_x + esp_w - 20
    right_pins = ["IO6", "IO7", "IO8", "IO9", "IO10", "IO20", "IO21", "GND", "3V3"]
    for idx, rname in enumerate(right_pins):
        py = pin_y_start + idx * pin_step
        draw_pad(right_pin_x, py, rname, pos="right", color=(80, 90, 110, 255))

    # =========================================================================
    # COMPONENT 5: SSD1306 0.96" OLED DISPLAY (Top-Right)
    # =========================================================================
    oled_x, oled_y = 1080, MOD_Y
    oled_w, oled_h = 445, 260

    draw.text((oled_x, oled_y - 28), "[5] 0.96\" I2C OLED SSD1306 (128x64)", fill=(100, 210, 255, 255), font=f_header)
    draw.rounded_rectangle([oled_x, oled_y, oled_x + oled_w, oled_y + oled_h], radius=12, fill=(15, 25, 42, 255), outline=(48, 80, 138, 255), width=2)

    # Mounting Holes
    for hx, hy in [(oled_x + 14, oled_y + 14), (oled_x + oled_w - 14, oled_y + 14), (oled_x + 14, oled_y + oled_h - 14), (oled_x + oled_w - 14, oled_y + oled_h - 14)]:
        draw.ellipse([hx - 7, hy - 7, hx + 7, hy + 7], outline=(150, 130, 80, 255), width=2)
        draw.ellipse([hx - 4, hy - 4, hx + 4, hy + 4], fill=(10, 14, 20, 255))

    # 4 OLED Header Pins at TOP of OLED PCB
    oled_pin_y = oled_y + 24
    oled_pins_def = [
        ("GND", oled_x + 90,  (160, 170, 190, 255)),
        ("VCC", oled_x + 180, (255, 160, 40, 255)),
        ("SCL", oled_x + 270, (60, 225, 240, 255)),
        ("SDA", oled_x + 360, (255, 225, 60, 255)),
    ]

    oled_pin_pts = {}
    for pname, px, pcol in oled_pins_def:
        draw_pad(px, oled_pin_y, pname, pos="bottom", color=pcol)
        oled_pin_pts[pname] = (px, oled_pin_y)

    # Screen Glass
    scr_x, scr_y = oled_x + 40, oled_y + 55
    scr_w, scr_h = oled_w - 80, oled_h - 75
    draw.rounded_rectangle([scr_x, scr_y, scr_x + scr_w, scr_y + scr_h], radius=5, fill=(4, 5, 8, 255), outline=(70, 85, 115, 255), width=1)

    # Screen Graphics
    draw.text((scr_x + 14, scr_y + 10), "HUD v2.4", fill=(140, 150, 175, 255), font=f_tiny)
    draw.text((scr_x + scr_w - 80, scr_y + 10), "BLE  94%", fill=(140, 150, 175, 255), font=f_tiny)
    draw.line([(scr_x + 10, scr_y + 26), (scr_x + scr_w - 10, scr_y + 26)], fill=(35, 45, 60, 255), width=1)
    draw.text((scr_x + 16, scr_y + 35), "14:38:26", fill=(255, 255, 255, 255), font=f_oled)
    draw.text((scr_x + 175, scr_y + 40), "MON 09 NOV", fill=(180, 195, 220, 255), font=f_tiny)
    draw.text((scr_x + 175, scr_y + 56), "28 C  82% RH", fill=(130, 145, 170, 255), font=f_tiny)
    draw.line([(scr_x + 10, scr_y + 78), (scr_x + scr_w - 10, scr_y + 78)], fill=(35, 45, 60, 255), width=1)
    draw.text((scr_x + 14, scr_y + 90), ">> SPOTIFY: wave to earth [Hangul OK]", fill=(200, 215, 240, 255), font=f_tiny)

    # =========================================================================
    # COMPONENT 6: 3 ARCADE PUSH BUTTONS (Middle-Right)
    # =========================================================================
    btn_box_x = 1080
    btn_box_y = 480
    btn_box_w = 445
    btn_box_h = 200

    draw.text((btn_box_x, btn_box_y - 24), "[6] 3x TOMBOL ARCADE (INPUT_PULLUP)", fill=(100, 210, 255, 255), font=f_header)
    draw.rounded_rectangle([btn_box_x, btn_box_y, btn_box_x + btn_box_w, btn_box_y + btn_box_h], radius=12, fill=(19, 23, 33, 255), outline=(48, 62, 85, 255), width=2)

    buttons_spec = [
        ("TOMBOL KIRI",  btn_box_x + 75,  "IO0", (60, 220, 120, 255),  "PREV / LEFT"),
        ("TOMBOL OK",    btn_box_x + 222, "IO3", (230, 80, 230, 255), "ACTION / PLAY"),
        ("TOMBOL KANAN", btn_box_x + 370, "IO1", (80, 170, 255, 255), "NEXT / RIGHT"),
    ]

    btn_pin_pts = {}
    for btitle, bx, bgpio, bcolor, bdesc in buttons_spec:
        by = btn_box_y + 75
        # Button Housing
        draw.rounded_rectangle([bx - 48, by - 44, bx + 48, by + 44], radius=8, fill=(28, 34, 46, 255), outline=(62, 76, 102, 255), width=1)
        # Cap
        draw.ellipse([bx - 30, by - 30, bx + 30, by + 30], fill=bcolor, outline=(255, 255, 255, 255), width=2)
        draw.ellipse([bx - 22, by - 22, bx + 22, by + 22], fill=(20, 24, 32, 255))
        draw.text((bx - 12, by - 7), bgpio, fill=bcolor, font=f_label)

        # Labels
        draw.text((bx - 38, by + 52), btitle, fill=(230, 235, 245, 255), font=f_label)
        draw.text((bx - 36, by + 70), bdesc, fill=(140, 155, 180, 255), font=f_tiny)

        # 2 Solder Lugs at BOTTOM (wires enter cleanly from bottom without crossing text!)
        lug1 = (bx - 18, by + 95)
        lug2 = (bx + 18, by + 95)
        draw_pad(lug1[0], lug1[1], "IO", pos="bottom", color=bcolor)
        draw_pad(lug2[0], lug2[1], "GND", pos="bottom", color=(160, 170, 190, 255))
        btn_pin_pts[bgpio] = lug1
        btn_pin_pts[bgpio + "_GND"] = lug2

    # =========================================================================
    # COMPONENT 7: REFERENCE TABLE & ASSEMBLY TIPS (Bottom)
    # =========================================================================
    tbl_x, tbl_y = 65, 750
    tbl_w, tbl_h = 900, 420

    draw.text((tbl_x, tbl_y - 24), "TABEL LENGKAP KONEKSI KABEL & FUNGSI PIN", fill=(100, 210, 255, 255), font=f_header)
    draw.rounded_rectangle([tbl_x, tbl_y, tbl_x + tbl_w, tbl_y + tbl_h], radius=10, fill=(16, 20, 28, 255), outline=(45, 56, 78, 255), width=2)

    # Table Header
    draw.rectangle([tbl_x, tbl_y, tbl_x + tbl_w, tbl_y + 38], fill=(26, 33, 48, 255))
    draw.text((tbl_x + 18, tbl_y + 11), "WARNA KABEL", fill=(210, 225, 250, 255), font=f_label)
    draw.text((tbl_x + 160, tbl_y + 11), "DARI (SUMBER)", fill=(210, 225, 250, 255), font=f_label)
    draw.text((tbl_x + 360, tbl_y + 11), "MENUJU (TUJUAN)", fill=(210, 225, 250, 255), font=f_label)
    draw.text((tbl_x + 590, tbl_y + 11), "FUNGSI & PENJELASAN", fill=(210, 225, 250, 255), font=f_label)

    rows = [
        ("BAT+ (Merah)",    "Kutub (+) Baterai 18650", "Pin B+ TP4056",          "Jalur pengisian Li-ion 3.7V - 4.2V"),
        ("BAT- (Hitam)",    "Kutub (-) Baterai 18650", "Pin B- TP4056",          "Ground Baterai terproteksi DW01A"),
        ("PWR IN (Merah)",  "Pin OUT+ TP4056",         "Kaki IN Saklar ON/OFF",  "Input tegangan baterai ke saklar"),
        ("PWR OUT (Merah)", "Kaki OUT Saklar ON/OFF",  "Pin 5V (VIN) ESP32-C3",  "Power board (masuk regulator LDO bawaan)"),
        ("COMMON GND",      "Pin OUT- TP4056",         "Pin GND ESP32-C3",       "Ground utama seluruh sistem"),
        ("OLED VCC (Oren)", "Pin 3V3 ESP32-C3",        "Pin VCC Layar OLED",     "Suplai 3.3V teregulasi stabil untuk display"),
        ("OLED GND (Abu)",  "Pin GND ESP32-C3",        "Pin GND Layar OLED",     "Ground return untuk layar OLED"),
        ("I2C SCL (Cyan)",  "Pin IO5 ESP32-C3",        "Pin SCL Layar OLED",     "I2C Clock line dedicated"),
        ("I2C SDA (Kuning)","Pin IO4 ESP32-C3",        "Pin SDA Layar OLED",     "I2C Data line dedicated"),
        ("BTN KIRI (Hijau)","Pin IO0 ESP32-C3",        "Kaki Tombol Kiri -> GND","Internal Pull-up (Active LOW saat ditekan)"),
        ("BTN OK (Ungu)",   "Pin IO3 ESP32-C3",        "Kaki Tombol OK -> GND",  "Internal Pull-up (Active LOW saat ditekan)"),
        ("BTN KANAN (Biru)","Pin IO1 ESP32-C3",        "Kaki Tombol Kanan -> GND","Internal Pull-up (Active LOW saat ditekan)")
    ]

    for ridx, (r_col, r_src, r_dst, r_desc) in enumerate(rows):
        ry = tbl_y + 44 + ridx * 30
        if ridx % 2 == 1:
            draw.rectangle([tbl_x + 2, ry - 3, tbl_x + tbl_w - 2, ry + 26], fill=(22, 27, 38, 255))
        draw.text((tbl_x + 18, ry + 3), r_col, fill=(240, 245, 255, 255), font=f_small)
        draw.text((tbl_x + 160, ry + 3), r_src, fill=(180, 205, 240, 255), font=f_small)
        draw.text((tbl_x + 360, ry + 3), r_dst, fill=(140, 225, 180, 255), font=f_small)
        draw.text((tbl_x + 590, ry + 3), r_desc, fill=(160, 175, 200, 255), font=f_tiny)

    # Right Box: Tips & Safety (Bottom-Right)
    note_x, note_y = 1000, 750
    note_w, note_h = 525, 420
    draw.rounded_rectangle([note_x, note_y, note_x + note_w, note_y + note_h], radius=10, fill=(18, 22, 32, 255), outline=(55, 70, 95, 255), width=2)
    draw.text((note_x + 20, note_y + 16), "PANDUAN PRAKTIS & KESELAMATAN BATERAI", fill=(255, 215, 80, 255), font=f_header)

    tips = [
        "1. PILIH MODUL TP4056 DENGAN DUAL IC (DENGAN PROTEKSI):",
        "   - Pastikan modul TP4056 memiliki 4 terminal output:",
        "     B+, B-, OUT+, OUT- (memiliki chip DW01A + FS8205A).",
        "   - Jangan gunakan versi 2 terminal (hanya B+ / B-) karena",
        "     tidak memiliki pemutus saat baterai drop di bawah 2.5V.",
        "",
        "2. REKOMENDASI SAKLAR (SLIDE SWITCH):",
        "   - Pasang saklar SPST antara OUT+ modul TP4056 dan pin 5V",
        "     board ESP32-C3 SuperMini.",
        "   - Dengan saklar ini, alat bisa dimatikan total tanpa",
        "     mengganggu proses charging baterai dari port USB-C TP4056.",
        "",
        "3. KENAPA MASUK KE PIN 5V (BUKAN 3V3)?",
        "   - Baterai 18650 menghasilkan 3.7V - 4.2V saat penuh.",
        "   - Pin 5V pada SuperMini terhubung ke onboard LDO regulator",
        "     (ME6211) yang menurunkan tegangan ke 3.3V secara bersih",
        "     dan aman untuk chip ESP32-C3 dan OLED.",
        "",
        "4. KONEKSI GROUND BERSAMA (COMMON GND):",
        "   - Hubungkan OUT- modul charger ke pin GND ESP32-C3.",
        "   - GND Layar OLED dan salah satu kaki ketiga tombol",
        "     disambungkan ke jalur GND ini.",
        "",
        "5. SAAT FLASHING FIRMWARE DARI PC:",
        "   - Kamu bisa matikan saklar baterai saat colok USB SuperMini",
        "     ke laptop/PC untuk upload code atau Spotify companion."
    ]
    for tidx, tline in enumerate(tips):
        draw.text((note_x + 20, note_y + 44 + tidx * 16), tline, fill=(210, 220, 240, 255), font=f_tiny)

    # =========================================================================
    # WIRING CONNECTIONS (Clean Orthogonal Wires with ZERO Overlap!)
    # =========================================================================

    def draw_wire(points, color, width=3):
        shadow_pts = [(p[0], p[1] + 1) for p in points]
        draw.line(shadow_pts, fill=(0, 0, 0, 160), width=width + 2)
        draw.line(points, fill=color, width=width)
        draw.ellipse([points[0][0] - 4, points[0][1] - 4, points[0][0] + 4, points[0][1] + 4], fill=color)
        draw.ellipse([points[-1][0] - 4, points[-1][1] - 4, points[-1][0] + 4, points[-1][1] + 4], fill=color)

    # 1. Battery (+) -> TP4056 B+ (Straight horizontal!)
    wire_bat_pos = [
        bat_pos_pt,
        pad_tp_bp
    ]
    draw_wire(wire_bat_pos, (240, 50, 50, 255), width=4)

    # 2. Battery (-) -> TP4056 B- (Straight horizontal!)
    wire_bat_neg = [
        bat_neg_pt,
        pad_tp_bm
    ]
    draw_wire(wire_bat_neg, (70, 75, 88, 255), width=4)

    # 3. TP4056 OUT+ -> Switch IN
    wire_tp_sw = [
        pad_tp_outp,
        (pad_tp_outp[0] + 25, pad_tp_outp[1]),
        (pad_tp_outp[0] + 25, sw_pin1[1] - 25),
        (sw_pin1[0], sw_pin1[1] - 25),
        sw_pin1
    ]
    draw_wire(wire_tp_sw, (240, 50, 50, 255), width=3)

    # 4. Switch OUT -> ESP32 5V (VIN)
    esp_5v_pt = esp_pin_pts["5V"]
    wire_sw_esp = [
        sw_pin2,
        (sw_pin2[0], sw_pin2[1] + 25),
        (580, sw_pin2[1] + 25),
        (580, esp_5v_pt[1]),
        esp_5v_pt
    ]
    draw_wire(wire_sw_esp, (255, 60, 60, 255), width=4)

    # 5. TP4056 OUT- -> ESP32 GND
    esp_gnd_pt = esp_pin_pts["GND"]
    wire_tp_gnd = [
        pad_tp_outm,
        (pad_tp_outm[0] + 35, pad_tp_outm[1]),
        (pad_tp_outm[0] + 35, 640),
        (600, 640),
        (600, esp_gnd_pt[1]),
        esp_gnd_pt
    ]
    draw_wire(wire_tp_gnd, (80, 85, 100, 255), width=4)

    # 6-9. OLED Wires:
    # Top Highway Channel: y = 100 to y = 150 (Safely above all module headers and titles!)
    esp_3v3_pt = esp_pin_pts["3V3"]
    oled_vcc_pt = oled_pin_pts["VCC"]
    wire_3v3_oled = [
        esp_3v3_pt,
        (esp_3v3_pt[0] - 25, esp_3v3_pt[1]),
        (esp_3v3_pt[0] - 25, 142),
        (oled_vcc_pt[0], 142),
        oled_vcc_pt
    ]
    draw_wire(wire_3v3_oled, (255, 160, 40, 255), width=3)

    oled_gnd_pt = oled_pin_pts["GND"]
    wire_gnd_oled = [
        (600, esp_gnd_pt[1]),
        (600, 128),
        (oled_gnd_pt[0], 128),
        oled_gnd_pt
    ]
    draw_wire(wire_gnd_oled, (80, 85, 100, 255), width=3)

    esp_sda_pt = esp_pin_pts["IO4"]
    oled_sda_pt = oled_pin_pts["SDA"]
    wire_sda = [
        esp_sda_pt,
        (esp_sda_pt[0] - 38, esp_sda_pt[1]),
        (esp_sda_pt[0] - 38, 114),
        (oled_sda_pt[0], 114),
        oled_sda_pt
    ]
    draw_wire(wire_sda, (255, 225, 60, 255), width=3)

    esp_scl_pt = esp_pin_pts["IO5"]
    oled_scl_pt = oled_pin_pts["SCL"]
    wire_scl = [
        esp_scl_pt,
        (esp_scl_pt[0] - 50, esp_scl_pt[1]),
        (esp_scl_pt[0] - 50, 100),
        (oled_scl_pt[0], 100),
        oled_scl_pt
    ]
    draw_wire(wire_scl, (60, 225, 240, 255), width=3)

    # 10. Buttons to ESP32 GPIOs:
    # Under Highway Channel: y = 690 to y = 730 (Safely UNDER ESP32, zero chip or title crossing!)
    esp_io0_pt = esp_pin_pts["IO0"]
    btn_io0_pt = btn_pin_pts["IO0"]
    wire_btn_io0 = [
        esp_io0_pt,
        (esp_io0_pt[0] - 30, esp_io0_pt[1]),
        (esp_io0_pt[0] - 30, 680),
        (btn_io0_pt[0], 680),
        btn_io0_pt
    ]
    draw_wire(wire_btn_io0, (60, 220, 120, 255), width=3)

    esp_io3_pt = esp_pin_pts["IO3"]
    btn_io3_pt = btn_pin_pts["IO3"]
    wire_btn_io3 = [
        esp_io3_pt,
        (esp_io3_pt[0] - 20, esp_io3_pt[1]),
        (esp_io3_pt[0] - 20, 694),
        (btn_io3_pt[0], 694),
        btn_io3_pt
    ]
    draw_wire(wire_btn_io3, (230, 80, 230, 255), width=3)

    esp_io1_pt = esp_pin_pts["IO1"]
    btn_io1_pt = btn_pin_pts["IO1"]
    wire_btn_io1 = [
        esp_io1_pt,
        (esp_io1_pt[0] - 10, esp_io1_pt[1]),
        (esp_io1_pt[0] - 10, 708),
        (btn_io1_pt[0], 708),
        btn_io1_pt
    ]
    draw_wire(wire_btn_io1, (80, 170, 255, 255), width=3)

    # 11. Buttons Common GND Bus (Under Highway Channel y = 722)
    btn_gnd0 = btn_pin_pts["IO0_GND"]
    btn_gnd3 = btn_pin_pts["IO3_GND"]
    btn_gnd1 = btn_pin_pts["IO1_GND"]
    wire_btn_gnd = [
        btn_gnd1,
        (btn_gnd1[0], 722),
        (600, 722),
        (600, esp_gnd_pt[1])
    ]
    draw_wire(wire_btn_gnd, (80, 85, 100, 255), width=3)
    draw.line([btn_gnd0, (btn_gnd0[0], 722)], fill=(80, 85, 100, 255), width=3)
    draw.ellipse([btn_gnd0[0] - 4, 722 - 4, btn_gnd0[0] + 4, 722 + 4], fill=(80, 85, 100, 255))
    draw.line([btn_gnd3, (btn_gnd3[0], 722)], fill=(80, 85, 100, 255), width=3)
    draw.ellipse([btn_gnd3[0] - 4, 722 - 4, btn_gnd3[0] + 4, 722 + 4], fill=(80, 85, 100, 255))

    os.makedirs('assets', exist_ok=True)
    out_file = 'assets/wiring_diagram.png'
    img.save(out_file, 'PNG')
    print(f"Wiring diagram successfully generated: {out_file}")

if __name__ == '__main__':
    create_wiring_diagram()
