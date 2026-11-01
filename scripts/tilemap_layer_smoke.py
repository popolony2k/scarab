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
Smoke test for a tile-map layer's opacity as Lua reads and writes it
(tilemap_get_layer / tilemap_set_layer and their _by_name forms).

Runs scripts/tilemap_layer_smoke/main.lua headless once, on resources/tilemap/test.tmx:
opacity is a number from 0 to 1: an opaque layer reads 1, the authored 0.69/0.65 layers read
exactly 0.69/0.65, writing a layer's own values back changes nothing however often, values
including 0.5 (which the old 0..255 integer moved to 0.502) round-trip exactly by id and by
name, and a value outside 0..1 or not a number is refused. See the Lua script's own header,
and plans/MASTER_PLAN.md items V3 and P7.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: tilemap_layer_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/tilemap_layer_smoke/project.json"
OK_MARKER           = "TILEMAP LAYER SMOKE OK"


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    scarab    = os.path.abspath(sys.argv[1])
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    result    = subprocess.run([scarab, "--headless", "--fast", "--max-frames", str(MAX_FRAMES), PROJECT],
                               cwd=repo_root, capture_output=True, text=True,
                               timeout=RUN_TIMEOUT_SECONDS)
    output    = (result.stdout or "") + (result.stderr or "")
    ok        = result.returncode == 0 and OK_MARKER in output

    print(f"[{'PASS' if ok else 'FAIL'}] tilemap layer: exit={result.returncode}")

    if not ok:
        print("---- scarab output (last 30 lines) ----")
        print("\n".join(output.splitlines()[-30:]))
        print("---------------------------------------")

    print("tilemap layer smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
