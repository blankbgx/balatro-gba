"""Fill Ancient Joker v2 into the empty gfx18 fallback sheet (10 colors < 16, zero quant loss)."""
from PIL import Image

SRC = "graphics/ancient_dl/AncientJoker v2.png"
DST = "graphics/joker_gfx18.png"
BAK = "graphics/joker_gfx18.pre_ancient.png"

src = Image.open(SRC).convert("RGBA")
print(f"source: {src.size}")

# 1. Collect source colors + check 5-bit quantization collisions
px = src.load()
colors = []
for y in range(32):
    for x in range(32):
        c = px[x, y]
        if c[3] != 0 and c not in colors:
            colors.append(c)
print(f"source colors: {len(colors)}")
q5 = {}
for c in colors:
    key = (c[0] >> 3, c[1] >> 3, c[2] >> 3)
    if key in q5:
        print(f"  5-bit COLLISION: #{c[0]:02X}{c[1]:02X}{c[2]:02X} vs #{q5[key][0]:02X}{q5[key][1]:02X}{q5[key][2]:02X}")
    q5[key] = c
print(f"unique 5-bit colors: {len(q5)} (must be <= 15 non-transparent)")

# 2. Back up empty sheet, fill in the art
orig = Image.open(DST).convert("RGBA")
orig.save(BAK)
canvas = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
canvas.paste(src, (0, 0))
canvas.save(DST)
print(f"saved {DST} ({orig.size} -> {canvas.size}), backup: {BAK}")

# 3. Sanity: pixels outside source bbox stay transparent
sp = src.load()
op = canvas.load()
outside = 0
for y in range(32):
    for x in range(32):
        if sp[x, y][3] == 0 and op[x, y][3] != 0:
            outside += 1
print("non-transparent pixels outside source alpha:", outside, "(must be 0)")
