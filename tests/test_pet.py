"""Functional tests of the pet app: the app in Speculos, driven through its buttons.

    ./scripts/emulate.sh pet test

The tests run in order on one running app, which opens on a new egg. Each ends in the pet's room
with the shelf on its first action (meal). After every action it takes, the pet says a physics fact
in a box (no bar on screen), which LEFT or RIGHT closes. The last one quits the app, which stops the emulator: it
is checked from outside (scripts/emulate.sh), since this script stops with it.
"""

import sys
import traceback

from drive import Device
from test_arcade import moving, stable

INFO = 6  # the shelf: meal, treat, play, clean, sleep, meds, info


def bar_inked(d):
    """Something is written in the bar (y 0 to 7): the pet's level and meters, not the egg's."""
    return any(d.image().crop((0, 0, 128, 8)).tobytes())


def shelf(d):
    """The shelf of actions (y 53 to 63), or the egg's hearts."""
    return d.image().crop((0, 53, 128, 64)).tobytes()


def shelf_inked(d):
    return any(d.image().crop((0, 53, 128, 64)).tobytes())


def reading(d):
    """A fact's pages: the dotted scroll bar along the right edge, under the bar."""
    img = d.image()
    return d.has_bar() and all(img.getpixel((127, y)) for y in range(10, 64, 2))


def close_fact(d):
    assert d.wait_for(lambda: not d.has_bar(), timeout=3.0), "the pet says a fact, in a box"
    d.click("left")
    assert d.has_bar(), "LEFT closes it: the room"


def test_the_egg_hatches(d):
    assert d.has_bar(), "the pet's room"
    assert not bar_inked(d), "a new pet is an egg: no level, no meters"
    for _ in range(25):  # warm it up until it hatches
        d.click("right")
        if bar_inked(d):
            break
    assert bar_inked(d), "it hatched: the level and the meters"
    d.wait(2.0)  # the hatching sparkles


def test_it_eats(d):
    d.select()  # meal
    assert moving(d), "it eats"
    assert d.has_bar(), "no fact while it eats"


def test_a_fact_after_eating(d):
    assert d.wait_for(lambda: not d.has_bar(), timeout=3.0), "after eating, a fact in a box"
    d.select()  # open it
    assert reading(d), "BOTH opens the fact on pages"
    first = d.pixels()
    assert stable(d), "the pages stay still (the pet waits while you read)"
    d.click("right")
    assert reading(d) and d.pixels() != first, "RIGHT turns the page"
    d.click("left")
    assert d.pixels() == first, "LEFT turns it back"
    d.click("left")
    assert not d.has_bar(), "LEFT from the first page: the box again"
    d.select()
    for _ in range(12):  # to the last page, and past it
        d.click("right")
        if not reading(d):
            break
    assert d.has_bar() and not reading(d) and shelf_inked(d), "past the last page: the room"


def test_info_and_the_collection(d):
    room = shelf(d)
    d.click("left")  # the shelf turns round to info
    assert shelf(d) != room, "LEFT moves along the shelf"
    d.select()
    assert stable(d), "the info page stays"
    page = d.pixels()
    d.click("right")
    assert d.pixels() != page, "RIGHT picks another form in the collection"
    d.select()  # back to the room
    assert shelf_inked(d)
    d.click("right")  # info -> meal


def test_catch(d):
    d.click("right", 2)  # play
    d.select()
    d.wait(0.3)
    assert not shelf_inked(d), "catch: no shelf"
    assert moving(d, 3.0), "a ball comes"
    d.hold()  # stop playing
    d.wait(0.3)
    assert shelf_inked(d), "back in the room"
    d.click("left", 2)


def test_sleep(d):
    d.click("left", 3)  # sleep
    lit = d.lit()
    d.select()
    close_fact(d)
    assert d.lit() > lit + 2000, "lights off: the room goes dark"
    d.select()  # lights on
    close_fact(d)
    assert d.lit() < lit + 2000
    d.click("right", 3)


def hold_quits(d):
    """HOLD in the room saves and quits: the app ends, and the emulator with it."""
    d.press("both")


if __name__ == "__main__":
    device = Device()
    device.wait(0.5)
    tests = [(name, fn) for name, fn in list(globals().items()) if name.startswith("test_")]
    failed = 0
    for name, fn in tests:
        try:
            fn(device)
            print(f"PASS {name}", flush=True)
        except Exception:
            failed += 1
            print(f"FAIL {name} (the screen: screenshots/fail_{name}.png)")
            try:
                device.shot(f"fail_{name}")
            except Exception:
                pass
            traceback.print_exc()
            break  # the next tests start from a screen this one did not reach
    print("stopped at the first failure" if failed else f"all {len(tests)} passed", flush=True)
    if failed:
        sys.exit(1)
    print("HOLD to quit ...", flush=True)
    hold_quits(device)
    device.wait(3.0)  # the emulator stops with the app, and this script with it
    print("the app did not quit", flush=True)
    sys.exit(1)
