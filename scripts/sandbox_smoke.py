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
Smoke test for the sandbox and the path rule (plans/MASTER_PLAN.md item C8).

Runs scripts/sandbox_smoke/main.lua headless (see its own header for each scenario):
  loose      the "rule" suite from the repo root, with --save-dir: reading finds the save directory, then
             the game's own files; writing goes into the save directory; then checks on disk that the
             saves are there and that NO game file was changed
  zip        the same suite from a plain .zip, run from an EMPTY folder that holds outside.txt (which the
             game must not see: the launch folder is not the game's files when it runs from a .zip)
  encrypted  with --require-encrypted, the same from a `scarab --pack` archive
  nodir      no save directory: reading works, writing is refused
  switch     a save folder named by the project, then changed by app_set_save_directory: the old folder's
             files are no longer found (sunlight I22's Unmount)
  nosandbox  --no-sandbox gives Lua's own functions back

Every run starts from a temp COPY of the game (never from the repo) and points HOME / APPDATA /
XDG_DATA_HOME at a temp folder, so a broken sandbox can change nothing real; the script also refuses
to run its writes unless the sandbox is installed. A failing script never quits, so --max-frames runs
out and scarab exits 3.

Usage: sandbox_smoke.py <path-to-scarab-executable> [--require-encrypted]
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
OK_MARKER           = "SANDBOX SMOKE OK"
SCRIPT_DIR          = os.path.join("scripts", "sandbox_smoke")
LOOSE_GAME_DIR      = "game/"                      # the game's files as a plain path from the loose launch folder
LOOSE_APPDIR_FILE   = "tools/pack.lua"              # scarab_copy_binaries puts tools/ next to the executable
ZIP_APPDIR_FILE     = "game.txt"                    # from a .zip, APP_DIR is the archive
GAME_FILES          = ["game.txt", "lines.txt", "numbers.txt", "append.txt", "rplus.txt", "data.json",
                       "project.json", "switch.json", "main.lua", "mod.lua", "shadow_mod.lua", "envmod.lua"]
SWITCH_FOLDERS      = ("scarab-sandbox-smoke-a", "scarab-sandbox-smoke-b")


def user_data_base(home):
    """The per-user data folder Scarab computes for a fake home, and the environment that gives it."""
    if sys.platform == "win32":
        appdata = os.path.join(home, "appdata")
        return appdata, {"APPDATA": appdata}
    if sys.platform == "darwin":
        return os.path.join(home, "Library", "Application Support"), {"HOME": home}
    xdg = os.path.join(home, "xdg")
    return xdg, {"HOME": home, "XDG_DATA_HOME": xdg}


def run(scarab, args, cwd, home, env_extra, label):
    """Runs scarab headless with a fake per-user folder; True if the script reported OK."""
    env = dict(os.environ, **user_data_base(home)[1])
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


def expect(condition, label):
    """Prints a PASS/FAIL line for a check made here, on disk; returns the condition."""
    print(f"[{'PASS' if condition else 'FAIL'}] {label}")
    return condition


def read_text(path):
    """A file's text, or None when there is none."""
    if not os.path.isfile(path):
        return None
    with open(path, encoding="utf-8") as f:
        return f.read()


def check_saves(save_dir, game_dir, label):
    """What the rule suite must leave in the save directory."""
    ok  = expect(read_text(os.path.join(save_dir, game_dir, "append.txt")) == "base+more", label + ": append.txt saved")
    ok &= expect(read_text(os.path.join(save_dir, game_dir, "rplus.txt")) == "abXYef", label + ": rplus.txt saved")
    ok &= expect(read_text(os.path.join(save_dir, "appdir.txt")) == "via APP_DIR", label + ": APP_DIR write under the part after it")
    ok &= expect(read_text(os.path.join(save_dir, "out.txt")) == "hello", label + ": io.output's file saved")
    ok &= expect(not os.path.exists(os.path.join(save_dir, game_dir, "game.txt")), label + ": os.remove removed the save")
    return ok


def snapshot(folder):
    """Every game file's bytes (None for a missing one), to prove the suite changed none of them."""
    contents = {}
    for name in GAME_FILES:
        path = os.path.join(folder, name)
        if os.path.isfile(path):
            with open(path, "rb") as f:
                contents[name] = f.read()
        else:
            contents[name] = None
    return contents


def make_loose_copy(source_dir, work):
    """A launch folder holding a copy of the game under LOOSE_GAME_DIR; returns (launch folder, game folder)."""
    launch = tempfile.mkdtemp(prefix="loose-", dir=work)
    game   = os.path.join(launch, LOOSE_GAME_DIR)
    os.makedirs(game)
    for name in GAME_FILES:
        shutil.copy(os.path.join(source_dir, name), game)
    return launch, game


def make_zip(source_dir, zip_path):
    """A plain archive of the game's files, at its root (entry names always use '/')."""
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name in GAME_FILES:
            archive.write(os.path.join(source_dir, name), name)


def run_archive(scarab, archive, work, home, label):
    """The rule suite from an archive, from an empty folder holding outside.txt."""
    empty    = tempfile.mkdtemp(prefix="run-", dir=work)
    save_dir = os.path.join(work, "saves-" + os.path.basename(empty))

    with open(os.path.join(empty, "outside.txt"), "w", encoding="utf-8") as f:
        f.write("not the game's\n")

    ok  = run(scarab, ["--save-dir", save_dir, archive], empty, home,
              {"SMOKE_SCENARIO": "rule", "SMOKE_GAME_DIR": "", "SMOKE_APPDIR_FILE": ZIP_APPDIR_FILE, "SMOKE_ZIP": "1"},
              label)
    ok &= check_saves(save_dir, "", label)
    return ok


def main():
    args  = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = set(a for a in sys.argv[1:] if a.startswith("--"))

    if len(args) != 1 or not flags <= {"--require-encrypted"}:
        print(__doc__)
        return 2

    scarab     = os.path.abspath(args[0])
    repo_root  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    source_dir = os.path.join(repo_root, SCRIPT_DIR)
    before     = snapshot(source_dir)
    ok         = True

    with tempfile.TemporaryDirectory(prefix="scarab-sandbox-smoke-") as work:
        home = os.path.join(work, "home")

        # loose
        launch, game = make_loose_copy(source_dir, work)
        game_before  = snapshot(game)
        save_dir     = os.path.join(work, "loose-saves")
        ok &= run(scarab, ["--save-dir", save_dir, LOOSE_GAME_DIR + "project.json"], launch, home,
                  {"SMOKE_SCENARIO": "rule", "SMOKE_GAME_DIR": LOOSE_GAME_DIR, "SMOKE_APPDIR_FILE": LOOSE_APPDIR_FILE},
                  "loose, the path rule")
        ok &= check_saves(save_dir, LOOSE_GAME_DIR, "loose")
        ok &= expect(snapshot(game) == game_before, "loose: no game file was changed")

        # zip, and encrypted
        plain_zip = os.path.join(work, "game.zip")
        make_zip(source_dir, plain_zip)
        ok &= run_archive(scarab, plain_zip, work, home, "plain .zip, the path rule")

        if "--require-encrypted" in flags:
            encrypted_zip = os.path.join(work, "game-encrypted.zip")
            pack_source   = os.path.join(work, "pack-source")
            pack_config   = os.path.join(work, "pack-config.json")

            os.makedirs(pack_source)
            for name in GAME_FILES:
                shutil.copy(os.path.join(source_dir, name), pack_source)
            with open(pack_config, "w", encoding="utf-8") as f:
                json.dump({"source_dir": pack_source, "output": encrypted_zip}, f)

            result = subprocess.run([scarab, "--pack", pack_config], cwd=work, capture_output=True, text=True,
                                    timeout=RUN_TIMEOUT_SECONDS)
            if expect(result.returncode == 0 and os.path.isfile(encrypted_zip), "encrypted pack built"):
                ok &= run_archive(scarab, encrypted_zip, work, home, "ENCRYPTED pack, the path rule")
            else:
                print((result.stdout or "") + (result.stderr or ""))
                ok = False

        # nodir
        launch, _ = make_loose_copy(source_dir, work)
        ok &= run(scarab, [LOOSE_GAME_DIR + "project.json"], launch, home,
                  {"SMOKE_SCENARIO": "nodir", "SMOKE_GAME_DIR": LOOSE_GAME_DIR}, "nodir, no save directory")

        # switch
        launch, _   = make_loose_copy(source_dir, work)
        switch_home = os.path.join(work, "switch-home")
        base        = user_data_base(switch_home)[0]
        ok &= run(scarab, [LOOSE_GAME_DIR + "switch.json"], launch, switch_home,
                  {"SMOKE_SCENARIO": "switch"}, "switch, the save folder changes")
        ok &= expect(read_text(os.path.join(base, SWITCH_FOLDERS[0], "switch.txt")) == "a", "switch: the first folder kept its file")
        ok &= expect(read_text(os.path.join(base, SWITCH_FOLDERS[1], "switch.txt")) == "b", "switch: the second folder has its own")

        # nosandbox
        launch, _ = make_loose_copy(source_dir, work)
        ok &= run(scarab, ["--no-sandbox", LOOSE_GAME_DIR + "project.json"], launch, home,
                  {"SMOKE_SCENARIO": "nosandbox", "SMOKE_GAME_DIR": LOOSE_GAME_DIR}, "--no-sandbox, Lua's own functions")

    ok &= expect(snapshot(source_dir) == before, "the repo's own copy of the game was not touched")

    print("sandbox smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
