"""Walks through every screen of the arcade in Speculos and saves a screenshot of each.

    ./scripts/emulate.sh tour      (screenshots land in screenshots/)
"""

from drive import Device

d = Device()

# Launcher
d.wait(0.5)
d.shot("launcher_1_dodge")
d.click("right")
d.shot("launcher_2_dice")
d.click("right")
d.shot("launcher_3_duel")
d.click("right")
d.shot("launcher_4_quit")
d.click("right")  # wraps around to Dodge

# Dodge
d.select()
d.shot("dodge_title")
d.select()
d.wait(1.0)
d.shot("dodge_play")
d.press("left")
d.wait(0.6)
d.release("left")
d.shot("dodge_moved_left")
d.select()
d.shot("dodge_paused")
d.select()
assert d.wait_for(d.has_footer, timeout=60), "standing still, a block hits the ship"
d.shot("dodge_over")
d.click("left")
d.shot("dodge_title_after")
d.hold()
d.shot("launcher_after_dodge")

# Dice
d.click("right")
d.select()
d.shot("dice_start")
d.select()
d.wait(0.25)
d.shot("dice_rolling")
d.wait(1.0)
d.shot("dice_rolled")
d.click("right", 3)
d.shot("dice_five")
d.select()
d.wait(1.2)
d.shot("dice_five_rolled")
d.click("left", 4)
d.select()
d.wait(1.2)
d.shot("dice_one_rolled")
d.hold()

# Duel
d.click("right")
d.select()
d.shot("duel_title")
d.select()
d.wait(0.5)
d.shot("duel_wait")
d.click("left")  # too early: right wins the round
d.shot("duel_early")
d.wait(3.0)
d.shot("duel_round2_wait")
# wait for the flash, then the right player presses
for _ in range(80):
    if d.lit() > 4000:
        break
    d.wait(0.05)
d.shot("duel_flash")
d.wait(0.2)
d.click("right")
d.shot("duel_round_won")
d.wait(3.0)
d.shot("duel_over")
d.hold()
d.shot("duel_title_after")
d.hold()
d.shot("launcher_end")
print("tour done")
