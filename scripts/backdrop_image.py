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
Draws resources/tilemap/screenspace/backdrop.png: a full-window (1260x920, Scarab's default render area)
picture for the screen-space image layer of resources/tilemap/screenspace/screenspace.tmx (plans/MASTER_PLAN.md
item C9). A night-blue to violet gradient, a gold border along every edge, a gold square in each corner and a
ring in the middle: if any edge, corner or the ring's centre is missing, the layer is not being drawn whole at
its window position - which is what an eye check of samples/backdrop looks for.

Needs only the standard library (it writes the PNG itself). The output is deterministic: running it again
writes the same bytes.
"""

import os
import struct
import zlib

WIDTH, HEIGHT  = 1260, 920                    # Scarab's default render area (src/main.h DISPLAY_W/H)
TOP, BOTTOM    = ( 16, 32, 64 ), ( 64, 32, 80 )
GOLD           = ( 224, 176, 64 )
BORDER         = 8                            # the gold edge, in pixels
CORNER         = 48                           # the corner squares' side
RING_RADIUS    = 200
RING_WIDTH     = 10
OUTPUT         = os.path.join( os.path.dirname( os.path.dirname( os.path.abspath( __file__ ) ) ),
                               "resources", "tilemap", "screenspace", "backdrop.png" )


def pixel( x, y ):
    """The colour of one pixel."""
    if x < BORDER or y < BORDER or x >= WIDTH - BORDER or y >= HEIGHT - BORDER:
        return GOLD
    if ( x < CORNER or x >= WIDTH - CORNER ) and ( y < CORNER or y >= HEIGHT - CORNER ):
        return GOLD
    dx, dy = x - WIDTH // 2, y - HEIGHT // 2
    if abs( ( dx * dx + dy * dy ) ** 0.5 - RING_RADIUS ) < RING_WIDTH / 2:
        return GOLD
    t = y / ( HEIGHT - 1 )
    return tuple( round( a + ( b - a ) * t ) for a, b in zip( TOP, BOTTOM ) )


def write_png( path ):
    """Write the picture as an 8-bit RGB PNG."""
    raw = bytearray()
    for y in range( HEIGHT ):
        raw.append( 0 )
        for x in range( WIDTH ):
            raw.extend( pixel( x, y ) )

    def chunk( kind, data ):
        """One PNG chunk: length, kind, data, CRC."""
        return struct.pack( ">I", len( data ) ) + kind + data + struct.pack( ">I", zlib.crc32( kind + data ) & 0xffffffff )

    png = ( b"\x89PNG\r\n\x1a\n" + chunk( b"IHDR", struct.pack( ">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0 ) ) +
            chunk( b"IDAT", zlib.compress( bytes( raw ), 9 ) ) + chunk( b"IEND", b"" ) )
    os.makedirs( os.path.dirname( path ), exist_ok=True )
    with open( path, "wb" ) as f:
        f.write( png )


def main():
    """Draw the backdrop and say where it went."""
    write_png( OUTPUT )
    print( OUTPUT, os.path.getsize( OUTPUT ), "bytes" )


if __name__ == "__main__":
    main()
