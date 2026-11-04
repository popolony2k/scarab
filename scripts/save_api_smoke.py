#!/usr/bin/env python3
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
Smoke test for the save functions save_write / save_read / save_exists / save_delete
(plans/MASTER_PLAN.md item C1c).

Runs scripts/save_api_smoke/main.lua headless in these scenarios (see that script's header):
  api       --save-dir on a temp folder, from the script's own folder: every function and every
            refusal; afterwards no "*.tmp" may be left in the save folder;
  fixed     twice, into two save folders: fixed.json must be byte-identical to EXPECTED_FIXED both times
            (pretty-printed, keys sorted - Lua's hash order changes per launch, the file must not);
  nodir     a project with no save directory;
  spoiled   saves edited by hand: one not JSON, one a bare number;
  archive   the game packed as a plain .zip, run twice from an EMPTY directory with one --save-dir: the
            first run saves, the second reads it back; the save on disk is plain JSON;
  encrypted the same from an encrypted `scarab --pack` archive (with --require-encrypted, as ci.yml
            passes; needs a build with SCARAB_CONTENT_KEY) - the save is still plain JSON.

Every run points HOME / APPDATA / XDG_DATA_HOME at a temp folder, so no run can reach the real per-user
folder. Every run quits itself only if every check passed (exit 0); a failure never quits, so
--max-frames runs out and scarab exits 3 with the failed checks printed.

Usage: save_api_smoke.py <path-to-scarab-executable> [--require-encrypted]
"""

import glob
import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
OK_MARKER           = "SAVE API SMOKE OK"
SCRIPT_DIR          = os.path.join("scripts", "save_api_smoke")
FOLDER_NAME         = "folder.json"           # a FOLDER named like a save (the api scenario)
EXPECTED_FIXED      = """{
  "empty": {},
  "list": [
    1,
    2.5,
    "x"
  ],
  "name": "AAA",
  "nested": {
    "b": {
      "c": false
    }
  },
  "ratio": 0.5,
  "score": 3000,
  "sound": true
}
"""


def safe_environment(home):
    """HOME / APPDATA / XDG_DATA_HOME pointed into the test's temp folder."""
    return dict(os.environ, HOME=home, APPDATA=os.path.join(home, "appdata"), XDG_DATA_HOME=os.path.join(home, "xdg"))


def run(scarab, args, cwd, home, scenario, label):
    """Runs scarab headless in one scenario; True if the script reported OK."""
    env    = safe_environment(home)
    env["SMOKE_SCENARIO"] = scenario
    result = subprocess.run([scarab, "--headless", "--fast", "--max-frames", str(MAX_FRAMES)] + args,
                            cwd=cwd, env=env, capture_output=True, text=True, timeout=RUN_TIMEOUT_SECONDS)
    output = (result.stdout or "") + (result.stderr or "")
    ok     = result.returncode == 0 and OK_MARKER in output

    print(f"[{'PASS' if ok else 'FAIL'}] {label}: exit={result.returncode}")

    if not ok:
        failures = [line for line in output.splitlines() if "SMOKE FAIL" in line]
        print("---- failed checks ----")
        print("\n".join(failures) if failures else "(none printed)")
        print("---- scarab output (last 40 lines) ----")
        print("\n".join(output.splitlines()[-40:]))
        print("---------------------------------------")

    return ok


def report(ok, label):
    """Prints one driver-side check."""
    print(f"[{'PASS' if ok else 'FAIL'}] {label}")
    return ok


def no_temp_files(save_dir, label):
    """No sunlight temp file ("<name>.tmp") may be left behind after the saves."""
    left = glob.glob(os.path.join(save_dir, "**", "*.tmp"), recursive=True)
    return report(not left, f"{label}: no .tmp file left behind {left if left else ''}")


def read_bytes(path):
    """A file's bytes, or None when it is missing."""
    if not os.path.isfile(path):
        return None
    with open(path, "rb") as f:
        return f.read()


def build_archive_source(source_dir, repo_root):
    """The packed game: the smoke script, a project file, and the shipped file."""
    os.makedirs(source_dir)
    for name in ("main.lua", "shipped.json"):
        shutil.copy(os.path.join(repo_root, SCRIPT_DIR, name), os.path.join(source_dir, name))
    with open(os.path.join(source_dir, "project.json"), "w", encoding="utf-8", newline="\n") as f:
        f.write('{ "main_script": "main.lua" }\n')


def make_plain_zip(source_dir, zip_path):
    """A plain (unencrypted) archive - entry names always use '/'."""
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name in os.listdir(source_dir):
            archive.write(os.path.join(source_dir, name), name)


def run_archive_twice(scarab, archive, work, home, label):
    """First run: no save, save. Second run: the save. Same --save-dir, from an empty folder."""
    empty    = tempfile.mkdtemp(prefix="empty-", dir=work)
    save_dir = tempfile.mkdtemp(prefix="saves-", dir=work)
    ok       = run(scarab, ["--save-dir", save_dir, archive], empty, home, "first", label + ", first run")
    saved    = read_bytes(os.path.join(save_dir, "progress.json"))
    ok      &= report(saved is not None and b'"level": 2' in saved, label + ": the save on disk is plain JSON")
    ok      &= run(scarab, ["--save-dir", save_dir, archive], empty, home, "second", label + ", second run")
    ok      &= no_temp_files(save_dir, label)
    return ok


def main():
    args  = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = {a for a in sys.argv[1:] if a.startswith("--")}

    if len(args) != 1 or not flags <= {"--require-encrypted"}:
        print(__doc__)
        return 2

    scarab     = os.path.abspath(args[0])
    repo_root  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    script_dir = os.path.join(repo_root, SCRIPT_DIR)
    ok         = True

    with tempfile.TemporaryDirectory(prefix="scarab-save-api-smoke-") as work:
        home = os.path.join(work, "safe-home")

        # api: every function, from the script's folder (shipped.json at the virtual root)
        save_dir = os.path.join(work, "api-saves")
        os.makedirs(os.path.join(save_dir, FOLDER_NAME))
        ok &= run(scarab, ["--save-dir", save_dir, "project.json"], script_dir, home, "api", "api, with --save-dir")
        ok &= no_temp_files(save_dir, "api")
        ok &= report(read_bytes(os.path.join(script_dir, "shipped.json")) is not None, "api: the shipped file is untouched")

        # fixed: the exact bytes, the same on two launches
        written = []
        for launch in (1, 2):
            fixed_dir = os.path.join(work, f"fixed-{launch}")
            ok &= run(scarab, ["--save-dir", fixed_dir, "project.json"], script_dir, home, "fixed", f"fixed, launch {launch}")
            written.append(read_bytes(os.path.join(fixed_dir, "fixed.json")))
        ok &= report(written[0] == EXPECTED_FIXED.encode("utf-8"),
                     "fixed: fixed.json is the expected text" + ("" if written[0] == EXPECTED_FIXED.encode("utf-8") else f"\n{written[0]!r}"))
        ok &= report(written[0] == written[1], "fixed: two launches write the same bytes")

        # nodir
        ok &= run(scarab, ["nodir.json"], script_dir, home, "nodir", "nodir, no save directory")
        ok &= report(not os.path.exists(home), "nodir: nothing written into the (fake) per-user folder")

        # spoiled: saves edited by hand
        spoiled_dir = os.path.join(work, "spoiled")
        os.makedirs(spoiled_dir)
        with open(os.path.join(spoiled_dir, "broken.json"), "w", encoding="utf-8") as f:
            f.write("{ not json")
        with open(os.path.join(spoiled_dir, "scalar.json"), "w", encoding="utf-8") as f:
            f.write("42\n")
        ok &= run(scarab, ["--save-dir", spoiled_dir, "project.json"], script_dir, home, "spoiled", "spoiled saves")

        # archive and encrypted pack
        source_dir = os.path.join(work, "source")
        build_archive_source(source_dir, repo_root)
        plain_zip = os.path.join(work, "game.zip")
        make_plain_zip(source_dir, plain_zip)
        ok &= run_archive_twice(scarab, plain_zip, work, home, "plain .zip")

        if "--require-encrypted" in flags:
            encrypted_zip = os.path.join(work, "game-encrypted.zip")
            pack_config   = os.path.join(work, "pack-config.json")

            with open(pack_config, "w", encoding="utf-8") as f:
                json.dump({"source_dir": source_dir, "output": encrypted_zip}, f)

            result = subprocess.run([scarab, "--pack", pack_config], cwd=work, capture_output=True, text=True,
                                    timeout=RUN_TIMEOUT_SECONDS)

            if result.returncode != 0 or not os.path.isfile(encrypted_zip):
                print(f"[FAIL] encrypted pack: `scarab --pack` exited {result.returncode}")
                print((result.stdout or "") + (result.stderr or ""))
                ok = False
            else:
                ok &= run_archive_twice(scarab, encrypted_zip, work, home, "ENCRYPTED pack")

    print("save api smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
