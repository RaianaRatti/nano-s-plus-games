"""Writes the apps' 14 x 14 dashboard icons: icons/arcade_14px.gif (a game pad),
pet/icons/pet_14px.gif (a little cat) and gallery/icons/gallery_14px.gif (a framed picture).

Run it inside the Ledger builder image (it has Pillow):  python3 tools/make_icon.py
"""

from pathlib import Path

from PIL import Image

ARCADE = """
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

PET = """
..............
..#........#..
..##......##..
..#.#....#.#..
..#..####..#..
.#..........#.
.#..#....#..#.
.#..#....#..#.
.#..........#.
.#...#..#...#.
.#....##....#.
..##########..
..##......##..
..............
"""

GALLERY = """
..............
.############.
.#..........#.
.#.......##.#.
.#.......##.#.
.#..........#.
.#....#.....#.
.#...###....#.
.#..#####.#.#.
.#.########.#.
.#.##########.
.############.
..............
..............
"""

ROOT = Path(__file__).resolve().parent.parent


def write(art, out):
    rows = [r for r in art.strip("\n").splitlines()]
    assert len(rows) == 14 and all(len(r) == 14 for r in rows), "an icon is 14 x 14"

    img = Image.new("P", (14, 14), 0)
    img.putpalette([0, 0, 0, 255, 255, 255] + [0, 0, 0] * 254)
    for y, row in enumerate(rows):
        for x, c in enumerate(row):
            img.putpixel((x, y), 1 if c == "#" else 0)

    out.parent.mkdir(parents=True, exist_ok=True)
    img.save(out)
    print(f"wrote {out}")


write(ARCADE, ROOT / "icons" / "arcade_14px.gif")
write(PET, ROOT / "pet" / "icons" / "pet_14px.gif")
write(GALLERY, ROOT / "gallery" / "icons" / "gallery_14px.gif")
