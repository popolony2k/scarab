#!/usr/bin/env python3
"""
Smoke test for a tile-map layer's opacity as Lua reads and writes it
(tilemap_get_layer / tilemap_set_layer and their _by_name forms).

Runs scripts/tilemap_layer_smoke/main.lua headless once, on resources/tilemap/test.tmx:
an opaque layer reads 255, the authored 0.69/0.65 layers read 176/166, writing a layer's
own values back changes nothing, 0/1/128/254/255 round-trip exactly by id and by name,
writing 255 keeps a layer visible, and out-of-range values are clamped to 0..255. See the
Lua script's own header, and doc/MASTER_PLAN.md item V3.

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
