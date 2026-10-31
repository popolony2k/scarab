"""
Smoke test for tilemap_load_map over a loaded map (doc/MASTER_PLAN.md item V10).

Runs scripts/load_map_smoke/main.lua headless twice: a map that fails to parse leaves the loaded
map loaded, sprites stay registered across a load over a loaded map, and - on Linux and macOS,
where the standard library can read a child's peak memory - loading the map over itself many
times does not grow peak memory compared with a few loads (up to sunlight v0.38.0 every such load
leaked the whole previous map: the published Scarab v0.1.25 grew about 30 KB per load headless).
On Windows only the functional checks run, and so under a sanitizer (SCARAB_SANITIZER set, as
scripts/asan_run.py does): AddressSanitizer keeps freed memory in quarantine, so peak memory grows
whatever the code does, and LeakSanitizer checks leaks there more strictly. See the Lua script's own
header.

Each run quits itself only if every check passed (exit 0); otherwise it prints each failure and
never quits, so --max-frames runs out and scarab exits 3.

Usage: load_map_smoke.py <path-to-scarab-executable>
"""

import os
import subprocess
import sys

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 300
PROJECT             = "scripts/load_map_smoke/project.json"
OK_MARKER           = "LOAD MAP SMOKE OK"
FEW_LOADS           = 20
MANY_LOADS          = 1000
# Peak-memory growth allowed from FEW_LOADS to MANY_LOADS. Measured 2026-10-29 on macOS x64,
# headless: the fixed build (sunlight a6ac4f6) grows about 0 MB, the published v0.1.25 (sunlight
# v0.38.0, which leaked every previous map) about 30 MB - see the measurement in
# doc/MASTER_PLAN.md item V10.
MAX_GROWTH_BYTES    = 8 * 1024 * 1024
# ru_maxrss is in bytes on macOS, in kilobytes on Linux.
MAXRSS_UNIT         = 1 if sys.platform == "darwin" else 1024
# Set by scripts/asan_run.py: the peak-memory comparison means nothing under AddressSanitizer.
UNDER_SANITIZER     = bool(os.environ.get("SCARAB_SANITIZER"))


def run(scarab, repo_root, loads):
    """Run the project once with LOADS=loads; return (ok, output, peak bytes or None)."""

    env  = dict(os.environ, LOADS=str(loads))
    args = [scarab, "--headless", "--fast", "--max-frames", str(MAX_FRAMES), PROJECT]

    if not hasattr(os, "wait4"):
        result = subprocess.run(args, cwd=repo_root, env=env, capture_output=True, text=True,
                                timeout=RUN_TIMEOUT_SECONDS)
        output = (result.stdout or "") + (result.stderr or "")
        return result.returncode == 0 and OK_MARKER in output, output, None

    process = subprocess.Popen(args, cwd=repo_root, env=env, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True)
    output  = process.stdout.read()
    _, status, usage = os.wait4(process.pid, 0)
    process.returncode = os.waitstatus_to_exitcode(status)

    return process.returncode == 0 and OK_MARKER in output, output, usage.ru_maxrss * MAXRSS_UNIT


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    scarab    = os.path.abspath(sys.argv[1])
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ok        = True
    peaks     = {}

    for loads in (FEW_LOADS, MANY_LOADS):
        run_ok, output, peak = run(scarab, repo_root, loads)
        peaks[loads]         = peak

        print(f"[{'PASS' if run_ok else 'FAIL'}] load map, {loads} loads: peak "
              f"{'n/a' if peak is None else f'{peak / 1048576:.1f} MB'}")

        if not run_ok:
            ok = False
            failures = [line for line in output.splitlines() if "SMOKE FAIL" in line]
            print("---- failed checks ----")
            print("\n".join(failures) if failures else "(none printed)")
            print("---- scarab output (last 30 lines) ----")
            print("\n".join(output.splitlines()[-30:]))
            print("---------------------------------------")

    if UNDER_SANITIZER:
        print("[SKIP] peak memory growth: under a sanitizer (its quarantine holds freed memory; LeakSanitizer checks leaks)")
    elif peaks[FEW_LOADS] is not None and peaks[MANY_LOADS] is not None:
        growth = peaks[MANY_LOADS] - peaks[FEW_LOADS]
        grew   = growth > MAX_GROWTH_BYTES

        print(f"[{'FAIL' if grew else 'PASS'}] peak memory growth from {FEW_LOADS} to {MANY_LOADS} loads: "
              f"{growth / 1048576:.1f} MB (limit {MAX_GROWTH_BYTES / 1048576:.0f} MB)")

        if grew:
            ok = False
    else:
        print("[SKIP] peak memory growth: this platform has no os.wait4")

    print("load map smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
