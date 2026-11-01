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
Smoke test for the layer tint functions (plans/MASTER_PLAN.md item V12).

Runs scripts/layer_tint_smoke/main.lua headless once on resources/tilemap/tint/tint.tmx:
every authored Tiled tintcolor read back exactly (alpha included, white without one) for tile,
image, object and group layers, set then get, a write-back changing nothing, tilemap_set_layer
keeping the tint, and every error. See the Lua script's own header.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: layer_tint_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/layer_tint_smoke/project.json"
OK_MARKER           = "LAYER TINT SMOKE OK"


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

    print(f"[{'PASS' if ok else 'FAIL'}] layer tint: exit={result.returncode}")

    if not ok:
        failures = [line for line in output.splitlines() if "SMOKE FAIL" in line or "SMOKE NOTE" in line]
        print("---- failed checks ----")
        print("\n".join(failures) if failures else "(none printed)")
        print("---- scarab output (last 60 lines) ----")
        print("\n".join(output.splitlines()[-60:]))
        print("---------------------------------------")

    print("layer tint smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
