"""Functional tests: the app in Speculos, driven through its buttons.

    ./scripts/emulate.sh test

The tests run in order on one running app. Each starts on the launcher's first card (Dodge) and
goes back there at the end.
"""

import sys
import traceback

from drive import Device


def stable(d, seconds=0.6):
    """The screen does not change for a while."""
    before = d.pixels()
    d.wait(seconds)
    return before == d.pixels()


# The launcher's cards, in order. The last one is Quit.
CARDS = ["dodge", "dice", "duel", "snake", "pong", "bricks", "racer", "cave", "flip", "runner", "quit"]


def card(name):
    return CARDS.index(name)


def moving(d, seconds=0.5):
    """The screen changes at least once in a while (sampled every 0.1 s)."""
    first = d.pixels()
    end = seconds / 0.1
    for _ in range(int(end)):
        d.wait(0.1)
        if d.pixels() != first:
            return True
    return False


def test_launcher_turns_through_every_card(d):
    cards = []
    for _ in CARDS:
        cards.append(d.pixels())
        d.click("right")
    assert len(set(cards)) == len(CARDS)
    assert d.pixels() == cards[0], "one more click is the first card again"
    d.click("left")
    assert d.pixels() == cards[-1], "LEFT turns back"
    d.click("right")


def test_dodge_run_ends_and_the_best_score_is_kept(d):
    launcher_before = d.pixels()
    d.select()
    assert d.has_footer(), "the title"
    d.select()
    d.wait(0.3)
    assert not d.has_footer(), "playing: the field fills the screen"
    assert d.wait_for(d.has_footer, timeout=60), "standing still, a block hits the ship"
    d.click("left")  # game over -> title
    d.hold()  # title -> launcher
    assert d.has_footer()
    assert d.pixels() != launcher_before, "the launcher shows the new best"


def test_dodge_pauses_and_resumes(d):
    d.select()
    d.select()
    d.wait(1.0)
    d.select()  # pause
    assert stable(d), "nothing moves while paused"
    d.select()  # resume
    assert not stable(d, 0.5), "the blocks fall again"
    d.hold()  # end the run
    d.wait(0.3)
    assert d.has_footer(), "back on the title (or a game over, if a block was quicker)"
    back(d)


def test_dice_roll_settles_on_a_result(d):
    d.click("right")
    d.select()
    placeholder = d.pixels()
    d.select()
    d.wait(1.0)
    assert stable(d), "the dice have settled"
    assert d.pixels() != placeholder
    d.click("right")
    assert d.pixels() != placeholder, "three dice now"
    back(d)
    d.click("left")


def test_duel_false_start_and_flash(d):
    d.click("right", 2)
    d.select()  # duel title
    d.select()  # round 1: wait
    d.wait(0.3)
    waiting = d.pixels()
    d.click("left")  # too early: the round goes to the right player
    assert d.wait_for(lambda: d.pixels() != waiting, timeout=1.0)

    # Round 2: wait for the flash (the screen lights up), then the right player presses.
    assert d.wait_for(lambda: d.lit() > 4000, timeout=15, step=0.05), "the flash"
    d.click("right")
    assert d.lit() < 4000, "the round's result"

    # Right won two rounds: after the result, the duel is over and the screen stays.
    d.wait(3.0)
    assert stable(d, 1.0)
    d.hold()  # over -> title
    d.hold()  # title -> launcher
    d.click("left", 2)


LAUNCHER_FOOTER = None  # "BOTH:OPEN  HOLD:QUIT", read when the tests start


def footer(d):
    return d.image().crop((0, 55, 128, 64)).tobytes()


def back(d):
    """HOLD until the launcher is on screen, whatever the game was doing (a crash ignores it)."""
    for _ in range(6):
        if footer(d) == LAUNCHER_FOOTER:
            return
        d.hold()
        d.wait(1.0)
    raise AssertionError("could not get back to the launcher")


def play_pause_and_leave(d, name, pause_after=0.5):
    """Open a score game, play, pause, resume, end the run and go back to the launcher."""
    d.click("right", card(name))
    d.select()
    assert d.has_footer(), f"{name}: the title"
    d.select()
    d.wait(pause_after)
    assert not d.has_footer(), f"{name}: playing, the field fills the screen"
    assert moving(d), f"{name}: the field moves"
    d.select()  # pause
    assert stable(d), f"{name}: nothing moves while paused"
    d.select()  # resume
    assert moving(d), f"{name}: it moves again"
    back(d)
    d.click("left", card(name))


def ends_alone(d, name, first_press=False):
    """Left alone, the run ends: the game over screen, then the title."""
    d.click("right", card(name))
    d.select()
    d.select()
    d.wait(0.3)
    if first_press:
        d.click("left")  # the game waits for it
    assert not d.has_footer()
    assert d.wait_for(d.has_footer, timeout=90), f"{name}: nobody plays, the run ends"
    back(d)
    d.click("left", card(name))


def test_snake(d):
    play_pause_and_leave(d, "snake")
    ends_alone(d, "snake")


def test_bricks(d):
    play_pause_and_leave(d, "bricks", pause_after=1.5)


def test_racer(d):
    play_pause_and_leave(d, "racer")


def test_cave(d):
    play_pause_and_leave(d, "cave")
    ends_alone(d, "cave", first_press=True)


def test_flip(d):
    play_pause_and_leave(d, "flip")
    ends_alone(d, "flip")


def test_runner(d):
    play_pause_and_leave(d, "runner")
    ends_alone(d, "runner")


def test_pong_both_modes(d):
    d.click("right", card("pong"))
    d.select()
    one = d.pixels()
    d.click("right")
    assert d.pixels() != one, "the title switches to two players"
    d.click("left")
    assert d.pixels() == one, "and back"

    d.select()  # one player
    d.wait(1.5)
    assert not d.has_footer(), "the court"
    d.select()  # pause
    assert stable(d), "paused"
    d.hold()  # quit to the title
    assert d.pixels() == one

    d.click("right")
    d.select()  # two players: each press turns a paddle around
    d.wait(1.5)
    assert moving(d), "the paddles slide"
    d.hold()
    d.click("left")
    d.hold()  # title -> launcher
    d.click("left", card("pong"))


if __name__ == "__main__":
    device = Device()
    device.wait(0.5)
    LAUNCHER_FOOTER = footer(device)
    tests = [(name, fn) for name, fn in list(globals().items()) if name.startswith("test_")]
    failed = 0
    for name, fn in tests:
        try:
            fn(device)
            print(f"PASS {name}")
        except Exception:
            failed += 1
            print(f"FAIL {name} (the screen: screenshots/fail_{name}.png)")
            device.shot(f"fail_{name}")
            traceback.print_exc()
            break  # the next tests start from a screen this one did not reach
    print("stopped at the first failure" if failed else f"all {len(tests)} passed")
    sys.exit(1 if failed else 0)
