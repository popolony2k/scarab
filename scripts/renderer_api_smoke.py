#!/usr/bin/env python3
"""
Smoke test for the renderer Lua API (renderer_create / renderer_get_* / renderer_destroy).

A process gets exactly ONE renderer, so each scenario is a separate headless run of
scripts/renderer_api_smoke/main.lua, selected with RENDERER_SMOKE_SCENARIO:

  defaults  renderer_create() with no options gives exactly Scarab's defaults;
  create    every option set to a NON-default value, read back through
            renderer_get_config (plus the live title/zoom);
  errors    every rejection path (unknown option, wrong type, bad zoom, viewport that
            does not fit, ...) returns nil + the exact reason, and a valid create still works;
  lazy      a window-needing call FIRST creates the default renderer, and a later
            renderer_create says which call did it;
  nowindow  renderer-free calls do not create a renderer;
  views     every view_* function on the default view: exact zoom/limits/enabled/dimension/
            scroll-step/camera round trips, every rejection, and agreement with the older
            global camera_*/zoom_*/viewport_* functions;
  views_norenderer  a view call before any renderer is an error and creates none.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: renderer_api_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

SCENARIOS           = ["defaults", "create", "errors", "lazy", "nowindow", "views", "views_norenderer"]
MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/renderer_api_smoke/project.json"


def run_scenario(scarab, repo_root, scenario):
    env = dict(os.environ, RENDERER_SMOKE_SCENARIO=scenario)
    result = subprocess.run([scarab, "--headless", "--fast", "--max-frames", str(MAX_FRAMES), PROJECT],
                            cwd=repo_root, env=env, capture_output=True, text=True,
                            timeout=RUN_TIMEOUT_SECONDS)
    output = (result.stdout or "") + (result.stderr or "")
    ok = result.returncode == 0 and ("RENDERER SMOKE OK: " + scenario) in output

    print(f"[{'PASS' if ok else 'FAIL'}] renderer api scenario '{scenario}': exit={result.returncode}")

    if not ok:
        print("---- scarab output (last 30 lines) ----")
        print("\n".join(output.splitlines()[-30:]))
        print("---------------------------------------")

    return ok


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    scarab    = os.path.abspath(sys.argv[1])
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    passed    = True

    for scenario in SCENARIOS:
        passed &= run_scenario(scarab, repo_root, scenario)

    print("renderer api smoke test:", "PASSED" if passed else "FAILED")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
