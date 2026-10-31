#!/usr/bin/env python3
"""
Smoke test for the camera's boundary behavior (camera_move_* / view_move_camera_*).

Runs scripts/camera_boundary_smoke/main.lua headless once: on resources/tilemap/test.tmx,
with a scroll step that divides none of the distances to the map's far edges, the camera
must stop EXACTLY on the far edge (a partial last step) and come back EXACTLY to the
origin - on the default view at Scarab's default zoom (3.8125), through both the old
camera_move_* globals and view_move_camera_*, and on an extra view at zoom 1.0.
See the Lua script's own header, and plans/MASTER_PLAN.md item V1.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: camera_boundary_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/camera_boundary_smoke/project.json"
OK_MARKER           = "CAMERA BOUNDARY SMOKE OK"


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

    print(f"[{'PASS' if ok else 'FAIL'}] camera boundary: exit={result.returncode}")

    if not ok:
        print("---- scarab output (last 30 lines) ----")
        print("\n".join(output.splitlines()[-30:]))
        print("---------------------------------------")

    print("camera boundary smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
