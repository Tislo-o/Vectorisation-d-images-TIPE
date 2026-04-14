import matplotlib.pyplot as plt
import numpy as np
import re
import sys

data = sys.stdin.read()

def parse_input(text):
    pixels = []

    current_color = (255, 255, 255)

    for line in text.splitlines():
        line = line.strip()

        if line.startswith("Color"):
            match = re.match(r"Color\s+(\d+)\s+(\d+)\s+(\d+):", line)
            if match:
                current_color = (
                    int(match.group(1)),
                    int(match.group(2)),
                    int(match.group(3))
                )

        elif line.startswith("x:"):
            match = re.match(r"x:(-?\d+)\s+y:(-?\d+)", line)
            if match:
                x, y = int(match.group(1)), int(match.group(2))
                pixels.append((x, y, current_color))

    return pixels


def display_pixels(pixels):
    xs = [p[0] for p in pixels]
    ys = [p[1] for p in pixels]

    min_x, max_x = min(xs), max(xs)
    min_y, max_y = min(ys), max(ys)

    width = max_x - min_x + 1
    height = max_y - min_y + 1

    # Image buffer
    img = np.full((height, width, 3), (128, 128, 128), dtype=np.uint8)
    # Fill pixels
    for x, y, color in pixels:
        px = x - min_x
        py = y - min_y
        img[py, px] = color 

    plt.imshow(img, interpolation='nearest')
    plt.title("RGB Pixel Image")
    plt.axis('off')
    plt.show()

pixels = parse_input(data)
display_pixels(pixels)
