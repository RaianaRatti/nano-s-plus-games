"""Drives the app running in Speculos through its REST API and saves screenshots.

Used by scripts/emulate.sh and tests/test_arcade.py:
    from drive import Device
    d = Device()                # Speculos on localhost:5000
    d.click("right"); d.select(); d.hold(); d.shot("name")
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
        # Speculos draws lit pixels grey (221): threshold, do not dither.
        return Image.open(io.BytesIO(r.content)).convert("L").point(lambda p: 255 if p > 64 else 0).convert("1")

    def pixels(self):
        """The screen, for comparing two moments."""
        return self.image().tobytes()

    def lit(self):
        """How many pixels are lit."""
        return self.image().convert("L").histogram()[255]

    def has_footer(self):
        """The footer's perforated rule (y 54) is on screen: a menu, a title or a result, not play."""
        img = self.image()
        return all(img.getpixel((x, 54)) for x in range(0, 128, 2)) and not any(
            img.getpixel((x, 54)) for x in range(1, 128, 2)
        )

    def wait_for(self, predicate, timeout=30.0, step=0.1):
        end = time.time() + timeout
        while time.time() < end:
            if predicate():
                return True
            time.sleep(step)
        return False

    def shot(self, name, scale=4):
        img = self.image()
        big = img.resize((img.width * scale, img.height * scale), Image.NEAREST)
        path = os.path.join(SHOTS, f"{name}.png")
        big.save(path)
        return path

    def wait(self, seconds):
        time.sleep(seconds)
