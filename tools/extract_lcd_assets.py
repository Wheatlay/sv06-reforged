"""Extract status-screen fonts and bitmaps from Marlin source into JSON for the HTML mockup.

Usage: uv run --no-project tools/extract_lcd_assets.py <Marlin dir> <out.json>
"""
import json
import re
import sys
from pathlib import Path


def arrays(text):
    """Yield (name, [bytes]) for every C array initializer, in file order."""
    for m in re.finditer(r"(\w+)\s*\[\s*\d*\s*\][^=]*=\s*\{(.*?)\};", text, re.S):
        body = re.sub(r"//.*", "", m.group(2))
        vals = []
        for tok in re.findall(r"0x[0-9A-Fa-f]+|\bB[01]{1,8}\b|\b\d+\b", body):
            if tok.startswith("0x"):
                vals.append(int(tok, 16))
            elif tok.startswith("B"):
                vals.append(int(tok[1:], 2))
            else:
                vals.append(int(tok))
        yield m.group(1), vals


def first(path, name):
    for n, v in arrays(path.read_text(encoding="utf-8", errors="replace")):
        if n == name:
            return v
    raise KeyError(f"{name} not in {path}")


def s8(b):
    return b - 256 if b > 127 else b


def parse_u8g_font(data):
    """u8g format-0/1 font -> {codepoint: {w,h,dx,x,y,rows}} plus ascent/descent."""
    fmt, enc_start, enc_end = data[0], data[10], data[11]
    font = {"ascent": s8(data[13]), "descent": s8(data[14]), "glyphs": {}}
    p = 17
    for code in range(enc_start, enc_end + 1):
        if p >= len(data):
            break
        if data[p] == 255:
            p += 1
            continue
        if fmt == 1:  # packed 3-byte header, see u8g_CopyGlyphDataToCache in U8glib
            b0, b1, b2 = data[p], data[p + 1], data[p + 2]
            x, y, w, h, size, dx, hdr = b0 >> 4, (b0 & 15) - 2, b1 >> 4, b1 & 15, b2 & 15, b2 >> 4, 3
        else:
            w, h, size, dx, x, y, hdr = data[p], data[p + 1], data[p + 2], data[p + 3], s8(data[p + 4]), s8(data[p + 5]), 6
        raw = data[p + hdr:p + hdr + size]
        bw = (w + 7) // 8
        rows = ["".join("1" if raw[r * bw + c // 8] & (0x80 >> (c % 8)) else "0" for c in range(w)) for r in range(h)]
        font["glyphs"][code] = {"w": w, "h": h, "dx": dx, "x": x, "y": y, "rows": rows}
        p += hdr + size
    return font


def bitmap(vals, width):
    bw = (width + 7) // 8
    return ["".join("1" if vals[r * bw + c // 8] & (0x80 >> (c % 8)) else "0" for c in range(width))
            for r in range(len(vals) // bw)]


def main(marlin, out):
    marlin = Path(marlin)
    dogm = marlin / "src/lcd/dogm"
    st = dogm / "status"
    assets = {
        "font_menu": parse_u8g_font(first(dogm / "fontdata/fontdata_ISO10646_1.h", "ISO10646_1_5x7")),
        "font_small": parse_u8g_font(first(dogm / "fontdata/fontdata_6x9_marlin.h", "u8g_font_6x9")),
        "font_tiny": parse_u8g_font(first(Path(__file__).parent / "fonts/u8g_font_4x6r.h", "u8g_font_4x6r")),
        **{f"font_{n}": parse_u8g_font(first(Path(__file__).parent / "fonts/u8g_extra.h", f"u8g_font_{n}"))
           for n in ("9x18Br", "7x14Br", "10x20r", "freedoomr10r", "freedoomr25n", "6x10r")},
        "logo": bitmap(first(marlin / "_Statusscreen.h", "status_logo_bmp"), 40),
        "hotend_off": bitmap(first(st / "hotend.h", "status_hotend_a_bmp"), 16),
        "hotend_on": bitmap(first(st / "hotend.h", "status_hotend_b_bmp"), 16),
        "bed_off": bitmap(first(st / "bed.h", "status_bed_bmp"), 24),
        "bed_on": bitmap(first(st / "bed.h", "status_bed_on_bmp"), 24),
        "fan0": bitmap(first(st / "fan.h", "status_fan0_bmp"), 20),
        "fan1": bitmap(first(st / "fan.h", "status_fan1_bmp"), 20),
    }
    Path(out).write_text(json.dumps(assets, separators=(",", ":")), encoding="utf-8")
    for k, v in assets.items():
        print(k, len(v["glyphs"]) if isinstance(v, dict) else f"{len(v[0])}x{len(v)}")


if __name__ == "__main__":
    main(*sys.argv[1:3])
