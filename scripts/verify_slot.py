#!/usr/bin/env python3
"""Reconstruct slot 31 of joker_gfx0 from built .s (tiles + palette) and diff vs source PNG."""
import re, sys
from PIL import Image

def bgr555(v):
    b = (v & 0x1F) << 3
    g = ((v >> 5) & 0x1F) << 3
    r = ((v >> 10) & 0x1F) << 3
    return (r | (r >> 5), g | (g >> 5), b | (b >> 5))  # 5->8 bit expand

def parse_s(path):
    with open(path, encoding="utf-8") as f:
        lines = f.readlines()
    pal = None
    tiles = []
    in_tiles = False
    for ln in lines:
        if 'joker_gfx0Pal:' in ln:
            pal = []
            in_tiles = False
            continue
        if 'joker_gfx0Tiles:' in ln:
            pal_done = True
            in_tiles = True
            continue
        if in_tiles:
            # .word 0x...,0x... or .byte ... lines
            vals = re.findall(r'0x([0-9A-Fa-f]{8})', ln)
            for x in vals:
                v = int(x, 16)
                tiles.extend([(v >> (8 * i)) & 0xFF for i in range(4)])
        elif pal is not None:
            vals = re.findall(r'0x([0-9A-Fa-f]{4})', ln)
            for x in vals:
                pal.append(bgr555(int(x, 16)))
    return pal, tiles

def build_slot(pal, tiles, slot_idx):
    """slot_idx-th 32x32 slot; 4x4 tiles of 8x8, 4bpp."""
    per_slot = 16
    start = slot_idx * per_slot
    img = Image.new("RGBA", (32, 32))
    for ty in range(4):
        for tx in range(4):
            t = start + ty * 4 + tx
            if t * 32 + 32 > len(tiles):
                return None
            tile = tiles[t * 32:(t + 1) * 32]
            for py in range(8):
                for px in range(8):
                    byte = tile[py * 4 + px // 2]
                    idx = (byte >> (4 if px % 2 == 0 else 0)) & 0xF
                    c = pal[idx] if idx < len(pal) else (255, 0, 255)
                    if idx == 0:
                        c = (0, 0, 0, 0)
                    else:
                        c = (c[0], c[1], c[2], 255)
                    img.putpixel((tx * 8 + px, ty * 8 + py), c)
    return img

if __name__ == "__main__":
    pal, tiles = parse_s(sys.argv[1] if len(sys.argv) > 1 else "build/joker_gfx0.s")
    print("palette:", len(pal) if pal else "MISSING", "| tile bytes:", len(tiles))
    img = build_slot(pal, tiles, 31)
    if img is None:
        print("FAIL: tile data too short"); sys.exit(1)
    v3 = Image.open("graphics/ttm_v2/To the moon v3.png").convert("RGBA")
    img.save("build/slot31_rebuilt.png")
    # compare with v3: count pixel diffs and opaque-color diffs
    diff = 0
    for y in range(32):
        for x in range(32):
            a, b = img.getpixel((x, y)), v3.getpixel((x, y))
            if a != b:
                diff += 1
    print("diff pixels vs v3:", diff, "/1024")
    # count colors in rebuilt
    cs = {}
    for p in img.getdata():
        if p[3] > 0:
            cs[p[:3]] = cs.get(p[:3], 0) + 1
    print("rebuilt colors:", len(cs))
