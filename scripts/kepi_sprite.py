#!/usr/bin/env python3
#
# Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
# claim that you wrote the original software. If you use this software
# in a product, an acknowledgment in the product documentation would be
# appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
# misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#
"""
Draws resources/sprites/kepi_idle.png: Kepi, Scarab's mascot (a scarab carrying a small sun, the same
character as the logo docs/images/kepi.svg), as a 128x32 strip of four 32x32 pixel-art frames in the
logo's palette - the same shape as sunny_idle_down.png, for sprite_configure_texture with 4 frames.

The idle loop: the sun lifts a pixel on frames 1-2, its rays are longer on frames 1 and 3, and the legs
shift one way on frame 1 and the other way on frame 3. Temporary art, like the logo.

Needs only the standard library (it writes the PNG itself). The output is deterministic: running it again
writes the same bytes.

Usage: kepi_sprite.py [output.png [preview.png]]
  output.png   default: resources/sprites/kepi_idle.png (from the repo root)
  preview.png  optional: the same strip scaled 8x, to look at
"""
import math
import os
import struct
import sys
import zlib

W, H, FRAMES = 32, 32, 4
T = None
OUTLINE = (5, 32, 42)
LEG = (10, 61, 78)
DEEP, DARK, MID, LIGHT, GLOSS = (8, 51, 71), (13, 95, 110), (31, 165, 142), (127, 240, 208), (220, 255, 245)
HEAD_D, HEAD_M, HEAD_L = (10, 61, 78), (18, 122, 120), (95, 214, 192)
SUN_E, SUN_M, SUN_L, RAY = (232, 137, 12), (255, 210, 58), (255, 251, 224), (242, 165, 28)

def frame(n):
    px = [[T] * W for _ in range(H)]
    def put(x, y, c):
        if 0 <= x < W and 0 <= y < H: px[y][x] = c
    def line(x0, y0, x1, y1, c):
        dx, dy = abs(x1 - x0), -abs(y1 - y0); sx = 1 if x0 < x1 else -1; sy = 1 if y0 < y1 else -1; e = dx + dy
        while True:
            put(x0, y0, c)
            if x0 == x1 and y0 == y1: break
            e2 = 2 * e
            if e2 >= dy: e += dy; x0 += sx
            if e2 <= dx: e += dx; y0 += sy
    bob = -1 if n in (1, 2) else 0          # the sun lifts a pixel on frames 1-2
    sway = 1 if n == 1 else (-1 if n == 3 else 0)   # legs shift: frame 1 one way, frame 3 the other
    # legs (under the body): middle and back pairs, then the front pair up to the sun
    for side in (-1, 1):
        line(16 + side * 6, 17, 16 + side * 11, 16 + (sway * side), LEG)
        line(16 + side * 11, 16 + (sway * side), 16 + side * 13, 19 + (sway * side), LEG)
        line(16 + side * 6, 25, 16 + side * 10, 27 - (sway * side), LEG)
        line(16 + side * 10, 27 - (sway * side), 16 + side * 11, 31 - (sway * side), LEG)
        line(16 + side * 3, 11, 16 + side * 7, 9, LEG)
        line(16 + side * 7, 9, 16 + side * 4, 5 + bob, LEG)
    # wing cases: an ellipse, shaded from a highlight at the upper left
    for y in range(14, 31):
        for x in range(7, 26):
            if ((x - 16) / 8.6) ** 2 + ((y - 22.5) / 8.2) ** 2 <= 1:
                d = math.hypot(x - 12, y - 19)
                shine = ( ( x - 11.5 ) / 1.3 ) ** 2 + ( ( y - 20 ) / 3.2 ) ** 2 <= 1
                put(x, y, LIGHT if shine else MID if d < 5.5 else DARK if d < 9 else DEEP)
    # pronotum
    for y in range(12, 18):
        for x in range(7, 26):
            if ((x - 16) / 8.4) ** 2 + ((y - 15) / 2.9) ** 2 <= 1:
                d = math.hypot(x - 13, y - 14)
                put(x, y, MID if d < 4.5 else DARK)
    # head with its toothed edge and eyes
    for y in range(8, 13):
        for x in range(11, 22):
            if ((x - 16) / 5.4) ** 2 + ((y - 12.5) / 4.2) ** 2 <= 1:
                put(x, y, HEAD_L if math.hypot(x - 14, y - 10) < 1.8 else HEAD_M if y < 11 else HEAD_D)
    for x in (12, 14, 16, 18, 20):
        put(x, 8, HEAD_D)
    put(14, 11, OUTLINE); put(18, 11, OUTLINE)
    # the seam between the wing cases, and a gloss highlight
    line(16, 18, 16, 29, OUTLINE)
    put(11, 18, GLOSS); put(11, 19, GLOSS); put(13, 14, LIGHT); put(14, 14, LIGHT)
    # outline around everything drawn so far (not the sun)
    body = [row[:] for row in px]
    for y in range(H):
        for x in range(W):
            if body[y][x] is T and any(0 <= x + ox < W and 0 <= y + oy < H and body[y + oy][x + ox] not in (T, LEG)
                                     for ox, oy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                put(x, y, OUTLINE)
    # the sun it carries, and its rays (they pulse: long on frames 1 and 3)
    cx, cy = 16, 3 + bob
    for y in range(H):
        for x in range(W):
            d = math.hypot(x - cx, y - cy)
            if d <= 3.3:
                put(x, y, SUN_L if math.hypot(x - cx + 1, y - cy + 1) < 1.3 else SUN_M if d < 2.4 else SUN_E)
    ray = 6 if n in (1, 3) else 5
    for ang in (180, 210, 330, 0):            # left, upper-left, upper-right, right (none into the head)
        a = math.radians(ang)
        put(round(cx + math.cos(a) * ray), round(cy - math.sin(a) * ray), RAY)
        put(round(cx + math.cos(a) * (ray - 1)), round(cy - math.sin(a) * (ray - 1)), RAY)
    return px

def write_png(path, rows, scale=1):
    w, h = len(rows[0]) * scale, len(rows) * scale
    raw = b""
    for row in rows:
        line = b"".join(bytes((c or (0, 0, 0)) + ((255,) if c else (0,))) * scale for c in row)
        raw += (b"\x00" + line) * scale
    def chunk(t, d): return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) + \
          chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    open(path, "wb").write(png)

PREVIEW_SCALE = 8
DEFAULT_OUTPUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "resources", "sprites", "kepi_idle.png")


def main():
    frames = [frame(n) for n in range(FRAMES)]
    strip  = [sum((frames[n][y] for n in range(FRAMES)), []) for y in range(H)]
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT

    write_png(output, strip)
    print("wrote", output)

    if len(sys.argv) > 2:
        write_png(sys.argv[2], strip, PREVIEW_SCALE)
        print("wrote", sys.argv[2])
    return 0


if __name__ == "__main__":
    sys.exit(main())
