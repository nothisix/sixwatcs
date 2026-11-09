import os
from PIL import Image, ImageDraw, ImageFont, ImageFilter

def create_pcb_blueprint():
    # High-Res Canvas: 1600 x 1100
    W, H = 1600, 1100
    img = Image.new('RGBA', (W, H), (10, 12, 17, 255))
    draw = ImageDraw.Draw(img)

    # Fonts
    font_bahn_bold = 'C:/Windows/Fonts/bahnschrift.ttf'
    font_mono = 'C:/Windows/Fonts/consolab.ttf'
    font_segoe_bold = 'C:/Windows/Fonts/segoeuib.ttf'

    f_title = ImageFont.truetype(font_bahn_bold, 32)
    f_sub = ImageFont.truetype(font_mono, 13)
    f_h2 = ImageFont.truetype(font_segoe_bold, 15)
    f_label = ImageFont.truetype(font_segoe_bold, 12)
    f_pin = ImageFont.truetype(font_mono, 11)
    f_tiny = ImageFont.truetype(font_mono, 10)

    # Background grid
    for x in range(25, W - 25, 25):
        for y in range(25, H - 25, 25):
            draw.rectangle([x, y, x + 1, y + 1], fill=(18, 22, 32, 255))

    pad = 25
    draw.rectangle([pad, pad, W - pad, H - pad], outline=(36, 44, 60, 255), width=2)
    for cx, cy in [(pad, pad), (W - pad, pad), (pad, H - pad), (W - pad, H - pad)]:
        draw.line([(cx - 10, cy), (cx + 10, cy)], fill=(180, 195, 220, 255), width=3)
        draw.line([(cx, cy - 10), (cx, cy + 10)], fill=(180, 195, 220, 255), width=3)

    # Title
    draw.text((pad + 25, pad + 12), "SIXWATCS // HARDWARE PCB DESIGN & PINOUT BLUEPRINT", fill=(255, 255, 255, 255), font=f_title)
    draw.text((pad + 27, pad + 48), "PANDUAN FABRIKASI PCB (EASYEDA / KICAD / JLCPCB) • DIMENSI 1:1 DENGAN JALUR COPPER & SILKSCREEN", fill=(130, 150, 180, 255), font=f_sub)
    draw.line([(pad + 20, pad + 68), (W - pad - 20, pad + 68)], fill=(38, 46, 64, 255), width=1)

    # Helper pad drawer
    def draw_trough_hole(px, py, drill_r=4, copper_r=7, fill_copper=(212, 175, 55, 255)):
        draw.ellipse([px - copper_r, py - copper_r, px + copper_r, py + copper_r], fill=fill_copper, outline=(255, 220, 90, 255), width=1)
        draw.ellipse([px - drill_r, py - drill_r, px + drill_r, py + drill_r], fill=(10, 12, 17, 255))

    # =========================================================================
    # LEFT SECTION: THE ACTUAL PCB BOARD PREVIEW (Matte Black Solder Mask)
    # =========================================================================
    pcb_x, pcb_y = 65, 120
    pcb_w, pcb_h = 760, 560

    draw.text((pcb_x, pcb_y - 26), "TOP COPPER & SILKSCREEN LAYOUT (85mm x 65mm CARRIER BOARD)", fill=(100, 210, 255, 255), font=f_h2)

    # FR4 PCB Substrate (Deep Matte Black Solder Mask)
    draw.rounded_rectangle([pcb_x, pcb_y, pcb_x + pcb_w, pcb_y + pcb_h], radius=16, fill=(16, 18, 22, 255), outline=(50, 58, 75, 255), width=2)

    # 4 Corner Mounting Holes (M3 / 3.2mm drill)
    for hx, hy in [(pcb_x + 20, pcb_y + 20), (pcb_x + pcb_w - 20, pcb_y + 20), (pcb_x + 20, pcb_y + pcb_h - 20), (pcb_x + pcb_w - 20, pcb_y + pcb_h - 20)]:
        draw.ellipse([hx - 14, hy - 14, hx + 14, hy + 14], outline=(180, 150, 70, 255), width=2)
        draw.ellipse([hx - 8, hy - 8, hx + 8, hy + 8], fill=(10, 12, 17, 255))
        draw.text((hx - 6, hy + 16), "M3", fill=(120, 130, 150, 255), font=f_tiny)

    # Silkscreen Border Frame on PCB
    draw.rounded_rectangle([pcb_x + 10, pcb_y + 10, pcb_x + pcb_w - 10, pcb_y + pcb_h - 10], radius=12, outline=(220, 225, 235, 180), width=1)
    draw.text((pcb_x + 25, pcb_y + 20), "SIXWATCS v2.4", fill=(255, 255, 255, 230), font=f_h2)
    draw.text((pcb_x + pcb_w - 180, pcb_y + 22), "DESIGNED BY @NOTHISIX", fill=(170, 180, 200, 200), font=f_tiny)

    # --- PCB COMPONENT 1: OLED 0.96" SSD1306 (Center-Top) ---
    oled_cx, oled_cy = pcb_x + pcb_w//2, pcb_y + 160
    oled_bw, oled_bh = 260, 200
    oled_rx = oled_cx - oled_bw//2
    oled_ry = oled_cy - oled_bh//2

    # OLED Silkscreen Courtyard Outline
    draw.rounded_rectangle([oled_rx, oled_ry, oled_rx + oled_bw, oled_ry + oled_bh], radius=8, outline=(220, 225, 240, 220), width=1)
    draw.rectangle([oled_rx + 25, oled_ry + 45, oled_rx + oled_bw - 25, oled_ry + oled_bh - 20], fill=(6, 8, 12, 255), outline=(70, 85, 115, 200), width=1)
    draw.text((oled_cx - 45, oled_cy), "0.96\" OLED GLASS", fill=(130, 145, 175, 200), font=f_label)
    draw.text((oled_cx - 48, oled_cy + 18), "128x64 SSD1306", fill=(100, 115, 140, 200), font=f_tiny)

    # OLED 4 Header Pins at Top (GND, VCC, SCL, SDA)
    oled_pad_y = oled_ry + 20
    oled_pins_x = [oled_cx - 45, oled_cx - 15, oled_cx + 15, oled_cx + 45]
    oled_pin_names = ["GND", "VCC", "SCL", "SDA"]
    for px, pname in zip(oled_pins_x, oled_pin_names):
        draw_trough_hole(px, oled_pad_y, drill_r=4, copper_r=7)
        draw.text((px - 9, oled_pad_y - 18), pname, fill=(240, 245, 255, 255), font=f_tiny)

    # --- PCB COMPONENT 2: ESP32-C3 SuperMini (Left-Middle) ---
    esp_x0, esp_y0 = pcb_x + 35, pcb_y + 90
    esp_bw, esp_bh = 175, 240

    # Courtyard
    draw.rounded_rectangle([esp_x0, esp_y0, esp_x0 + esp_bw, esp_y0 + esp_bh], radius=6, outline=(220, 225, 240, 220), width=1)
    # USB-C Port notch
    draw.rounded_rectangle([esp_x0 + esp_bw//2 - 28, esp_y0 - 8, esp_x0 + esp_bw//2 + 28, esp_y0 + 14], radius=3, fill=(180, 185, 195, 255), outline=(100, 110, 130, 255), width=1)
    draw.text((esp_x0 + esp_bw//2 - 26, esp_y0 + 3), "USB-C", fill=(30, 32, 40, 255), font=f_tiny)

    # SoC chip outline in center of module
    draw.rectangle([esp_x0 + 48, esp_y0 + 95, esp_x0 + esp_bw - 48, esp_y0 + 155], fill=(22, 25, 33, 255), outline=(75, 88, 110, 200), width=1)
    draw.text((esp_x0 + 55, esp_y0 + 112), "ESP32-C3", fill=(200, 215, 240, 255), font=f_tiny)
    draw.text((esp_x0 + 58, esp_y0 + 128), "160MHz", fill=(140, 155, 180, 255), font=f_tiny)

    # 2 Rows of 8 Header Sockets
    left_row_x = esp_x0 + 16
    right_row_x = esp_x0 + esp_bw - 16
    pin_y_start = esp_y0 + 35
    step_y = 24

    left_row_pins = ["5V", "GND", "IO0", "IO1", "IO2", "IO3", "IO4", "IO5"]
    right_row_pins = ["3V3", "GND", "10", "9", "8", "7", "6", "20"]

    for idx in range(8):
        py = pin_y_start + idx * step_y
        draw_trough_hole(left_row_x, py, drill_r=3, copper_r=6)
        draw.text((left_row_x + 9, py - 6), left_row_pins[idx], fill=(230, 240, 255, 255), font=f_tiny)

        draw_trough_hole(right_row_x, py, drill_r=3, copper_r=6)
        draw.text((right_row_x - 24, py - 6), right_row_pins[idx], fill=(230, 240, 255, 255), font=f_tiny)

    # --- PCB COMPONENT 3: TP4056 Module (Right-Top) ---
    tp_x0, tp_y0 = pcb_x + pcb_w - 200, pcb_y + 110
    tp_bw, tp_bh = 150, 180
    draw.rounded_rectangle([tp_x0, tp_y0, tp_x0 + tp_bw, tp_y0 + tp_bh], radius=6, outline=(220, 225, 240, 220), width=1)
    # USB-C Charge Port
    draw.rounded_rectangle([tp_x0 + tp_bw//2 - 25, tp_y0 - 8, tp_x0 + tp_bw//2 + 25, tp_y0 + 12], radius=3, fill=(180, 185, 195, 255), outline=(100, 110, 130, 255), width=1)
    draw.text((tp_x0 + 35, tp_y0 + 75), "TP4056 LIPO", fill=(255, 255, 255, 255), font=f_label)
    draw.text((tp_x0 + 40, tp_y0 + 94), "CHARGER", fill=(180, 195, 220, 255), font=f_tiny)

    # Solder Pads (B+, B-, OUT+, OUT-)
    tp_pad_bp = (tp_x0 + 20, tp_y0 + 140)
    tp_pad_bm = (tp_x0 + 20, tp_y0 + 165)
    tp_pad_outp = (tp_x0 + tp_bw - 20, tp_y0 + 140)
    tp_pad_outm = (tp_x0 + tp_bw - 20, tp_y0 + 165)

    draw_trough_hole(tp_pad_bp[0], tp_pad_bp[1], drill_r=3, copper_r=6)
    draw.text((tp_pad_bp[0] + 9, tp_pad_bp[1] - 6), "B+", fill=(255, 90, 90, 255), font=f_tiny)
    draw_trough_hole(tp_pad_bm[0], tp_pad_bm[1], drill_r=3, copper_r=6)
    draw.text((tp_pad_bm[0] + 9, tp_pad_bm[1] - 6), "B-", fill=(170, 185, 210, 255), font=f_tiny)

    draw_trough_hole(tp_pad_outp[0], tp_pad_outp[1], drill_r=3, copper_r=6)
    draw.text((tp_pad_outp[0] - 30, tp_pad_outp[1] - 6), "OUT+", fill=(255, 90, 90, 255), font=f_tiny)
    draw_trough_hole(tp_pad_outm[0], tp_pad_outm[1], drill_r=3, copper_r=6)
    draw.text((tp_pad_outm[0] - 30, tp_pad_outm[1] - 6), "OUT-", fill=(170, 185, 210, 255), font=f_tiny)

    # --- PCB COMPONENT 4: POWER SLIDE SWITCH (Right-Middle) ---
    sw_x0, sw_y0 = pcb_x + pcb_w - 180, pcb_y + 315
    draw.rounded_rectangle([sw_x0, sw_y0, sw_x0 + 110, sw_y0 + 55], radius=4, outline=(220, 225, 240, 220), width=1)
    draw.text((sw_x0 + 14, sw_y0 + 8), "PWR SWITCH", fill=(200, 215, 240, 255), font=f_tiny)
    # 3 Pins
    sw_p1 = (sw_x0 + 25, sw_y0 + 36)
    sw_p2 = (sw_x0 + 55, sw_y0 + 36)
    sw_p3 = (sw_x0 + 85, sw_y0 + 36)
    draw_trough_hole(sw_p1[0], sw_p1[1], drill_r=3, copper_r=6)
    draw_trough_hole(sw_p2[0], sw_p2[1], drill_r=3, copper_r=6)
    draw_trough_hole(sw_p3[0], sw_p3[1], drill_r=3, copper_r=6)
    draw.text((sw_x0 + 20, sw_y0 + 44), "IN", fill=(255, 120, 120, 255), font=f_tiny)
    draw.text((sw_x0 + 48, sw_y0 + 44), "OUT", fill=(255, 120, 120, 255), font=f_tiny)

    # --- PCB COMPONENT 5: 3 ARCADE PUSH BUTTONS (Bottom Row) ---
    btn_y = pcb_y + pcb_h - 100
    btn_spec = [
        (pcb_x + 160, "BTN 1 (PREV)", "GPIO 0", (60, 220, 120, 255)),
        (pcb_x + pcb_w//2, "BTN 2 (ACTION)", "GPIO 3", (230, 80, 230, 255)),
        (pcb_x + pcb_w - 160, "BTN 3 (NEXT)", "GPIO 1", (80, 170, 255, 255))
    ]

    for bx, bname, bgpio, bcolor in btn_spec:
        # 12x12mm or 6x6mm button footprint outline
        draw.rounded_rectangle([bx - 45, btn_y - 40, bx + 45, btn_y + 40], radius=6, outline=(220, 225, 240, 200), width=1)
        draw.ellipse([bx - 26, btn_y - 26, bx + 26, btn_y + 26], fill=(24, 28, 38, 255), outline=bcolor, width=2)
        draw.text((bx - 28, btn_y - 8), bgpio, fill=bcolor, font=f_label)

        draw.text((bx - 36, btn_y + 45), bname, fill=(210, 220, 240, 255), font=f_tiny)

        # 2 Pads for button switch
        p_sig = (bx - 20, btn_y + 24)
        p_gnd = (bx + 20, btn_y + 24)
        draw_trough_hole(p_sig[0], p_sig[1], drill_r=3, copper_r=5, fill_copper=bcolor)
        draw_trough_hole(p_gnd[0], p_gnd[1], drill_r=3, copper_r=5, fill_copper=(160, 170, 190, 255))

    # --- PCB COMPONENT 6: BATTERY JST CONNECTOR / TERMINAL BLOCK (Left-Lower) ---
    bat_conn_x, bat_conn_y = pcb_x + 40, pcb_y + 355
    draw.rounded_rectangle([bat_conn_x, bat_conn_y, bat_conn_x + 95, bat_conn_y + 50], radius=4, outline=(220, 225, 240, 220), width=1)
    draw.text((bat_conn_x + 12, bat_conn_y + 6), "18650 JST-PH", fill=(180, 200, 230, 255), font=f_tiny)
    draw_trough_hole(bat_conn_x + 30, bat_conn_y + 28, drill_r=3, copper_r=6)
    draw.text((bat_conn_x + 25, bat_conn_y + 35), "+", fill=(255, 90, 90, 255), font=f_label)
    draw_trough_hole(bat_conn_x + 65, bat_conn_y + 28, drill_r=3, copper_r=6)
    draw.text((bat_conn_x + 62, bat_conn_y + 35), "-", fill=(170, 185, 210, 255), font=f_label)

    # =========================================================================
    # RIGHT SECTION: PINOUT SPECIFICATIONS & NETLIST TABLE
    # =========================================================================
    sec_x = pcb_x + pcb_w + 35
    sec_y = 100
    sec_w = W - sec_x - pad - 20

    draw.text((sec_x, sec_y - 24), "ESP32-C3 SUPERMINI PINOUT VERIFIKASI (GOOGLE HARDWARE DATA)", fill=(255, 215, 80, 255), font=f_h2)

    # Pinout Table Box
    tbl_h = 440
    draw.rounded_rectangle([sec_x, sec_y, sec_x + sec_w, sec_y + tbl_h], radius=10, fill=(16, 20, 28, 255), outline=(48, 60, 82, 255), width=2)

    # Header
    draw.rectangle([sec_x, sec_y, sec_x + sec_w, sec_y + 34], fill=(25, 33, 48, 255))
    draw.text((sec_x + 14, sec_y + 9), "PIN #", fill=(200, 215, 245, 255), font=f_label)
    draw.text((sec_x + 65, sec_y + 9), "SIDE", fill=(200, 215, 245, 255), font=f_label)
    draw.text((sec_x + 125, sec_y + 9), "LABEL", fill=(200, 215, 245, 255), font=f_label)
    draw.text((sec_x + 215, sec_y + 9), "NET / SAMBUNGAN DI PCB", fill=(200, 215, 245, 255), font=f_label)
    draw.text((sec_x + 440, sec_y + 9), "KETERANGAN HW", fill=(200, 215, 245, 255), font=f_label)

    pin_table_data = [
        ("1",  "LEFT",  "5V",   "OUT Saklar Power (3.7V - 4.2V)", "VBUS / Input Daya LDO"),
        ("2",  "LEFT",  "GND",  "Common Ground PCB",              "Power Ground Rail"),
        ("3",  "LEFT",  "IO0",  "Tombol Kiri (Signal Pad)",       "ADC1_CH0 / Pullup Aktif"),
        ("4",  "LEFT",  "IO1",  "Tombol Kanan (Signal Pad)",      "ADC1_CH1 / Pullup Aktif"),
        ("5",  "LEFT",  "IO2",  "NC (Tidak disambung)",           "Strapping Pin (Biarkan Kosong)"),
        ("6",  "LEFT",  "IO3",  "Tombol OK (Signal Pad)",         "ADC1_CH3 / Pullup Aktif"),
        ("7",  "LEFT",  "IO4",  "OLED Pin SDA",                   "I2C Data Line (Dedicated)"),
        ("8",  "LEFT",  "IO5",  "OLED Pin SCL",                   "I2C Clock Line (Dedicated)"),
        ("9",  "RIGHT", "3V3",  "OLED Pin VCC (3.3V)",            "Output LDO Onboard 3.3V"),
        ("10", "RIGHT", "GND",  "OLED Pin GND / Battery (-)",     "Secondary Ground Rail"),
        ("11", "RIGHT", "IO10", "NC (Tidak disambung)",           "General GPIO"),
        ("12", "RIGHT", "IO9",  "NC (Terhubung ke tombol BOOT)",  "Strapping / Boot Button"),
        ("13", "RIGHT", "IO8",  "NC (Terhubung ke LED Biru)",     "Strapping / Blue LED (Active LOW)"),
        ("14", "RIGHT", "IO7",  "NC (Tidak disambung)",           "JTAG / SPI"),
        ("15", "RIGHT", "IO6",  "NC (Tidak disambung)",           "JTAG / SPI"),
        ("16", "RIGHT", "IO20", "NC (Tidak disambung)",           "UART RX")
    ]

    for ridx, (r_num, r_side, r_lbl, r_net, r_desc) in enumerate(pin_table_data):
        ry = sec_y + 40 + ridx * 24
        if ridx % 2 == 1:
            draw.rectangle([sec_x + 2, ry - 2, sec_x + sec_w - 2, ry + 21], fill=(20, 25, 36, 255))
        draw.text((sec_x + 18, ry + 2), r_num, fill=(240, 245, 255, 255), font=f_tiny)
        draw.text((sec_x + 65, ry + 2), r_side, fill=(160, 180, 210, 255), font=f_tiny)
        draw.text((sec_x + 125, ry + 2), r_lbl, fill=(255, 215, 80, 255), font=f_tiny)
        draw.text((sec_x + 215, ry + 2), r_net, fill=(140, 225, 180, 255), font=f_tiny)
        draw.text((sec_x + 440, ry + 2), r_desc, fill=(150, 165, 190, 255), font=f_tiny)

    # Footprint Specs Box (Below table)
    spec_box_y = sec_y + tbl_h + 18
    spec_box_h = 160
    draw.rounded_rectangle([sec_x, spec_box_y, sec_x + sec_w, spec_box_y + spec_box_h], radius=10, fill=(18, 22, 32, 255), outline=(48, 60, 82, 255), width=2)
    draw.text((sec_x + 18, spec_box_y + 14), "DIMENSI & PARAMETER FOOTPRINT KICAD / EASYEDA", fill=(100, 210, 255, 255), font=f_h2)

    specs_list = [
        ("Pin Pitch (Jarak antar pin 1 baris)", "2.54 mm (100 mil) - Standard Header DIP"),
        ("Row Spacing (Jarak baris kiri ke kanan)", "15.24 mm (600 mil) - Center-to-Center"),
        ("Dimensi Fisik Board ESP32-C3 SuperMini", "22.52 mm (Panjang) x 18.0 mm (Lebar)"),
        ("Diameter Lubang Through-Hole (Drill Size)", "0.9 mm s/d 1.0 mm (Pad Diameter: 1.6 mm)"),
        ("Clearance Port USB-C", "Beri lebihan 2.0 mm di tepi PCB untuk soket USB-C menonjol")
    ]
    for sidx, (sp_name, sp_val) in enumerate(specs_list):
        s_y = spec_box_y + 40 + sidx * 22
        draw.text((sec_x + 18, s_y), sp_name + ":", fill=(180, 195, 220, 255), font=f_tiny)
        draw.text((sec_x + 360, s_y), sp_val, fill=(255, 255, 255, 255), font=f_tiny)

    # =========================================================================
    # BOTTOM SECTION: ATURAN ROUTING & TIPS FABRIKASI JLCPCB
    # =========================================================================
    bot_y = pcb_y + pcb_h + 25
    bot_w = W - pad*2 - 40
    bot_h = H - bot_y - pad - 15

    draw.rounded_rectangle([pcb_x, bot_y, pcb_x + bot_w, bot_y + bot_h], radius=10, fill=(16, 20, 28, 255), outline=(48, 60, 82, 255), width=2)
    draw.text((pcb_x + 20, bot_y + 16), "PANDUAN ROUTING JALUR PCB & ATURAN DESAIN (DESIGN RULES)", fill=(255, 215, 80, 255), font=f_h2)

    rules = [
        "1. JALUR POWER & GROUND (VCC & GND):",
        "   - Lebar trace daya utama (B+, OUT+, 5V, 3V3) minimal 24 mil (0.6 mm) s/d 32 mil (0.8 mm) agar sanggup menahan lonjakan arus WiFi/BLE (hingga 300mA peak).",
        "   - Buat Copper Pour (GND Plane) pada Bottom Layer untuk meredam noise RF dan menjaga kestabilan I2C OLED.",
        "",
        "2. JALUR I2C OLED (GPIO 4 SDA & GPIO 5 SCL):",
        "   - Lebar trace signal I2C cukup 10 mil s/d 12 mil (0.25 - 0.3 mm).",
        "   - Pasang sejajar dengan jarak aman (clearance >= 8 mil) dari jalur switching daya.",
        "",
        "3. AREA ANTENA ESP32-C3:",
        "   - JANGAN taruh jalur tembaga (copper pour) atau komponen di bawah area antena PCB SuperMini (Keepout Area minimal 5mm) agar jangkauan sinyal Bluetooth LE optimal.",
        "",
        "4. KONEKTOR BATERAI 18650:",
        "   - Sediakan footprint 2-pin JST-PH (2.0mm pitch) atau terminal block 2.54mm di dekat modul TP4056 agar baterai mudah dilepas-pasang saat perakitan case."
    ]

    for ridx, rline in enumerate(rules):
        draw.text((pcb_x + 20, bot_y + 44 + ridx * 16), rline, fill=(210, 225, 245, 255), font=f_tiny)

    os.makedirs('assets', exist_ok=True)
    out_file = 'assets/pcb_layout_design.png'
    img.save(out_file, 'PNG')
    print(f"PCB blueprint successfully generated: {out_file}")

if __name__ == '__main__':
    create_pcb_blueprint()
