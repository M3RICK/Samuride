#!/usr/bin/env python3
"""Draws the Arasaka security drone of the CYBERPSYCHO mode, pixel by pixel.

Writes assets/drone/drone.png: 4 frames of 32x24 side by side. Rotors spin,
the red eye sweeps left and right, the thruster flickers. The game scales it
up without smoothing.

Usage: python3 tools/draw_drone.py
"""

from pathlib import Path

from PIL import Image

OUT = Path(__file__).resolve().parent.parent / "assets" / "drone" / "drone.png"
WIDTH, HEIGHT, FRAMES = 32, 24, 4

CLEAR = (0, 0, 0, 0)
OUTLINE = (14, 12, 22, 255)
HULL = (48, 44, 66, 255)
HULL_LIGHT = (92, 88, 120, 255)
ARASAKA_RED = (220, 20, 50, 255)
EYE = (255, 40, 70, 255)
EYE_DIM = (120, 0, 30, 255)
ROTOR = (150, 150, 175, 255)
ROTOR_BLUR = (0, 150, 180, 255)
THRUST = (0, 240, 255, 255)
WHITE = (255, 255, 255, 255)


def rect(pixels, x0, y0, x1, y1, color):
    for x in range(x0, x1 + 1):
        for y in range(y0, y1 + 1):
            pixels[x, y] = color


def draw_frame(index):
    image = Image.new("RGBA", (WIDTH, HEIGHT), CLEAR)
    px = image.load()

    # Arms and spinning rotors: the blades shrink and blur as they turn
    rect(px, 3, 8, 28, 9, OUTLINE)
    rect(px, 4, 8, 27, 8, HULL)
    blade = [5, 3, 1, 3][index]
    for hub in (5, 26):
        rect(px, hub - blade, 5, hub + blade, 5, ROTOR if index % 2 == 0 else ROTOR_BLUR)
        rect(px, hub, 6, hub, 7, OUTLINE)

    # Hull, with the Arasaka red stripe
    rect(px, 9, 9, 22, 18, OUTLINE)
    rect(px, 10, 10, 21, 17, HULL)
    rect(px, 10, 10, 21, 11, HULL_LIGHT)
    rect(px, 10, 15, 21, 15, ARASAKA_RED)

    # Eye sweeping left and right
    eye_x = 13 + [0, 1, 2, 1][index]
    rect(px, 12, 12, 19, 13, OUTLINE)
    rect(px, eye_x, 12, eye_x + 1, 13, EYE if index != 2 else EYE_DIM)
    px[eye_x, 12] = WHITE

    # Thruster
    rect(px, 14, 19, 17, 19, OUTLINE)
    rect(px, 15, 20, 16, 20 + index % 2, THRUST)
    px[15, 20] = WHITE
    return image


if __name__ == "__main__":
    sheet = Image.new("RGBA", (WIDTH * FRAMES, HEIGHT), CLEAR)
    for i in range(FRAMES):
        sheet.paste(draw_frame(i), (i * WIDTH, 0))
    OUT.parent.mkdir(exist_ok=True)
    sheet.save(OUT)
    print(f"Drew {OUT.relative_to(OUT.parents[2])}")
