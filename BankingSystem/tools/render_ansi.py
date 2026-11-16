#!/usr/bin/env python3
"""Render ANSI-coloured terminal text to a PNG that looks like a terminal window."""
import re
from PIL import Image, ImageDraw, ImageFont

FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
FONT_B = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"
BG, FG = (13, 17, 23), (201, 209, 217)
COLORS = {30: (72, 79, 88), 31: (255, 123, 114), 32: (63, 185, 80), 33: (230, 180, 60), 34: (88, 166, 255),
          35: (188, 140, 255), 36: (57, 197, 207), 37: (201, 209, 217), 90: (139, 148, 158)}
SGR = re.compile(r"\x1b\[([0-9;]*)m")

def parse(text):
    """-> list of lines; each line is a list of (char, colour, bold)."""
    lines, cur, color, bold = [], [], FG, False
    pos = 0
    for m in SGR.finditer(text):
        for ch in text[pos:m.start()]:
            if ch == "\n":
                lines.append(cur); cur = []
            else:
                cur.append((ch, color, bold))
        pos = m.end()
        for code in (m.group(1) or "0").split(";"):
            code = int(code or 0)
            if code == 0: color, bold = FG, False
            elif code == 1: bold = True
            elif code in COLORS: color = COLORS[code]
    for ch in text[pos:]:
        if ch == "\n":
            lines.append(cur); cur = []
        else:
            cur.append((ch, color, bold))
    lines.append(cur)
    return lines

# box-drawing characters are drawn as real lines so rows connect seamlessly
SINGLE = {"─": "LR", "│": "UD", "┌": "RD", "┐": "LD", "└": "RU", "┘": "LU", "├": "UDR", "┤": "UDL", "┬": "LRD", "┴": "LRU", "┼": "UDLR"}
DOUBLE = {"═", "║", "╔", "╗", "╚", "╝"}

def draw_cell(d, ch, x, y, cw, lh, color, bg=BG):
    cx, cy = x + cw / 2, y + lh / 2
    x2, y2 = x + cw, y + lh
    if ch in SINGLE:
        dirs = SINGLE[ch]
        if "L" in dirs: d.line([x, cy, cx, cy], fill=color, width=2)
        if "R" in dirs: d.line([cx, cy, x2, cy], fill=color, width=2)
        if "U" in dirs: d.line([cx, y, cx, cy], fill=color, width=2)
        if "D" in dirs: d.line([cx, cy, cx, y2], fill=color, width=2)
        return True
    if ch in DOUBLE:
        o = 2.5
        if ch == "═":
            d.line([x, cy - o, x2, cy - o], fill=color, width=2); d.line([x, cy + o, x2, cy + o], fill=color, width=2)
        elif ch == "║":
            d.line([cx - o, y, cx - o, y2], fill=color, width=2); d.line([cx + o, y, cx + o, y2], fill=color, width=2)
        elif ch == "╔":
            d.line([cx - o, cy - o, x2, cy - o], fill=color, width=2); d.line([cx - o, cy - o, cx - o, y2], fill=color, width=2)
            d.line([cx + o, cy + o, x2, cy + o], fill=color, width=2); d.line([cx + o, cy + o, cx + o, y2], fill=color, width=2)
        elif ch == "╗":
            d.line([x, cy - o, cx + o, cy - o], fill=color, width=2); d.line([cx + o, cy - o, cx + o, y2], fill=color, width=2)
            d.line([x, cy + o, cx - o, cy + o], fill=color, width=2); d.line([cx - o, cy + o, cx - o, y2], fill=color, width=2)
        elif ch == "╚":
            d.line([cx - o, y, cx - o, cy + o], fill=color, width=2); d.line([cx - o, cy + o, x2, cy + o], fill=color, width=2)
            d.line([cx + o, y, cx + o, cy - o], fill=color, width=2); d.line([cx + o, cy - o, x2, cy - o], fill=color, width=2)
        elif ch == "╝":
            d.line([cx + o, y, cx + o, cy + o], fill=color, width=2); d.line([x, cy + o, cx + o, cy + o], fill=color, width=2)
            d.line([cx - o, y, cx - o, cy - o], fill=color, width=2); d.line([x, cy - o, cx - o, cy - o], fill=color, width=2)
        return True
    if ch == "█":
        d.rectangle([x, y + lh * 0.16, x2 - 0.5, y2 - lh * 0.16], fill=color); return True
    if ch == "░":
        dim = tuple(int(bg[i] + (color[i] - bg[i]) * 0.28) for i in range(3))
        d.rectangle([x, y + lh * 0.16, x2 - 0.5, y2 - lh * 0.16], fill=dim); return True
    return False

def render(text, out_path, title="nova-bank", cols=80, size=19):
    text = text.replace("\r", "")
    lines = parse(text)
    while lines and not lines[-1]: lines.pop()
    while lines and not lines[0]: lines.pop(0)
    font, fontb = ImageFont.truetype(FONT, size), ImageFont.truetype(FONT_B, size)
    cw = font.getlength("M")
    lh = int(size * 1.42)
    pad, bar = 22, 38
    width = int(cw * cols + pad * 2)
    height = bar + pad + lh * len(lines) + pad
    img = Image.new("RGB", (width, height), BG)
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, width, bar], fill=(22, 27, 34))
    for i, c in enumerate([(255, 95, 86), (255, 189, 46), (39, 201, 63)]):
        d.ellipse([16 + i * 24, 12, 30 + i * 24, 26], fill=c)
    d.text((width // 2, bar // 2), title, font=ImageFont.truetype(FONT, 15), fill=(139, 148, 158), anchor="mm")
    y = bar + pad
    for line in lines:
        x = pad
        for ch, color, bold in line:
            if not draw_cell(d, ch, x, y, cw, lh, color):
                d.text((x, y + (lh - size * 1.17) / 2), ch, font=fontb if bold else font, fill=color)
            x += cw
        y += lh
    img.save(out_path, optimize=True)
    return img.size

if __name__ == "__main__":
    import sys
    render(open(sys.argv[1], encoding="utf-8").read(), sys.argv[2])
