#!/usr/bin/env python3
"""Draw the README diagrams.

Writes the SVG sources next to this script and renders them to PNG in
docs/images/ with rsvg-convert (Debian/Ubuntu package librsvg2-bin):

    python3 docs/diagrams/make_diagrams.py
"""
import itertools
import re
import subprocess
from pathlib import Path

FONT = "Lato, 'DejaVu Sans', sans-serif"
INK = "#1f2937"
MUTED = "#6b7280"
CTRL = "#1d4ed8"
MEAS = "#c2410c"
RAIL = "#b91c1c"
WIRE = "#111827"

GROUPS = {
    "heat":   ("#fff4e5", "#e9a24f"),
    "grid":   ("#eaf6ec", "#5fae70"),
    "anode":  ("#e8f0fe", "#6b93db"),
    "screen": ("#f3ecfb", "#a27ad2"),
    "ui":     ("#f5f6f8", "#9ca3af"),
    "state":  ("#f5f6f8", "#9ca3af"),
    "main":   ("#e6f6f5", "#3a9d97"),
}


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def rich(s, size):
    """'U_{g1}' -> subscript tspans; '**x**' -> bold."""
    out = []
    sub = round(size * 0.76, 1)
    dy = round(size * 0.28, 1)
    low = False     # the baseline is lowered by a subscript; the next text raises it
    for tok in re.split(r"(_\{[^}]*\}|\*\*[^*]+\*\*)", s):
        if not tok:
            continue
        back = f' dy="-{dy}"' if low else ""
        if tok.startswith("_{"):
            out.append(f'<tspan dy="{dy}" font-size="{sub}">{esc(tok[2:-1])}</tspan>')
            low = True
            continue
        if tok.startswith("**"):
            out.append(f'<tspan{back} font-weight="700">{esc(tok[2:-2])}</tspan>')
        elif low:
            out.append(f'<tspan{back}>{esc(tok)}</tspan>')
        else:
            out.append(esc(tok))
        low = False
    return "".join(out)


class Svg:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.body = []
        self.markers = {}

    def marker(self, color):
        mid = "a" + color.lstrip("#")
        if mid not in self.markers:
            self.markers[mid] = (
                f'<marker id="{mid}" viewBox="0 0 10 10" refX="9" refY="5" '
                f'markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
                f'<path d="M0,0 L10,5 L0,10 z" fill="{color}"/></marker>')
        return mid

    def add(self, s):
        self.body.append(s)

    def text(self, x, y, s, size=14, weight=400, anchor="start", fill=INK,
             italic=False, extra=""):
        st = ' font-style="italic"' if italic else ""
        self.add(f'<text x="{x}" y="{y}" font-size="{size}" font-weight="{weight}" '
                 f'text-anchor="{anchor}" fill="{fill}"{st} {extra}>{rich(s, size)}</text>')

    def rect(self, x, y, w, h, fill, stroke, r=8, sw=1.5, extra=""):
        self.add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" '
                 f'fill="{fill}" stroke="{stroke}" stroke-width="{sw}" {extra}/>')

    def path(self, d, color, sw=1.8, dash=None, start=False, end=True, fill="none"):
        m = self.marker(color)
        a = ""
        if end:
            a += f' marker-end="url(#{m})"'
        if start:
            a += f' marker-start="url(#{m})"'
        ds = f' stroke-dasharray="{dash}"' if dash else ""
        self.add(f'<path d="{d}" fill="{fill}" stroke="{color}" stroke-width="{sw}" '
                 f'stroke-linejoin="round" stroke-linecap="round"{ds}{a}/>')

    def poly(self, pts, color, **kw):
        d = "M" + " L".join(f"{x},{y}" for x, y in pts)
        self.path(d, color, **kw)

    def block(self, x, y, w, h, group, lines, pad=12, title=15, size=13):
        fill, stroke = GROUPS[group]
        self.rect(x, y, w, h, fill, stroke)
        ty = y + 23
        for i, ln in enumerate(lines):
            if i == 0:
                self.text(x + pad, ty, ln, title, 700)
                ty += 21
            else:
                self.text(x + pad, ty, ln, size, 400, fill="#374151")
                ty += 18

    def pill(self, cx, cy, s, color=RAIL):
        w = 9 + 7.2 * len(s)
        self.rect(cx - w / 2, cy - 10, w, 20, "#ffffff", color, r=10, sw=1.4)
        self.text(cx, cy + 4.5, s, 12, 700, "middle", color)

    def save(self, path):
        head = (f'<svg xmlns="http://www.w3.org/2000/svg" xml:space="preserve" width="{self.w}" height="{self.h}" '
                f'viewBox="0 0 {self.w} {self.h}" font-family="{FONT}">\n'
                f'<defs>{"".join(self.markers.values())}</defs>\n'
                f'<rect width="100%" height="100%" fill="#ffffff"/>\n')
        Path(path).write_text(head + "\n".join(self.body) + "\n</svg>\n")


# ---------------------------------------------------------------- block diagram
def block_diagram(out):
    s = Svg(1250, 880)

    # Left column: power and user interface
    s.block(20, 30, 180, 96, "ui", ["Power supply", "+335 V anode rail", "+15 V rail",
                                    "LM317 → +5 V logic"])
    ui = [
        (165, ["LCD 4×20", "HD44780, 4-bit"], "PC2–PC7", "out"),
        (255, ["Rotary encoder", "with push button"], "PD2,3,6", "in"),
        (345, ["Buzzer"], "PD7", "out"),
        (415, ["RS-232", "MAX202 → DE-9"], "PD0,1", "both"),
        (505, ["LM35", "heatsink temp."], "ADC0", "meas"),
    ]
    MX, MW = 290, 140
    for y, lines, lab, kind in ui:
        h = 46 if len(lines) == 1 else 64
        s.block(20, y, 180, h, "ui", lines)
        cy = y + h / 2
        if kind == "meas":
            s.poly([(200, cy), (MX, cy)], MEAS, dash="6 4")
        elif kind == "in":
            s.poly([(200, cy), (MX, cy)], CTRL)
        elif kind == "out":
            s.poly([(MX, cy), (200, cy)], CTRL)
        else:
            s.poly([(200, cy), (MX, cy)], CTRL, start=True)
        s.text(245, cy - 7, lab, 12, 400, "middle", MUTED)

    # MCU
    s.rect(MX, 150, MW, 650, "#1f2937", "#111827", r=10)
    s.text(MX + MW / 2, 186, "ATmega32", 19, 700, "middle", "#ffffff")
    s.text(MX + MW / 2, 208, "16 MHz", 14, 400, "middle", "#cbd5e1")
    s.text(MX + MW / 2, 780, "ADC: port A", 12.5, 400, "middle", "#cbd5e1")

    R = MX + MW          # 430: MCU right edge
    B = 570              # blocks start
    BW = 180             # chain block width
    B2 = B + BW + 30     # second block in a chain
    B3 = B2 + BW + 30    # third block
    TX = 1150            # tube

    def ctrl(y, x2, label):
        s.poly([(R, y), (x2, y)], CTRL)
        s.text(R + 8, y - 6, label, 12, 400, fill=CTRL)

    def meas(pts, label, ly):
        s.poly(pts, MEAS, dash="6 4")
        s.text(R + 8, ly - 6, label, 12, 400, fill=MEAS)

    def to_tube(x1, y, label):
        s.poly([(x1, y), (TX, y)], WIRE, sw=2.4)
        s.add(f'<circle cx="{TX}" cy="{y}" r="4" fill="{WIRE}"/>')
        s.text(TX + 12, y + 5, label, 15, 700)

    # Tube under test
    s.rect(TX, 140, 80, 660, "#fffbeb", "#b45309", r=40, sw=2)
    s.add(f'<text transform="translate({TX + 58},470) rotate(-90)" font-size="15" '
          f'font-weight="700" text-anchor="middle" fill="#92400e" '
          f'letter-spacing="2">TUBE UNDER TEST</text>')

    full = B3 - 30 - B   # heater / grid block width, aligned with the chain
    # Heater
    s.block(B, 150, full, 82, "heat", ["Heater supply",
                                       "PWM step-down, voltage or current mode",
                                       "TC4426 · IRF9540N · 1 mH · 1N5822"])
    s.pill(B + full - 40, 150, "+15 V")
    ctrl(172, B, "PB3 · OC0 PWM")
    meas([(B, 214), (R, 214)], "ADC1 I_{h} · ADC2 U_{h}", 214)
    to_tube(B + full, 191, "H")

    # Control grid
    s.block(B, 270, full, 82, "grid", ["Grid bias −U_{g1}, 0 … −24 V",
                                       "Negative charge pump, regulated per sample",
                                       "TC4426"])
    s.pill(B + full - 40, 270, "+15 V")
    ctrl(292, B, "PB2 · pump clock")
    meas([(B, 334), (R, 334)], "ADC7 U_{g1}", 334)
    to_tube(B + full, 311, "G1")

    # Anode
    s.block(B, 400, BW, 82, "anode", ["Anode regulator", "linear, series pass",
                                      "LM358 · IRF740 · BD139"])
    s.pill(B + BW - 40, 400, "+335 V")
    s.block(B2, 400, BW, 82, "anode", ["I_{a} sense", "high side", "LM358 · MPSA94"])
    s.block(B3, 400, 130, 82, "anode", ["Anode select", "relay", "A1 / A2"])
    s.block(B2, 530, BW, 56, "anode", ["I_{a} range relay", "20 / 200 mA shunt"])
    ctrl(420, B, "PD4 · OC1B PWM")
    s.poly([(B + BW, 441), (B2, 441)], WIRE, sw=2.4)
    s.poly([(B2 + BW, 441), (B3, 441)], WIRE, sw=2.4)
    s.poly([(B2 + 140, 482), (B2 + 140, 530)], WIRE, sw=2.4, end=False)
    meas([(B2 + 40, 482), (B2 + 40, 508), (R, 508)], "ADC3 U_{a} · ADC4 I_{a}", 508)
    ctrl(558, B2, "PB0 · I_{a} range")
    s.poly([(R, 620), (B3 + 65, 620), (B3 + 65, 482)], CTRL)
    s.text(R + 8, 614, "PB1 · anode select", 12, 400, fill=CTRL)
    to_tube(B3 + 130, 426, "A1")
    to_tube(B3 + 130, 458, "A2")

    # Screen grid
    s.block(B, 670, BW, 82, "screen", ["Screen regulator", "linear, series pass",
                                       "LM358 · IRF740 · BD139"])
    s.pill(B + BW - 40, 670, "+335 V")
    s.block(B2, 670, BW, 82, "screen", ["I_{g2} sense", "high side", "LM358 · MPSA94"])
    ctrl(690, B, "PD5 · OC1A PWM")
    s.poly([(B + BW, 711), (B2, 711)], WIRE, sw=2.4)
    meas([(B2 + 40, 752), (B2 + 40, 778), (R, 778)], "ADC5 U_{g2} · ADC6 I_{g2}", 778)
    to_tube(B2 + BW, 711, "G2")


    # Legend
    ly = 848
    s.poly([(20, ly), (64, ly)], CTRL)
    s.text(74, ly + 5, "control (PWM, logic)", 13, fill="#374151")
    s.poly([(250, ly), (294, ly)], MEAS, dash="6 4")
    s.text(304, ly + 5, "measurement → ADC", 13, fill="#374151")
    s.poly([(470, ly), (514, ly)], WIRE, sw=2.4, end=False)
    s.text(524, ly + 5, "tube electrode / power path", 13, fill="#374151")
    s.pill(760, ly, "+15 V")
    s.text(790, ly + 5, "supply rail feeding the block", 13, fill="#374151")
    s.save(out)


# ------------------------------------------------------------- firmware runtime
def runtime_diagram(out):
    s = Svg(1240, 700)
    COLORS = {"adc": "#1d4ed8", "t2": "#15803d", "int1": "#7e22ce", "uart": "#c2410c",
              "main": "#0f766e"}
    LX, LW = 20, 400
    MX, MW = 640, 210
    RX, RW = 930, 290

    s.text(LX, 34, "Interrupts", 17, 700)
    s.text(LX + 96, 34, "(main.c) — all real-time work", 14, 400, fill=MUTED)
    s.text(MX, 34, "Shared state", 17, 700)
    s.text(MX + 118, 34, "(app.h)", 14, 400, fill=MUTED)
    s.text(RX, 34, "Main loop", 17, 700)
    s.text(RX + 92, 34, "(main.c)", 14, 400, fill=MUTED)

    def card(x, y, w, color, title, lines, sub=None):
        h = 40 + (20 if sub else 0) + 19 * len(lines) + 6
        s.rect(x, y, w, h, "#ffffff", color, r=8, sw=1.6)
        s.rect(x, y, 6, h, color, color, r=3, sw=0)
        s.text(x + 18, y + 25, title, 15, 700, fill=color)
        ty = y + 25
        if sub:
            ty += 20
            s.text(x + 18, ty, sub, 13, 400, fill=MUTED, extra='font-family="DejaVu Sans Mono, monospace"')
        ty += 22
        for ln in lines:
            s.text(x + 18, ty, ln, 13.5, fill="#374151")
            ty += 19
        return y, h

    adc = card(LX, 56, LW, COLORS["adc"], "ADC_vect · 9.6 kHz", [
        "14-step scan: every other conversion reads U_{g1}",
        "and clocks the charge pump; the rest cycle through",
        "T, I_{h}, U_{h}, U_{a}, I_{a}, U_{g2}, I_{g2}",
        "• over-current trips → err",
        "• ramp U_{a} and U_{g2} PWM toward the set points",
        "• I_{a} auto-range 20 ↔ 200 mA",
        "• every 64 scans: latch the averages, regulate",
        "   the heater, check the heatsink temperature",
    ], "adc_scan_sample()")
    t2 = card(LX, adc[0] + adc[1] + 22, LW, COLORS["t2"], "TIMER2_COMP_vect · 1 kHz", [
        "• delay_ms() tick",
        "• button_poll(): a click starts or repeats a measurement",
        "• every 250 ms: seq_tick(), LCD blink, set redraw",
    ])
    i1 = card(LX, t2[0] + t2[1] + 22, LW, COLORS["int1"], "INT1_vect · encoder", [
        "• choose a tube, move between fields",
        "• change the value under the cursor",
        "• switch section, abort a measurement",
    ], "editor_on_encoder()")
    ua = card(LX, i1[0] + i1[1] + 22, LW, COLORS["uart"], "USART_TXC / RXC", [
        "• TX done; ESC received → report_request",
    ], "uart.c")

    # Shared state
    def state(y, title, sub):
        s.rect(MX, y, MW, 62, "#f5f6f8", "#9ca3af", r=8, sw=1.5)
        s.text(MX + 14, y + 25, title, 15, 700)
        s.text(MX + 14, y + 46, sub, 13, fill="#374151")
        return y
    st_avg = state(90, "ADC averages", "sums of 64 samples")
    st_sp = state(230, "Set points  sp", "U_{g1}, U_{h}, I_{h}, U_{a}, U_{g2}")
    st_seq = state(370, "Sequencer, errors", "step, err, lamp, field")
    st_fl = state(540, "Flags", "redraw, report_request")

    # Main loop
    def mbox(y, h, title, lines, mono=True):
        s.rect(RX, y, RW, h, "#e6f6f5", "#3a9d97", r=8, sw=1.5)
        s.text(RX + 14, y + 25, title, 15, 700,
               extra='font-family="DejaVu Sans Mono, monospace"' if mono else "")
        ty = y + 46
        for ln in lines:
            s.text(RX + 14, ty, ln, 13, fill="#374151")
            ty += 18
    mbox(56, 98, "Start-up", ["board and module init", "last tube from EEPROM",
                              "LCD init, splash screen"], mono=False)
    mbox(200, 62, "ui_draw()", ["if redraw: 4 Hz refresh"])
    mbox(300, 136, "panel_update()", ["error → abort the measurement",
                                      "load the selected record",
                                      "readings → live[]",
                                      "save edits to EEPROM",
                                      "build the report line"])
    mbox(474, 62, "send report", ["if report_request"], mono=False)
    s.poly([(RX + 60, 154), (RX + 60, 200)], "#0f766e")
    s.poly([(RX + 60, 262), (RX + 60, 300)], "#0f766e")
    s.poly([(RX + 60, 436), (RX + 60, 474)], "#0f766e")
    s.poly([(RX + RW - 30, 536), (RX + RW - 30, 566), (RX + RW + 6, 566),
            (RX + RW + 6, 231), (RX + RW, 231)], "#0f766e")
    s.text(RX + RW - 36, 586, "for (;;)", 13, 700, "end", "#0f766e",
           extra='font-family="DejaVu Sans Mono, monospace"')

    # Connections between columns: (colour, y on the left column, y on the right
    # column, True if the arrow points right). The lane order is the one with the
    # fewest crossings.
    def route(xl, xr, conns, gap0, step):
        def crossings(order):
            lane = {c: gap0 + i * step for i, c in enumerate(order)}
            n = 0
            for i in order:
                for j in order:
                    if lane[i] >= lane[j]:
                        continue
                    lo, hi = sorted(conns[j][1:3])
                    n += lo < conns[i][2] < hi        # i's right stub vs j's lane
                    lo, hi = sorted(conns[i][1:3])
                    n += lo < conns[j][1] < hi        # j's left stub vs i's lane
            return n
        best = min(itertools.permutations(range(len(conns))), key=crossings)
        for k, i in enumerate(best):
            color, yl, yr, right = conns[i]
            lx = gap0 + k * step
            pts = [(xl, yl), (lx, yl), (lx, yr), (xr, yr)]
            s.poly(pts if right else pts[::-1], color, sw=2)

    C = COLORS
    route(LX + LW, MX, [
        (C["adc"], adc[0] + 60, st_avg + 31, True),
        (C["adc"], adc[0] + 100, st_sp + 18, False),
        (C["adc"], adc[0] + 140, st_seq + 16, True),
        (C["t2"], t2[0] + 48, st_sp + 44, True),
        (C["t2"], t2[0] + 68, st_seq + 32, True),
        (C["t2"], t2[0] + 88, st_fl + 20, True),
        (C["int1"], i1[0] + 70, st_seq + 48, True),
        (C["uart"], ua[0] + 50, st_fl + 44, True),
    ], LX + LW + 28, 24)

    teal = C["main"]
    route(MX + MW, RX, [
        (teal, st_avg + 31, 330, True),
        (teal, st_sp + 31, 360, False),
        (teal, st_seq + 20, 231, True),
        (teal, st_seq + 44, 400, True),
        (teal, st_fl + 31, 505, True),
    ], MX + MW + 18, 14)

    s.text(MX, 650, "Arrows point from the writer to the reader;", 13, fill=MUTED)
    s.text(MX, 669, "the colour shows which context does it.", 13, fill=MUTED)
    s.save(out)


# ------------------------------------------------------------- sequencer timing
def sequencer_diagram(out):
    s = Svg(1250, 600)
    X0, S1, S2 = 260, 7.4, 15

    def x(t):
        return X0 + (112 - t) * S1 if t >= 16 else X0 + 96 * S1 + (16 - t) * S2
    XW0 = 130                      # heater on (start of the warm-up)
    XEND = x(0)

    # Phase band
    phases = [
        (XW0, x(112), "Warm-up 1–9 min", "#fff4e5", "#e9a24f"),
        (x(112), x(80), "Power up", "#eef2f7", "#94a3b8"),
        (x(80), x(60), "Measure S", "#e8f0fe", "#6b93db"),
        (x(60), x(24), "Measure R", "#e8f0fe", "#6b93db"),
        (x(24), x(16), "K", "#e8f0fe", "#6b93db"),
        (x(16), x(8), "Switch off", "#eef2f7", "#94a3b8"),
        (x(8), x(4), "Beep", "#eef2f7", "#94a3b8"),
        (x(4), x(3), "", "#fde68a", "#d97706"),
        (x(3), XEND, "Off", "#eef2f7", "#94a3b8"),
    ]
    PY, PH = 92, 30
    for x1, x2, lab, f, st in phases:
        s.rect(x1, PY, x2 - x1, PH, f, st, r=3, sw=1.2)
        if lab:
            s.text((x1 + x2) / 2, PY + 20, lab, 13, 700, "middle")
    s.text(x(3.5), PY - 8, "Hold", 13, 700, "middle", "#b45309")

    # Re-measure arc over the top
    s.path(f"M{x(3.5)},{PY - 26} C{x(3.5)},20 {x(112)},20 {x(112)},{PY - 4}", "#b45309", sw=1.8)
    s.text((x(3.5) + x(112)) / 2, 30, "click in Hold: re-measure, the heater is still warm",
           13, 400, "middle", "#b45309")

    # Warm-up axis break
    bx = (XW0 + x(112)) / 2 + 30
    rows = [("Heater", 150), ("U_{g1}", 210), ("U_{a}", 270), ("U_{g2}", 330),
            ("Readings", 390), ("Buzzer", 440)]
    for lab, y in rows:
        s.text(20, y + 26, lab, 15, 700)
        s.add(f'<line x1="{XW0 - 10}" y1="{y + 40}" x2="{XEND + 10}" y2="{y + 40}" '
              f'stroke="#e5e7eb" stroke-width="1"/>')

    def trace(pts, color=INK):
        s.poly(pts, color, sw=2.2, end=False)

    # Heater
    y = 150
    trace([(XW0 - 10, y + 36), (XW0, y + 36), (XW0, y + 10), (x(3), y + 10),
           (x(3), y + 36), (XEND + 10, y + 36)], "#c2410c")
    s.text(x(3) - 6, y + 4, "off at step 3", 12, 400, "end", MUTED)

    # -Ug1: higher = less negative
    y = 210
    off, lo, nom, hi = y + 36, y + 20, y + 13, y + 6
    trace([(XW0 - 10, off), (x(112), off), (x(112), lo), (x(78), lo), (x(78), hi),
           (x(60), hi), (x(60), nom), (x(10), nom), (x(10), off), (XEND + 10, off)], CTRL)
    s.text(XW0, off - 5, "−24 V", 12, 400, fill=MUTED)
    s.text((x(112) + x(78)) / 2, lo - 5, "U_{g} − 0.4 V", 12, 400, "middle", MUTED)
    s.text((x(78) + x(60)) / 2, hi - 5, "U_{g} + 0.4 V", 12, 400, "middle", MUTED)
    s.text((x(60) + x(24)) / 2, nom - 5, "U_{g}", 12, 400, "middle", MUTED)

    # Ua
    y = 270
    z, l, n, h = y + 36, y + 20, y + 13, y + 6
    trace([(XW0 - 10, z), (x(96), z), (x(93), n), (x(44), n), (x(44), l), (x(34), l),
           (x(34), h), (x(24), h), (x(24), n), (x(12), n), (x(11), z), (XEND + 10, z)], CTRL)
    s.text((x(96) + x(44)) / 2, n - 5, "U_{a}", 12, 400, "middle", MUTED)
    s.text((x(44) + x(34)) / 2, l + 16, "−10 V", 12, 400, "middle", MUTED)
    s.text((x(34) + x(24)) / 2, h - 5, "+10 V", 12, 400, "middle", MUTED)

    # Ug2
    y = 330
    trace([(XW0 - 10, y + 36), (x(88), y + 36), (x(85), y + 10), (x(14), y + 10),
           (x(13), y + 36), (XEND + 10, y + 36)], CTRL)
    s.text((x(88) + x(14)) / 2, y + 5, "U_{g2}", 12, 400, "middle", MUTED)

    # Readings
    y = 390
    def mark(t, lab, dy=0, color=MEAS):
        cx, cy = x(t), y + 20
        s.add(f'<path d="M{cx},{cy - 7} L{cx + 7},{cy} L{cx},{cy + 7} L{cx - 7},{cy} z" '
              f'fill="{color}"/>')
        s.text(cx + 11, cy + 5 + dy, lab, 13, 400, fill=color)
    mark(80, "I_{a}, U_{g1}")
    mark(62, "I_{a}, U_{g1} → S")
    mark(36, "I_{a}, U_{a}")
    mark(26, "→ R", -10)
    mark(24, "K = S·R", 10)
    s.add(f'<path d="M{x(16)},{y + 13} L{x(16) + 7},{y + 20} L{x(16)},{y + 27} '
          f'L{x(16) - 7},{y + 20} z" fill="#0f766e"/>')
    s.text(x(16) + 11, y + 10, "latch: LCD,", 13, 400, fill="#0f766e")
    s.text(x(16) + 11, y + 28, "RS-232 report", 13, 400, fill="#0f766e")

    # Buzzer
    y = 440
    trace([(XW0 - 10, y + 30), (x(8), y + 30), (x(8), y + 8), (x(5), y + 8),
           (x(5), y + 30), (XEND + 10, y + 30)], "#374151")
    s.text(x(8) - 8, y + 24, "one long beep; two short on error", 12, 400, "end", MUTED)

    # Step axis
    AY = 500
    s.add(f'<line x1="{x(112)}" y1="{AY}" x2="{XEND}" y2="{AY}" stroke="{INK}" stroke-width="1.4"/>')
    ticks = [112, 96, 88, 80, 78, 62, 60, 44, 36, 34, 26, 24, 16, 14, 12, 10, 8, 4, 3, 1, 0]
    low = {78, 60, 34, 24, 3, 0}
    for t in ticks:
        s.add(f'<line x1="{x(t)}" y1="{AY - 5}" x2="{x(t)}" y2="{AY + 5}" stroke="{INK}" stroke-width="1.4"/>')
        s.add(f'<line x1="{x(t)}" y1="{PY + PH}" x2="{x(t)}" y2="{AY - 5}" stroke="#d1d5db" '
              f'stroke-width="1" stroke-dasharray="2 4"/>')
        s.text(x(t), AY + (36 if t in low else 20), str(t), 12, 400, "middle", "#374151")
    # axis break between warm-up and the measurement
    s.add(f'<line x1="{XW0}" y1="{AY}" x2="{bx - 8}" y2="{AY}" stroke="{INK}" stroke-width="1.4"/>')
    s.add(f'<line x1="{bx + 8}" y1="{AY}" x2="{x(112)}" y2="{AY}" stroke="{INK}" stroke-width="1.4"/>')
    for dx in (-8, 8):
        s.add(f'<line x1="{bx + dx - 4}" y1="{AY + 8}" x2="{bx + dx + 4}" y2="{AY - 8}" '
              f'stroke="{INK}" stroke-width="1.4"/>')
    s.text(20, AY + 5, "Step", 15, 700)
    s.text(XW0, AY + 22, "click", 12, 700, fill="#374151")

    # Abort jump
    JY = 568
    s.add(f'<line x1="{XW0}" y1="{JY - 6}" x2="{XW0}" y2="{JY + 6}" stroke="{RAIL}" stroke-width="1.8"/>')
    s.poly([(XW0, JY), (x(16), JY), (x(16), AY + 27)], RAIL, sw=1.8, dash="6 4")
    s.text(XW0 + 10, JY - 8,
           "error or encoder turn: jump to step 16, then switch off (an error also stops the heater)",
           13, 400, fill=RAIL)
    s.text(x(16) + 12, JY + 5, "time runs right, 250 ms per step;", 12, 400, fill=MUTED)
    s.text(x(16) + 12, JY + 21, "steps 16…0 drawn at 2× scale", 12, 400, fill=MUTED)
    s.save(out)


if __name__ == "__main__":
    here = Path(__file__).resolve().parent
    images = here.parent / "images"
    for name, draw in [("block_diagram", block_diagram),
                       ("firmware_runtime", runtime_diagram),
                       ("sequencer_timing", sequencer_diagram)]:
        svg = here / f"{name}.svg"
        draw(svg)
        subprocess.run(["rsvg-convert", "-z", "2", "-o", str(images / f"{name}.png"),
                        str(svg)], check=True)
        print(images / f"{name}.png")
