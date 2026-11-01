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
Run Scarab's headless checks under AddressSanitizer / LeakSanitizer (plans/MASTER_PLAN.md item R1).

Runs, with a scarab built with -fsanitize=address (see ci.yml's "asan" job, or CLAUDE.md's ASan recipe):

  1. the headless smoke test (must exit 0) and its frame-budget run (must exit 3);
  2. every scripts/*_smoke.py driver (tileset_archive_smoke with --require-encrypted when asked);
  3. every sample headless for SAMPLE_FRAMES frames (exit 0 or 3: a sample never quits by itself,
     so its frame budget running out is the normal end) - shutdown with live sprites, fonts and maps.

Every sanitizer report is written to a log directory (ASAN_OPTIONS log_path), so a report fails the
run even where a smoke driver would only print its output's tail. With leak detection on (the default
on Linux), a leak is a report too. Known third-party leaks are suppressed only through
scripts/lsan.supp, each entry with its reason.

Usage: asan_run.py <path-to-asan-scarab> [--require-encrypted] [--no-leaks]
       --no-leaks: for macOS, whose AddressSanitizer has no LeakSanitizer.
"""

import glob
import os
import shutil
import subprocess
import sys
import tempfile

SAMPLE_FRAMES       = 120
RUN_TIMEOUT_SECONDS = 600
BUDGET_EXHAUSTED    = 3
HEADLESS_PROJECT    = "scripts/headless_smoke/project.json"
HEADLESS_FRAMES_OK  = 1000
HEADLESS_FRAMES_LOW = 50


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]

    if len(args) != 1:
        print(__doc__)
        return 2

    scarab       = os.path.abspath(args[0])
    encrypted    = "--require-encrypted" in sys.argv
    leaks        = "--no-leaks" not in sys.argv
    repo_root    = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    log_dir      = tempfile.mkdtemp(prefix="scarab-asan-")
    suppressions = os.path.join(repo_root, "scripts", "lsan.supp")

    env = dict(os.environ)
    env["ASAN_OPTIONS"] = ":".join([
        "detect_leaks=" + ("1" if leaks else "0"),
        "halt_on_error=1",
        "abort_on_error=1",
        "log_path=" + os.path.join(log_dir, "asan"),
    ])
    env["LSAN_OPTIONS"] = "suppressions=" + suppressions + ":print_suppressions=1"
    env["SCARAB_SANITIZER"] = "address"    # load_map_smoke skips its peak-memory comparison

    runs = []      # (label, command, accepted exit codes)
    runs.append(("headless smoke", [scarab, "--headless", "--fast", "--max-frames", str(HEADLESS_FRAMES_OK), HEADLESS_PROJECT], {0}))
    runs.append(("headless smoke, budget", [scarab, "--headless", "--fast", "--max-frames", str(HEADLESS_FRAMES_LOW), HEADLESS_PROJECT], {BUDGET_EXHAUSTED}))

    for driver in sorted(glob.glob(os.path.join(repo_root, "scripts", "*_smoke.py"))):
        command = [sys.executable, driver, scarab]

        if encrypted and os.path.basename(driver) == "tileset_archive_smoke.py":
            command.append("--require-encrypted")

        runs.append((os.path.basename(driver), command, {0}))

    for project in sorted(glob.glob(os.path.join(repo_root, "samples", "*", "project.json"))):
        relative = os.path.relpath(project, repo_root)
        runs.append((relative, [scarab, "--headless", "--fast", "--max-frames", str(SAMPLE_FRAMES), relative], {0, BUDGET_EXHAUSTED}))

    failed = []

    for label, command, accepted in runs:
        before = set(os.listdir(log_dir))
        result = subprocess.run(command, cwd=repo_root, env=env, capture_output=True, text=True, timeout=RUN_TIMEOUT_SECONDS)
        reports = sorted(set(os.listdir(log_dir)) - before)
        ok = result.returncode in accepted and not reports

        print(f"[{'PASS' if ok else 'FAIL'}] {label}: exit={result.returncode}" + (f", {len(reports)} sanitizer report(s)" if reports else ""))

        if not ok:
            failed.append(label)
            output = (result.stdout or "") + (result.stderr or "")
            print("---- output (last 40 lines) ----")
            print("\n".join(output.splitlines()[-40:]))

            for report in reports:
                print(f"---- sanitizer report {report} ----")
                with open(os.path.join(log_dir, report), errors="replace") as f:
                    print(f.read())

    shutil.rmtree(log_dir, ignore_errors=True)
    print(f"asan run: {len(runs) - len(failed)} of {len(runs)} passed" + (" - FAILED: " + ", ".join(failed) if failed else ""))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
