"""
Smoke test for tilemap_unload_map (plans/MASTER_PLAN.md item V9).

Runs scripts/unload_map_smoke/main.lua headless once: unloading a map unloads every sprite on a
layer and takes their colliders out, so unloaded sprites never collide and re-added ones collide
once per frame. See the Lua script's own header.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: unload_map_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 400
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/unload_map_smoke/project.json"
OK_MARKER           = "UNLOAD MAP SMOKE OK"


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

    print(f"[{'PASS' if ok else 'FAIL'}] unload map: exit={result.returncode}")

    if not ok:
        failures = [line for line in output.splitlines() if "SMOKE FAIL" in line]
        print("---- failed checks ----")
        print("\n".join(failures) if failures else "(none printed)")
        print("---- scarab output (last 60 lines) ----")
        print("\n".join(output.splitlines()[-60:]))
        print("---------------------------------------")

    print("unload map smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
