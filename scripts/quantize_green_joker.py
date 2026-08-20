"""Quantize green joker.png onto gfx2 palette (+#4F6367) and append as slot 2 (extend 64->96)."""
from PIL import Image

SRC = "graphics/green_dl/green joker.png"
DST = "graphics/joker_gfx2.png"
BAK = "graphics/joker_gfx2.pre_green.png"

# 1. Full-pixel palette of gfx2 (15 colors incl. transparent) + add #4F6367 (16th slot)
pal_img = Image.open(DST).convert("RGBA")
pal_px = pal_img.load()
palette = []
for y in range(32):
    for x in range(pal_img.size[0]):
        c = pal_px[x, y]
        if c not in palette:
            palette.append(c)
ADD = (79, 99, 103, 255)  # #4F6367 - the only missing Green Joker color
assert ADD not in palette and len(palette) < 16
palette.append(ADD)
print(f"target palette: {len(palette)} colors (16/16 full)")

# 2. Quantize source pixels to nearest palette color
src = Image.open(SRC).convert("RGBA")
src_px = src.load()
out = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
out_px = out.load()

def dist(a, b):
    return (a[0]-b[0])**2 + (a[1]-b[1])**2 + (a[2]-b[2])**2

mapping = {}
for y in range(32):
    for x in range(32):
        c = src_px[x, y]
        if c[3] == 0:
            continue
        if c not in mapping:
            mapping[c] = min(palette, key=lambda p: dist(c, p))
        out_px[x, y] = mapping[c]

print("\ncolor mapping (src -> palette):")
for c, p in sorted(mapping.items(), key=lambda kv: dist(kv[0], kv[1]), reverse=True):
    print(f"  #{c[0]:02X}{c[1]:02X}{c[2]:02X} -> #{p[0]:02X}{p[1]:02X}{p[2]:02X}  (dE2={dist(c,p)})")

# 3. Back up, extend canvas 64->96, paste original + new slot
orig = Image.open(DST).convert("RGBA")
orig.save(BAK)
canvas = Image.new("RGBA", (96, 32), (0, 0, 0, 0))
canvas.paste(orig, (0, 0))
canvas.paste(out, (64, 0))
canvas.save(DST)
print(f"\nsaved {DST}: {orig.size} -> {canvas.size}")
print(f"backup: {BAK}")

# 4. Verify first 64px identical to backup
a = Image.open(BAK).convert("RGBA")
b = Image.open(DST).convert("RGBA")
diff = 0
for y in range(32):
    for x in range(64):
        if a.getpixel((x, y)) != b.getpixel((x, y)):
            diff += 1
print("prefix pixel diffs vs backup:", diff, "(must be 0)")
