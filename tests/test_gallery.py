"""Functional tests of the gallery app (Curio): the app in Speculos, driven through its buttons.

    ./scripts/emulate.sh gallery test

The tests run in order on one running app, which opens on the first work's card. The last one
quits the app, which stops the emulator: that is checked from outside (scripts/emulate.sh).
"""

import sys
import traceback

from drive import Device
from test_arcade import stable

FIRST_WORK_PAGES = 5  # the dancers: the whole work, about, three pages of story


def test_cards_turn(d):
    assert d.has_bar(), "a card: the number and the title in the bar"
    first = d.pixels()
    d.click("right")
    second = d.pixels()
    assert second != first, "RIGHT shows the next work"
    d.click("left")
    assert d.pixels() == first, "LEFT goes back"
    d.click("left")
    assert d.has_bar() and d.pixels() not in (first, second), "LEFT from the first is the last"
    d.click("right")


def test_a_work_opens_on_the_whole_work(d):
    card = d.pixels()
    d.select()
    assert not d.has_bar(), "the work on the whole screen, no bar"
    assert d.pixels() != card
    assert stable(d), "a still picture"


def test_pages_turn_and_come_back_to_the_card(d):
    pages = [d.pixels()]
    for _ in range(FIRST_WORK_PAGES - 1):
        d.click("right")
        pages.append(d.pixels())
    assert len(set(pages)) == FIRST_WORK_PAGES, "every page is different"
    assert d.image().getpixel((127, 0)) or d.image().getpixel((127, 63)), "the scroll bar"
    d.click("right")
    assert d.has_bar(), "past the last page: the card again"


def test_both_goes_back_to_the_card(d):
    d.select()
    d.click("right")
    d.select()
    assert d.has_bar(), "BOTH closes the work"


def hold_quits(d):
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
            print(f"FAIL {name} (the screen: screenshots/fail_{name}.png)", flush=True)
            try:
                device.shot(f"fail_{name}")
            except Exception:
                pass
            traceback.print_exc()
            break
    print("stopped at the first failure" if failed else f"all {len(tests)} passed", flush=True)
    if failed:
        sys.exit(1)
    print("HOLD to quit ...", flush=True)
    hold_quits(device)
    device.wait(3.0)  # the emulator stops with the app, and this script with it
    print("the app did not quit", flush=True)
    sys.exit(1)
