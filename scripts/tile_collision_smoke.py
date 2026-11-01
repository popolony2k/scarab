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
Smoke test for sprite-vs-tile collisions and tile lookups (plans/MASTER_PLAN.md item V11).

Runs scripts/tile_collision_smoke/main.lua headless at zoom 1 and 3.8125: tilemap_to_tile_matrix
and tilemap_get_tile work in map space, a sprite swept past a collision tile collides exactly
where it overlaps it (view-relative and world-space, with and without a camera offset), the
handler gets the tile's map rectangle, one call per touched tile, a tile with two collision
shapes doesn't hang, and the map's far edge is handled. See the Lua script's own header.

Each run quits itself only if every check passed (exit 0); otherwise it prints each failure and
never quits, so --max-frames runs out and scarab exits 3.

Usage: tile_collision_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 3000
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/tile_collision_smoke/project.json"
OK_MARKER           = "TILE COLLISION SMOKE OK"
ZOOMS               = ("1", "3.8125")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    scarab    = os.path.abspath(sys.argv[1])
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ok        = True

    for zoom in ZOOMS:
        result = subprocess.run([scarab, "--headless", "--fast", "--max-frames", str(MAX_FRAMES), PROJECT],
                                cwd=repo_root, env=dict(os.environ, ZOOM=zoom), capture_output=True,
                                text=True, timeout=RUN_TIMEOUT_SECONDS)
        output = (result.stdout or "") + (result.stderr or "")
        run_ok = result.returncode == 0 and OK_MARKER in output

        print(f"[{'PASS' if run_ok else 'FAIL'}] tile collisions, zoom {zoom}: exit={result.returncode}")

        if not run_ok:
            ok = False
            failures = [line for line in output.splitlines() if "SMOKE FAIL" in line]
            print("---- failed checks ----")
            print("\n".join(failures) if failures else "(none printed)")
            print("---- scarab output (last 30 lines) ----")
            print("\n".join(output.splitlines()[-30:]))
            print("---------------------------------------")

    print("tile collision smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
