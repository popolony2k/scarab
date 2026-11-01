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
Smoke test for path schemes (plans/MASTER_PLAN.md item P9, sunlight I12).

Runs scripts/path_scheme_smoke/main.lua headless twice: a plain path, vpath://, virtualpath://,
hpath:// and hostpath:// all load the same file through load_json, dofile and tilemap_load_map, and
an unknown scheme is refused. The first run starts from project.json (a plain "main_script"), the
second from scheme_project.json, whose "main_script" is "vpath://..." - a scheme there is used as it
stands, not joined onto the project file's folder. See the Lua script's own header.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: path_scheme_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
PROJECTS            = ["scripts/path_scheme_smoke/project.json",          # main_script "main.lua"
                       "scripts/path_scheme_smoke/scheme_project.json"]   # main_script "vpath://..."
OK_MARKER           = "PATH SCHEME SMOKE OK"


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    scarab    = os.path.abspath(sys.argv[1])
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    env       = dict(os.environ, SCARAB_SMOKE_REPO_ROOT=repo_root)   # for the hpath:// cases
    all_ok    = True

    for project in PROJECTS:
        result = subprocess.run([scarab, "--headless", "--fast", "--max-frames", str(MAX_FRAMES), project],
                                cwd=repo_root, env=env, capture_output=True, text=True,
                                timeout=RUN_TIMEOUT_SECONDS)
        output = (result.stdout or "") + (result.stderr or "")
        ok     = result.returncode == 0 and OK_MARKER in output
        all_ok = all_ok and ok

        print(f"[{'PASS' if ok else 'FAIL'}] path schemes from {project}: exit={result.returncode}")

        if not ok:
            failures = [line for line in output.splitlines() if "SMOKE FAIL" in line or "SMOKE NOTE" in line]
            print("---- failed checks ----")
            print("\n".join(failures) if failures else "(none printed)")
            print("---- scarab output (last 60 lines) ----")
            print("\n".join(output.splitlines()[-60:]))
            print("---------------------------------------")

    print("path scheme smoke test:", "PASSED" if all_ok else "FAILED")
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
