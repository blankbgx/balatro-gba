"""Quantize supernova.png onto gfx6 palette and append as slot 4 (extend 128->160)."""
from PIL import Image

SRC = "graphics/supernova_dl/supernova.png"
DST = "graphics/joker_gfx6.png"
BAK = "graphics/joker_gfx6.pre_supernova.png"

# 1. Full-pixel palette of gfx6 (16 colors incl. transparent - already full)
pal_img = Image.open(DST).convert("RGBA")
pal_px = pal_img.load()
palette = []
for y in range(32):
    for x in range(pal_img.size[0]):
        c = pal_px[x, y]
        if c not in palette:
            palette.append(c)
print(f"target palette: {len(palette)} colors (16/16 full - no new colors needed)")

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
max_d = 0
for c, p in sorted(mapping.items(), key=lambda kv: dist(kv[0], kv[1]), reverse=True):
    d = dist(c, p)
    max_d = max(max_d, d)
    print(f"  #{c[0]:02X}{c[1]:02X}{c[2]:02X} -> #{p[0]:02X}{p[1]:02X}{p[2]:02X}  (dE2={d})")
print(f"max dE2={max_d} (sqrt={max_d**0.5:.1f})")

# 3. Back up, extend canvas 128->160, paste original + new slot
orig = Image.open(DST).convert("RGBA")
orig.save(BAK)
canvas = Image.new("RGBA", (160, 32), (0, 0, 0, 0))
canvas.paste(orig, (0, 0))
canvas.paste(out, (128, 0))
canvas.save(DST)
print(f"\nsaved {DST}: {orig.size} -> {canvas.size}")
print(f"backup: {BAK}")

# 4. Verify first 128px identical to backup
a = Image.open(BAK).convert("RGBA")
b = Image.open(DST).convert("RGBA")
diff = 0
for y in range(32):
    for x in range(128):
        if a.getpixel((x, y)) != b.getpixel((x, y)):
            diff += 1
print("prefix pixel diffs vs backup:", diff, "(must be 0)")
