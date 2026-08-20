"""Quantize square joker.png to gfx0 palette and append as slot 32 (extend 1024->1056)."""
from PIL import Image

SRC = "graphics/square_dl/square joker.png"
DST = "graphics/joker_gfx0.png"
BAK = "graphics/joker_gfx0.pre_square.png"

# 1. Extract gfx0 palette (16 colors incl. transparent at idx 0)
pal_img = Image.open(DST).convert("RGBA")
pal_px = pal_img.load()
palette = []
seen = set()
for y in range(0, 32, 8):
    for x in range(0, 1024, 8):
        c = pal_px[x, y]
        if c not in seen:
            seen.add(c)
            palette.append(c)
        if len(palette) == 16:
            break
    if len(palette) == 16:
        break
print("palette (16):")
for i, c in enumerate(palette):
    print(f"  [{i}] {c}")

# 2. Load source, quantize each pixel to nearest palette color
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
            best = min(palette, key=lambda p: dist(c, p))
            mapping[c] = best
        out_px[x, y] = mapping[c]

print("\ncolor mapping (src -> palette):")
for c, p in sorted(mapping.items()):
    print(f"  {c} -> {p}  (dE2={dist(c,p)})")

# 3. Back up original, extend canvas to 1056 wide, paste original + new slot
orig = Image.open(DST).convert("RGBA")
orig.save(BAK)
canvas = Image.new("RGBA", (1056, 32), (0, 0, 0, 0))
canvas.paste(orig, (0, 0))
canvas.paste(out, (1024, 0))
canvas.save(DST)
print(f"\nsaved {DST}: {orig.size} -> {canvas.size}")
print(f"backup: {BAK}")

# 4. Verify first 1024px identical to backup
a = Image.open(BAK).convert("RGBA")
b = Image.open(DST).convert("RGBA")
diff = 0
for y in range(32):
    for x in range(1024):
        if a.getpixel((x, y)) != b.getpixel((x, y)):
            diff += 1
print("prefix pixel diffs vs backup:", diff, "(must be 0)")
