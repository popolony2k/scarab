#!/usr/bin/env python3
"""
Smoke test for the blend-mode Lua API (BLEND_MODE_*, draw_begin_blend_mode /
draw_end_blend_mode, sprite_set / clear / get_blend_mode).

Runs scripts/blend_api_smoke/main.lua headless once: the constants' values, every round
trip, every error (nil + the reason), that a recycled sprite slot never inherits its previous
owner's mode, that the sprite functions create no renderer, and that a begin with no end is
harmless across frames. See the Lua script's own header, and doc/MASTER_PLAN.md item B1.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: blend_api_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/blend_api_smoke/project.json"
OK_MARKER           = "BLEND API SMOKE OK"


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

    print(f"[{'PASS' if ok else 'FAIL'}] blend API: exit={result.returncode}")

    if not ok:
        print("---- scarab output (last 30 lines) ----")
        print("\n".join(output.splitlines()[-30:]))
        print("---------------------------------------")

    print("blend API smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
