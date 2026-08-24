"""Append Gift Card to gfx1 slot 2 (extend 64->96) + add 1 new palette color (#FEBC43).
Greedy/Lusty keep their palette indices (new color appended last)."""
from PIL import Image

SRC = "graphics/giftcard_dl/GiftCard.png"
DST = "graphics/joker_gfx1.png"
BAK = "graphics/joker_gfx1.pre_giftcard.png"

orig = Image.open(DST).convert("RGBA")
op = orig.load()
palette = []
for y in range(orig.size[1]):
    for x in range(orig.size[0]):
        c = op[x, y]
        if c not in palette:
            palette.append(c)
print(f"existing palette: {len(palette)} (idx0=transparent, 1-14=Greedy/Lusty)")

src = Image.open(SRC).convert("RGBA")
sp = src.load()
src_colors = []
for y in range(32):
    for x in range(32):
        c = sp[x, y]
        if c[3] != 0 and c not in src_colors:
            src_colors.append(c)
print(f"source colors: {len(src_colors)}")

def dist(a, b):
    return (a[0]-b[0])**2 + (a[1]-b[1])**2 + (a[2]-b[2])**2

mapping = {}
new_colors = []
for c in sorted(src_colors, key=lambda c: min(dist(c, p) for p in palette[1:]), reverse=True):
    best = min(palette[1:], key=lambda p: dist(c, p))
    d = dist(c, best)
    if d <= 100:
        mapping[c] = best
        print(f"  #{c[0]:02X}{c[1]:02X}{c[2]:02X} -> #{best[0]:02X}{best[1]:02X}{best[2]:02X} (dE2={d})")
    else:
        mapping[c] = c
        new_colors.append(c)
        print(f"  #{c[0]:02X}{c[1]:02X}{c[2]:02X} -> NEW (dE2={d})")

if len(palette) + len(new_colors) > 16:
    print(f"FATAL: palette would exceed 16 ({len(palette)}+{len(new_colors)})")
    raise SystemExit(1)
print(f"new colors: {len(new_colors)} (palette {len(palette)} -> {len(palette)+len(new_colors)})")

orig.save(BAK)
canvas = Image.new("RGBA", (96, 32), (0, 0, 0, 0))
canvas.paste(orig, (0, 0))
cp = canvas.load()
for y in range(32):
    for x in range(32):
        c = sp[x, y]
        if c[3] != 0:
            cp[x + 64, y] = mapping[c]
canvas.save(DST)
print(f"saved {DST}: {orig.size} -> {canvas.size}, backup: {BAK}")

a = Image.open(BAK).convert("RGBA")
b = Image.open(DST).convert("RGBA")
diff = sum(1 for y in range(32) for x in range(64) if a.getpixel((x, y)) != b.getpixel((x, y)))
print("prefix pixel diffs:", diff, "(must be 0)")
