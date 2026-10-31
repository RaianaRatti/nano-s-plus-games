"""Drives the app running in Speculos through its REST API and saves screenshots.

Used by scripts/emulate.sh and tests/test_arcade.py:
    from drive import Device
    d = Device()                # Speculos on localhost:5000
    d.click("right"); d.select(); d.hold(); d.shot("name")
    d.has_bar()                 # a game is in play (its bar is on screen)
"""

import io
import os
import time

import requests
from PIL import Image

SHOTS = os.path.join(os.path.dirname(__file__), "..", "screenshots")


class Device:
    def __init__(self, url="http://127.0.0.1:5000"):
        self.url = url
        os.makedirs(SHOTS, exist_ok=True)

    def _button(self, which, action, delay=0.1):
        r = requests.post(f"{self.url}/button/{which}", json={"action": action, "delay": delay})
        r.raise_for_status()

    def press(self, which):
        self._button(which, "press")

    def release(self, which):
        self._button(which, "release")

    def click(self, which, times=1):
        """A LEFT or RIGHT click (which = 'left' or 'right')."""
        for _ in range(times):
            self._button(which, "press-and-release")
            time.sleep(0.15)

    def select(self):
        """Both buttons, pressed and released quickly."""
        self._button("both", "press-and-release")
        time.sleep(0.15)

    def hold(self, seconds=1.2):
        """Both buttons held: back."""
        self.press("both")
        time.sleep(seconds)
        self.release("both")
        time.sleep(0.2)

    def image(self):
        r = requests.get(f"{self.url}/screenshot")
        r.raise_for_status()
        # Speculos draws lit pixels grey (221): threshold, do not dither. The arcade draws dark ink
        # on a lit screen, so the image is inverted: white here is ink.
        return Image.open(io.BytesIO(r.content)).convert("L").point(lambda p: 0 if p > 64 else 255).convert("1")

    def pixels(self):
        """The screen, for comparing two moments."""
        return self.image().tobytes()

    def lit(self):
        """How many pixels are inked."""
        return self.image().convert("L").histogram()[255]

    def has_bar(self):
        """The lace edge under the bar (y 8) is on screen: a game in play, not a menu or a result."""
        img = self.image()
        return all(bool(img.getpixel((x, 8))) == (x % 4 == 0) for x in range(128))

    def dots(self):
        """The launcher's row of page dots (and whatever else is drawn there)."""
        return self.image().crop((0, 53, 128, 60)).tobytes()

    def wait_for(self, predicate, timeout=30.0, step=0.1):
        end = time.time() + timeout
        while time.time() < end:
            if predicate():
                return True
            time.sleep(step)
        return False

    def shot(self, name, scale=4):
        """Saves the screen as the device shows it: dark ink on a lit screen."""
        img = self.image().convert("L").point(lambda p: 255 - p)
        big = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
        path = os.path.join(SHOTS, f"{name}.png")
        big.save(path)
        return path

    def wait(self, seconds):
        time.sleep(seconds)
