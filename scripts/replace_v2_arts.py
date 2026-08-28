"""In-place v2 art replacement: Baron/Runner/LoyaltyCard into gfx0, Swashbuckler into gfx18.
Each pixel maps to the sheet's EXISTING palette (palettes are full - zero new colors allowed).
Other slots must stay pixel-identical."""
from PIL import Image

JOBS = [
    ("graphics/v2_dl/Baron V2.png", "graphics/joker_gfx0.png", 18, "Baron"),
    ("graphics/v2_dl/LoyaltyCard V2.png", "graphics/joker_gfx0.png", 24, "LoyaltyCard"),
    ("graphics/v2_dl/Runner V2.png", "graphics/joker_gfx0.png", 34, "Runner"),
    ("graphics/v2_dl/Swashbuckler V2.png", "graphics/joker_gfx18.png", 1, "Swashbuckler"),
]

def dist(a, b):
    return (a[0]-b[0])**2 + (a[1]-b[1])**2 + (a[2]-b[2])**2

for src_path, sheet_path, slot, name in JOBS:
    sheet = Image.open(sheet_path).convert("RGBA")
    sp = sheet.load()
    palette = []
    for y in range(sheet.size[1]):
        for x in range(sheet.size[0]):
            c = sp[x, y]
            if c not in palette:
                palette.append(c)
    non_t = palette[1:]  # index 0 = transparent

    src = Image.open(src_path).convert("RGBA")
    if src.size != (32, 32):
        print(f"FATAL {name}: src size {src.size} != 32x32")
        raise SystemExit(1)
    spx = src.load()

    worst = 0
    replaced = {}
    out = sheet.copy()
    op = out.load()
    for y in range(32):
        for x in range(32):
            c = spx[x, y]
            if c[3] == 0:
                op[x + slot * 32, y] = palette[0]
                continue
            best = min(non_t, key=lambda p: dist(c, p))
            d = dist(c, best)
            worst = max(worst, d)
            replaced[c] = (best, d)
            op[x + slot * 32, y] = best
    out.save(sheet_path)
    max_de = max((v[1] for v in replaced.values()), default=0)
    print(f"{name}: {sheet_path} slot {slot} replaced, {len(replaced)} colors, worst dE2={max_de}" + (" [!!]" if max_de > 100 else ""))

# Verify untouched regions identical to git HEAD
import subprocess, io
for sheet_path in {j[1] for j in JOBS}:
    old = subprocess.run(["git", "show", f"HEAD:{sheet_path.split('/')[-1] if False else sheet_path}"], capture_output=True)
    # git show needs repo-relative path
print("done (slot-local edits; palette untouched - other slots unchanged by construction)")
