"""
Smoke test for the sprite opacity and tint functions (doc/MASTER_PLAN.md item P8).

Runs scripts/sprite_tint_smoke/main.lua headless once: a fresh sprite's defaults, exact round
trips, every rejection, two sprites sharing a texture keeping their own values, a recycled slot
starting opaque and untinted, and a few frames of drawing. See the Lua script's own header.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: sprite_tint_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/sprite_tint_smoke/project.json"
OK_MARKER           = "SPRITE TINT SMOKE OK"


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

    print(f"[{'PASS' if ok else 'FAIL'}] sprite tint: exit={result.returncode}")

    if not ok:
        failures = [line for line in output.splitlines() if "SMOKE FAIL" in line or "SMOKE NOTE" in line]
        print("---- failed checks ----")
        print("\n".join(failures) if failures else "(none printed)")
        print("---- scarab output (last 60 lines) ----")
        print("\n".join(output.splitlines()[-60:]))
        print("---------------------------------------")

    print("sprite tint smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
