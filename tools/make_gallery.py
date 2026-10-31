"""Builds the gallery's pictures and texts: gallery/works.json -> gallery/src/works.c.

Run it inside the Ledger builder image (it has Pillow):

    python3 tools/make_gallery.py               fetch what is missing, write works.c and CREDITS.md
    python3 tools/make_gallery.py preview.png   the same, and a preview of every view

The pictures come from Wikimedia Commons (all public domain) and are kept in gallery/sources/, so
they are fetched once. Each view is cropped, turned to grey, stretched in contrast, resized and
rendered in two tones: Atkinson dithering for tones, a plain threshold for shapes (paper cuts,
woodcuts). The first view is the whole work: on its card, under the bar, and as the first page of
the work, as big as the screen allows. The other views are details, fitted to the screen in their
own proportions (or filling it, with "fill"). The texts are wrapped here, to the 20 characters a line holds on the screen.
"""

import json
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

from PIL import Image, ImageFilter, ImageOps

ROOT = Path(__file__).resolve().parent.parent
GALLERY = ROOT / "gallery"
SOURCES = GALLERY / "sources"
UA = {"User-Agent": "badge-arcade-gallery/1.0 (https://github.com/RaianaRatti)"}

SCREEN = (128, 64)
CARD = (128, 54)  # the whole work, under the bar
LINE = 20         # characters per line: 120 pixels, the scroll bar beside
PAGE_LINES = 7

# The letters the screen's font has beyond ASCII (see src/gfx.c).
GLYPHS = {"é": "\x06", "á": "\x07"}


# ---------------------------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------------------------

def fetch(key, title):
    """The picture (about 1200 pixels wide) and its credit, from Commons, once."""
    image, credit = SOURCES / f"{key}.jpg", SOURCES / f"{key}.json"
    if image.exists() and credit.exists():
        return Image.open(image), json.loads(credit.read_text())
    query = urllib.parse.urlencode({
        "action": "query", "titles": title, "prop": "imageinfo", "format": "json",
        "iiprop": "url|extmetadata", "iiurlwidth": 1200})
    request = urllib.request.Request(f"https://commons.wikimedia.org/w/api.php?{query}", headers=UA)
    page = next(iter(json.load(urllib.request.urlopen(request))["query"]["pages"].values()))
    info = page["imageinfo"][0]
    url = info.get("thumburl") or info["url"]
    image.write_bytes(urllib.request.urlopen(urllib.request.Request(url, headers=UA)).read())
    licence = info.get("extmetadata", {}).get("LicenseShortName", {}).get("value", "?")
    credit.write_text(json.dumps({"title": title, "url": info["descriptionurl"], "license": licence}))
    time.sleep(0.5)
    return Image.open(image), json.loads(credit.read_text())


# ---------------------------------------------------------------------------------------------
# Pictures
# ---------------------------------------------------------------------------------------------

def crop(im, box, aspect=None):
    """The box (fractions), grown to the aspect (width / height) around its centre if given."""
    w, h = im.size
    x0, y0, x1, y1 = box[0] * w, box[1] * h, box[2] * w, box[3] * h
    if aspect:
        bw, bh = x1 - x0, y1 - y0
        if bw / bh < aspect:
            bw = bh * aspect
        else:
            bh = bw / aspect
        bw, bh = min(bw, w), min(bh, h)
        cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
        x0 = min(max(cx - bw / 2, 0), w - bw)
        y0 = min(max(cy - bh / 2, 0), h - bh)
        x1, y1 = x0 + bw, y0 + bh
    return im.crop((round(x0), round(y0), round(x1), round(y1)))


def fit(size, area):
    s = min(area[0] / size[0], area[1] / size[1])
    return max(1, round(size[0] * s)), max(1, round(size[1] * s))


def render(im, view, area, fill):
    """A view as rows of booleans (True: ink), filling `area` or fitted into it."""
    part = crop(im, view["box"], aspect=area[0] / area[1] if fill else None)
    size = area if fill else fit(part.size, area)

    grey = ImageOps.autocontrast(part.convert("L"), cutoff=view.get("cutoff", 1))
    gamma = view.get("gamma", 1.0)
    if gamma != 1.0:
        grey = grey.point(lambda p: round(255 * (p / 255) ** gamma))
    if view.get("blur"):  # fine hatching becomes tone, measured in screen pixels
        big = (size[0] * 4, size[1] * 4)
        grey = grey.resize(big, Image.LANCZOS).filter(ImageFilter.GaussianBlur(view["blur"] * 4))
    grey = grey.resize(size, Image.LANCZOS)

    w, h = size
    px = [[grey.getpixel((x, y)) / 255 for x in range(w)] for y in range(h)]
    ink = [[False] * w for _ in range(h)]
    dither = view.get("mode", "dither") == "dither"
    threshold = view.get("threshold", 0.5)
    for y in range(h):
        for x in range(w):
            old = px[y][x]
            new = 1.0 if old >= threshold else 0.0
            ink[y][x] = new == 0.0
            if dither:  # Atkinson: 6/8 of the error, to six neighbours
                e = (old - new) / 8
                for dx, dy in ((1, 0), (2, 0), (-1, 1), (0, 1), (1, 1), (0, 2)):
                    if 0 <= x + dx < w and y + dy < h:
                        px[y + dy][x + dx] += e
    return ink


def pack(ink):
    """Rows of ceil(w / 8) bytes, the leftmost pixel in the high bit, 1 for ink."""
    out = bytearray()
    for row in ink:
        for x in range(0, len(row), 8):
            byte = 0
            for i, on in enumerate(row[x:x + 8]):
                if on:
                    byte |= 0x80 >> i
            out.append(byte)
    return out


# ---------------------------------------------------------------------------------------------
# Texts
# ---------------------------------------------------------------------------------------------

def screen_text(text):
    out = "".join(GLYPHS.get(c, c) for c in text)
    bad = {c for c in out if not ("\x01" <= c <= "\x07" or " " <= c <= "_" or "a" <= c <= "z")}
    assert not bad, f"the font has no {bad} in {text!r}"
    return out


def wrap(text, width=LINE):
    lines, line = [], ""
    for word in screen_text(text).split():
        assert len(word) <= width, f"{word!r} is longer than a line"
        if line and len(line) + 1 + len(word) > width:
            lines.append(line)
            line = word
        else:
            line = f"{line} {word}" if line else word
    if line:
        lines.append(line)
    return lines


def c_string(s):
    out = ""
    for c in s:
        if c in '"\\':
            out += "\\" + c
        elif c == "\n":
            out += "\\n"
        elif ord(c) < 32:
            out += f"\\x{ord(c):02x}\"\""  # end the escape before the next character
        else:
            out += c
    return f'"{out}"'


# ---------------------------------------------------------------------------------------------
# works.c
# ---------------------------------------------------------------------------------------------

def main():
    data = json.loads((GALLERY / "works.json").read_text())["works"]
    SOURCES.mkdir(exist_ok=True)
    preview = sys.argv[1] if len(sys.argv) > 1 else None

    bits = bytearray()
    text = ""
    pictures, works, credits, previews = [], [], [], []
    for work in data:
        im, credit = fetch(work["key"], work["file"])
        credits.append((work, credit))
        first = len(pictures)
        # The card, then the pages: the whole work on the whole screen, then the details.
        renders = [("card", render(im, work["views"][0], CARD, False), True),
                   ("full", render(im, work["views"][0], SCREEN, False), False)]
        for i, view in enumerate(work["views"][1:]):
            renders.append((f"detail {i + 1}", render(im, view, SCREEN, view.get("fill", False)), False))
        for name, ink, card in renders:
            pictures.append((len(bits), len(ink[0]), len(ink), card))
            bits += pack(ink)
            previews.append((f"{work['key']} {name}", ink, card))

        title = wrap(work["title"])
        about = [line for part in work["about"] for line in wrap(part)]
        assert len(title) <= 4 and len(title) + len(about) <= 7, f"{work['key']}: the about page is too long"
        story = wrap(work["story"])
        short = screen_text(work["short"])
        assert len(short) <= 15, f"{work['key']}: the short title is too long"

        about_at = len(text)
        text += "\n".join(title + about) + "\0"
        story_at = len(text)
        text += "\n".join(story) + "\0"
        works.append((short, about_at, len(title), len(about), story_at, len(story), first, len(renders) - 1))
        print(f"{work['key']:11s} {len(renders) - 2} details, story {len(story)} lines "
              f"({-(-len(story) // PAGE_LINES)} pages)")

    assert len(bits) < 1 << 32 and len(text) < 1 << 16

    out = [
        "// Generated by tools/make_gallery.py from gallery/works.json: do not edit.",
        "",
        '#include "works.h"',
        "",
        f"const uint8_t NB_WORKS = {len(works)};",
        "",
        "const work_t WORKS[] = {",
    ]
    for short, about_at, nt, na, story_at, ns, first, nd in works:
        out.append(f"    {{{c_string(short)}, {about_at}, {nt}, {na}, {story_at}, {ns}, {first}, {nd}}},")
    out += ["};", "", "const picture_t PICTURES[] = {"]
    for offset, w, h, whole in pictures:
        out.append(f"    {{{offset}, {w}, {h}}},")
    out += ["};", "", "const uint8_t ART_BITS[] = {"]
    for i in range(0, len(bits), 16):
        out.append("    " + " ".join(f"0x{b:02x}," for b in bits[i:i + 16]))
    out += ["};", "", "const char ART_TEXT[] ="]
    for chunk in text.split("\0")[:-1]:
        out.append(f"    {c_string(chunk + chr(0))}")
    out[-1] += ";"
    (GALLERY / "src" / "works.c").write_text("\n".join(out) + "\n")

    lines = ["# Credits", "", "Every picture in the gallery comes from Wikimedia Commons, where it is marked as",
             "public domain or CC0. The 1-bit versions are made by `tools/make_gallery.py`.", ""]
    for work, credit in credits:
        lines.append(f"- **{work['title']}**, {' '.join(work['about'][:2])}: "
                     f"[{credit['title']}]({credit['url']}) ({credit['license']})")
    (GALLERY / "CREDITS.md").write_text("\n".join(lines) + "\n")
    print(f"{len(works)} works, {len(pictures)} pictures, {len(bits)} bytes of pictures, {len(text)} of text")

    if preview:
        S, cols = 3, 3
        rows = -(-len(previews) // cols)
        sheet = Image.new("L", (cols * (SCREEN[0] * S + 8), rows * (SCREEN[1] * S + 8)), 120)
        for i, (name, ink, whole) in enumerate(previews):
            frame = Image.new("L", SCREEN, 235)
            ox = (SCREEN[0] - len(ink[0])) // 2
            oy = 10 + (CARD[1] - len(ink)) // 2 if whole else (SCREEN[1] - len(ink)) // 2
            for y, row in enumerate(ink):
                for x, on in enumerate(row):
                    if on:
                        frame.putpixel((ox + x, oy + y), 25)
            frame = frame.resize((SCREEN[0] * S, SCREEN[1] * S), Image.NEAREST)
            sheet.paste(frame, ((i % cols) * (SCREEN[0] * S + 8), (i // cols) * (SCREEN[1] * S + 8)))
        sheet.save(preview)


if __name__ == "__main__":
    main()
