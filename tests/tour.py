"""Walks through every screen of the arcade in Speculos and saves a screenshot of each.

    ./scripts/emulate.sh tour      (screenshots land in screenshots/)
"""

import test_arcade as t
from drive import Device

d = Device()
d.wait(0.5)

# Launcher: every card (and their page dots, to find the way back)
for i, name in enumerate(t.CARDS):
    t.LAUNCHER_DOTS.add(d.dots())
    d.shot(f"launcher_{i + 1}_{name}")
    d.click("right")


def open_card(name):
    d.click("right", t.card(name))
    d.select()


def leave(name):
    t.back(d)
    d.click("left", t.card(name))


# Dodge
open_card("dodge")
d.wait(0.2)
d.shot("dodge_ready")
d.wait(t.READY + 1.0)
d.shot("dodge_play")
d.select()
d.shot("dodge_paused")
d.select()
assert d.wait_for(lambda: not d.has_bar(), timeout=60), "standing still, a block hits the ship"
d.wait(0.3)
d.shot("dodge_over")
leave("dodge")

# Dice
open_card("dice")
d.shot("dice_start")
d.select()
d.wait(0.25)
d.shot("dice_rolling")
d.wait(1.0)
d.shot("dice_rolled")
d.click("right", 3)
d.select()
d.wait(1.2)
d.shot("dice_five_rolled")
d.click("left", 3)
leave("dice")

# Duel
open_card("duel")
d.wait(0.5)
d.shot("duel_wait")
d.click("left")  # too early: right wins the round
d.shot("duel_early")
d.wait(3.0)
d.wait_for(lambda: d.lit() > 4000, timeout=15, step=0.05)
d.shot("duel_flash")
d.wait(0.2)
d.click("right")
d.shot("duel_round_won")
d.wait(3.0)
d.shot("duel_over")
leave("duel")

# Snake
open_card("snake")
d.wait(t.READY + 0.6)
d.click("right")
d.wait(0.6)
d.shot("snake_play")
leave("snake")

# Pong
open_card("pong")
d.shot("pong_title")
d.select()
d.wait(2.5)
d.shot("pong_play")
d.hold()
d.click("right")
d.shot("pong_title_2p")
leave("pong")

# Bricks
open_card("bricks")
d.wait(t.READY + 0.4)
d.shot("bricks_serve")
d.wait(1.5)
d.shot("bricks_play")
leave("bricks")

# Racer
open_card("racer")
d.wait(t.READY + 2.5)
d.shot("racer_play")
leave("racer")

# Cave
open_card("cave")
d.wait(t.READY)
for _ in range(5):
    d.press("left")
    d.wait(0.25)
    d.release("left")
    d.wait(0.2)
d.shot("cave_play")
assert d.wait_for(lambda: not d.has_bar(), timeout=30)
d.wait(0.3)
d.shot("cave_over")
leave("cave")

# Flip
open_card("flip")
d.wait(t.READY + 0.8)
d.shot("flip_play")
d.click("left")
d.wait(0.8)
d.shot("flip_ceiling")
leave("flip")

# Runner
open_card("runner")
d.wait(t.READY + 1.0)
d.shot("runner_play")
d.press("right")
d.wait(0.4)
d.shot("runner_jump")
d.release("right")
d.wait(1.2)
d.press("left")
d.wait(0.3)
d.shot("runner_duck")
d.release("left")
leave("runner")
print("tour done")
