"""Walks through the screens of the pet app in Speculos and saves a screenshot of each.

    ./scripts/emulate.sh pet tour      (screenshots land in screenshots/)
"""

from drive import Device
from test_pet import bar_inked

d = Device()
d.wait(0.3)
d.shot("pet_egg")
d.click("right", 12)
d.shot("pet_egg_warm")
while not bar_inked(d):
    d.click("right")
d.wait(2.5)
d.shot("pet_baby")
d.select()  # meal
d.wait(0.6)
d.shot("pet_eat")
d.wait(2.0)
d.shot("pet_fact")  # once it has eaten, it says a fact
d.select()  # open it
d.wait(0.3)
d.shot("pet_fact_more")
d.click("right")
d.shot("pet_fact_page2")
d.select()  # back to the room
d.click("right", 2)
d.select()  # play
d.wait(2.2)
d.shot("pet_catch")
d.hold()
d.click("right", 2)
d.select()  # sleep
d.wait(0.5)
d.click("left")  # close the fact
d.shot("pet_sleep")
d.select()  # lights on
d.wait(0.3)
d.click("left")  # close the fact
d.click("right", 2)
d.select()  # info
d.wait(0.3)
d.shot("pet_info")
d.select()
print("tour done")
