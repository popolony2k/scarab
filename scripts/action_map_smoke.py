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
Smoke test for the save directory, the action map and rumble (plans/MASTER_PLAN.md item C1b).

Runs scripts/action_map_smoke/main.lua headless in five scenarios (see that script's header):
  api       with --save-dir on a temp folder - every function, every error; the saved file is then
            checked on disk;
  priority  no --save-dir, with HOME / APPDATA / XDG_DATA_HOME pointed at a temp folder, so the real
            per-user data folder is computed but never touched: project.json's "save_directory", then
            app_set_save_directory winning over it;
  nodir     a project with no save directory: a plain save refused, an hpath:// save working;
  archive   the game packed as a plain .zip, run twice from an EMPTY directory with one --save-dir: the
            first run loads the defaults shipped under APP_DIR and saves; the second loads the save;
  encrypted the same from an encrypted `scarab --pack` archive (with --require-encrypted, as ci.yml
            passes; needs a build with SCARAB_CONTENT_KEY) - the shipped defaults go through the read
            filter, the plain save too.

Every run quits itself only if every check passed (exit 0); a failure never quits, so --max-frames
runs out and scarab exits 3 with the failed checks printed.

Usage: action_map_smoke.py <path-to-scarab-executable> [--require-encrypted]
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
OK_MARKER           = "ACTION MAP SMOKE OK"
SCRIPT_DIR          = os.path.join("scripts", "action_map_smoke")
SHIPPED_DEFAULTS    = "# sunlight action map\njump=KEY:65\n"     # KEY_A - what the script expects
PROJECT_NAME        = "FromProject"                                 # project.json's "save_directory"
SCRIPT_NAME         = "FromScript"                                  # what the priority scenario sets


def user_data_base(home):
    """The per-user data folder Scarab must compute for a fake home, and the environment that gives it."""
    if sys.platform == "win32":
        appdata = os.path.join(home, "appdata")
        return appdata, {"APPDATA": appdata}
    if sys.platform == "darwin":
        return os.path.join(home, "Library", "Application Support"), {"HOME": home}
    xdg = os.path.join(home, "xdg")
    return xdg, {"HOME": home, "XDG_DATA_HOME": xdg}


SAFE_HOME = None      # set by main(): every run's HOME/APPDATA/XDG_DATA_HOME, so no run can reach the real ones


def run(scarab, args, cwd, env_extra, label):
    """Runs scarab headless, with the per-user folder pointed into the test's temp folder (even a broken
    --save-dir priority then writes nowhere real); True if the script reported OK."""
    env    = dict(os.environ, **user_data_base(SAFE_HOME)[1])
    env.update(env_extra)
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


def file_has(path, text, label):
    """True if a file exists and holds text; reports the check."""
    ok = os.path.isfile(path) and text in open(path, encoding="utf-8").read()
    print(f"[{'PASS' if ok else 'FAIL'}] {label}: {path}")
    return ok


def build_archive_source(source_dir, repo_root):
    """The packed game: the smoke script, a project file, and the defaults shipped under APP_DIR."""
    os.makedirs(source_dir)
    shutil.copy(os.path.join(repo_root, SCRIPT_DIR, "main.lua"), os.path.join(source_dir, "main.lua"))

    with open(os.path.join(source_dir, "project.json"), "w", encoding="utf-8", newline="\n") as f:
        f.write('{ "main_script": "main.lua" }\n')

    with open(os.path.join(source_dir, "default_bindings.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write(SHIPPED_DEFAULTS)


def make_plain_zip(source_dir, zip_path):
    """A plain (unencrypted) archive - entry names always use '/'."""
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name in os.listdir(source_dir):
            archive.write(os.path.join(source_dir, name), name)


def run_archive_twice(scarab, archive, work, label):
    """First run: shipped defaults, save. Second run: the save. Same --save-dir, from an empty folder."""
    empty    = tempfile.mkdtemp(prefix="empty-", dir=work)
    save_dir = tempfile.mkdtemp(prefix="saves-", dir=work)
    ok       = run(scarab, ["--save-dir", save_dir, archive], empty, {"SMOKE_SCENARIO": "default"}, label + ", first run (shipped defaults)")
    ok      &= file_has(os.path.join(save_dir, "bindings.txt"), "jump=KEY:66", label + ": the save is on disk")
    ok      &= run(scarab, ["--save-dir", save_dir, archive], empty, {"SMOKE_SCENARIO": "saved"}, label + ", second run (the save)")
    return ok


def main():
    args  = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = {a for a in sys.argv[1:] if a.startswith("--")}

    if len(args) != 1 or not flags <= {"--require-encrypted"}:
        print(__doc__)
        return 2

    scarab    = os.path.abspath(args[0])
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    project   = os.path.join(SCRIPT_DIR, "project.json")
    ok        = True

    global SAFE_HOME

    with tempfile.TemporaryDirectory(prefix="scarab-action-map-smoke-") as work:
        SAFE_HOME = os.path.join(work, "safe-home")
        # api: --save-dir wins
        save_dir = os.path.join(work, "cli-saves")
        ok &= run(scarab, ["--save-dir", save_dir, project], repo_root,
                  {"SMOKE_SCENARIO": "api", "SMOKE_EXPECT_DIR": save_dir}, "api, with --save-dir")
        ok &= file_has(os.path.join(save_dir, "bindings.txt"), "jump=KEY:", "api: the saved bindings are on disk")

        # priority: the real per-user folder, computed under a fake home
        home       = os.path.join(work, "home")
        base, env  = user_data_base(home)
        env.update({"SMOKE_SCENARIO": "priority",
                    "SMOKE_EXPECT_PROJECT": os.path.join(base, PROJECT_NAME),
                    "SMOKE_EXPECT_SCRIPT": os.path.join(base, SCRIPT_NAME)})
        ok &= run(scarab, [project], repo_root, env, "priority, project.json then app_set_save_directory")
        ok &= file_has(os.path.join(base, SCRIPT_NAME, "bindings.txt"), "jump=KEY:", "priority: saved into the script's folder")

        # nodir
        host_file = os.path.join(work, "host-bindings.txt")
        ok &= run(scarab, [os.path.join(SCRIPT_DIR, "nodir.json")], repo_root,
                  {"SMOKE_SCENARIO": "nodir", "SMOKE_HOST_FILE": host_file}, "nodir, no save directory")

        # archive and encrypted pack
        source_dir = os.path.join(work, "source")
        build_archive_source(source_dir, repo_root)
        plain_zip = os.path.join(work, "game.zip")
        make_plain_zip(source_dir, plain_zip)
        ok &= run_archive_twice(scarab, plain_zip, work, "plain .zip")

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
                ok &= run_archive_twice(scarab, encrypted_zip, work, "ENCRYPTED pack")

    print("action map smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
