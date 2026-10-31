"""Walks through the gallery app (Curio) in Speculos: every work's card and first page, and the
text pages of the first one.

    ./scripts/emulate.sh gallery tour      (screenshots land in screenshots/)
"""

from drive import Device

WORKS = 14

d = Device()
d.wait(0.3)
for i in range(WORKS):
    d.shot(f"gallery_{i + 1:02d}_card")
    d.select()
    d.shot(f"gallery_{i + 1:02d}_whole")
    if i == 0:
        for page in ("about", "story_1", "story_2"):
            d.click("right")
            d.shot(f"gallery_01_{page}")
    d.select()  # back to the card
    d.click("right")
print("tour done")
