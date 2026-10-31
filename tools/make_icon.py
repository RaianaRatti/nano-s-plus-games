"""Writes icons/arcade_14px.gif, the app's 14 x 14 dashboard icon: a game pad.

Run it inside the Ledger builder image (it has Pillow):  python3 tools/make_icon.py
"""

from pathlib import Path

from PIL import Image

ART = """
..............
..............
..............
...########...
..##########..
.###.####.###.
.##...##.#.##.
.###.####.###.
.############.
.####....####.
..##......##..
..............
..............
..............
"""

rows = [r for r in ART.strip("\n").splitlines()]
assert len(rows) == 14 and all(len(r) == 14 for r in rows), "the icon is 14 x 14"

img = Image.new("P", (14, 14), 0)
img.putpalette([0, 0, 0, 255, 255, 255] + [0, 0, 0] * 254)
for y, row in enumerate(rows):
    for x, c in enumerate(row):
        img.putpixel((x, y), 1 if c == "#" else 0)

out = Path(__file__).resolve().parent.parent / "icons" / "arcade_14px.gif"
out.parent.mkdir(exist_ok=True)
img.save(out)
print(f"wrote {out}")
