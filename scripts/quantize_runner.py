"""Append Runner to gfx0 slot 34 (extend 1088->1120). Zero new colors (art fits the full 16-color palette exactly)."""
from PIL import Image

SRC = "graphics/runner_dl/Runner.png"
DST = "graphics/joker_gfx0.png"
BAK = "graphics/joker_gfx0.pre_runner.png"

orig = Image.open(DST).convert("RGBA")
op = orig.load()
palette = []
for y in range(orig.size[1]):
    for x in range(orig.size[0]):
        c = op[x, y]
        if c not in palette:
            palette.append(c)
print(f"existing palette: {len(palette)} entries (full={len(palette)>=16})")

src = Image.open(SRC).convert("RGBA")
sp = src.load()

def dist(a, b):
    return (a[0]-b[0])**2 + (a[1]-b[1])**2 + (a[2]-b[2])**2

new_colors = []
mapping = {}
for y in range(32):
    for x in range(32):
        c = sp[x, y]
        if c[3] == 0 or c in mapping:
            continue
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
    print("FATAL: palette would exceed 16")
    raise SystemExit(1)

orig.save(BAK)
canvas = Image.new("RGBA", (orig.size[0] + 32, 32), (0, 0, 0, 0))
canvas.paste(orig, (0, 0))
cp = canvas.load()
for y in range(32):
    for x in range(32):
        c = sp[x, y]
        if c[3] != 0:
            cp[x + orig.size[0], y] = mapping[c]
canvas.save(DST)
print(f"saved {DST}: {orig.size} -> {canvas.size}, new colors: {len(new_colors)}")

a = Image.open(BAK).convert("RGBA")
b = Image.open(DST).convert("RGBA")
diff = sum(1 for y in range(32) for x in range(orig.size[0]) if a.getpixel((x, y)) != b.getpixel((x, y)))
print("prefix pixel diffs:", diff, "(must be 0)")
