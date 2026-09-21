#!/usr/bin/env python3
"""
Regression test: a map with EXTERNAL tilesets (.tsx) must load from a packed
project, with no loose copy of anything reachable on disk.

Background (found 2026-09-21, fixed in sunlight v0.31.0): libtmx opened
external tilesets/templates straight from the OS path instead of through
SunLight::FileSystem, so a game shipped only as a .zip (or an encrypted
pack) could not load any map that referenced a .tsx - "cannot open extern
tileset ...". A normal build hid it, since loose resources sit next to the
executable - the same "loose files mask the gap" pattern as the BMFont
bugs in CLAUDE.md.

What this does, cross-platform (pure Python: `zip` doesn't exist on Windows):
  1. builds a tiny project from this repo's own resources/tilemap (test.tmx
     plus its 8 sibling .tsx files),
  2. packs it as a plain .zip and runs it headless from an EMPTY directory,
  3. with --require-encrypted, also packs it with `scarab --pack` (needs a
     build configured with SCARAB_CONTENT_KEY - ci.yml's are) and runs the
     ENCRYPTED archive the same way, which additionally needs the read
     filter to apply to the .tsx files.

The project's script loads the map and quits ITSELF only if it loaded (with
the expected 20x20 size); otherwise it never quits, so --max-frames runs out
and scarab exits 3 (EXIT_FRAME_BUDGET_EXHAUSTED) - a failure with the
scarab log attached, not a hang.

Usage: tileset_archive_smoke.py <path-to-scarab-executable> [--require-encrypted]
"""

import json
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

EXPECTED_MAP_WIDTH  = 20
EXPECTED_MAP_HEIGHT = 20
MAX_FRAMES          = 300
RUN_TIMEOUT_SECONDS = 120

PROJECT_JSON = '{ "main_script": "main.lua" }\n'

# APP_DIR-anchored: a mounted archive is searched at the same virtual
# location as the executable's own directory, so this resolves into the
# archive exactly like a loose folder.
MAIN_LUA = f"""io.stdout:setvbuf( "line" )

local frames = 0

function on_load_stage( stageId )
    return true
end

function on_update( dt )
    frames = frames + 1

    if frames == 1 then
        local loaded = tilemap_load_map( APP_DIR .. "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT )
        local w, h   = 0, 0

        if loaded then
            w, h = tilemap_get_map_info()
        end

        local ok = loaded and w == {EXPECTED_MAP_WIDTH} and h == {EXPECTED_MAP_HEIGHT}

        print( string.format( "TILESET SMOKE %s: loaded=%s map=%sx%s", ok and "OK" or "FAIL", tostring( loaded ), tostring( w ), tostring( h ) ) )

        if ok then
            app_quit()
        end
    end
end

sp_wait( 1 )
"""


def build_source_dir(source_dir, repo_root):
    """Writes the tiny project (script + this repo's own tilemap resources)."""
    os.makedirs(source_dir)

    with open(os.path.join(source_dir, "project.json"), "w", encoding="utf-8", newline="\n") as f:
        f.write(PROJECT_JSON)

    with open(os.path.join(source_dir, "main.lua"), "w", encoding="utf-8", newline="\n") as f:
        f.write(MAIN_LUA)

    shutil.copytree(os.path.join(repo_root, "resources", "tilemap"),
                    os.path.join(source_dir, "resources", "tilemap"))


def make_plain_zip(source_dir, zip_path):
    """A plain (unencrypted) archive - entry names always use '/'."""
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for root, _dirs, files in os.walk(source_dir):
            for name in files:
                full = os.path.join(root, name)
                archive.write(full, os.path.relpath(full, source_dir).replace(os.sep, "/"))


def run(cmd, cwd):
    """Runs scarab; returns (exit code, combined output)."""
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True,
                            timeout=RUN_TIMEOUT_SECONDS)
    return result.returncode, (result.stdout or "") + (result.stderr or "")


def run_archive(scarab, archive, empty_dir, label):
    """Runs an archive headless from an EMPTY directory; True on success."""
    code, output = run([scarab, "--headless", "--fast", "--max-frames", str(MAX_FRAMES), archive],
                       cwd=empty_dir)

    smoke = [line for line in output.splitlines() if line.startswith("TILESET SMOKE")]
    ok    = (code == 0) and any(" OK:" in line for line in smoke)

    print(f"[{'PASS' if ok else 'FAIL'}] {label}: exit={code} {smoke[-1] if smoke else '(no TILESET SMOKE line)'}")

    if not ok:
        # exit 3 = frame budget ran out = the script never got its map loaded
        print("---- scarab output (last 25 lines) ----")
        print("\n".join(output.splitlines()[-25:]))
        print("----------------------------------------")

    return ok


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = {a for a in sys.argv[1:] if a.startswith("--")}

    if len(args) != 1 or not flags <= {"--require-encrypted"}:
        print(__doc__)
        return 2

    scarab     = os.path.abspath(args[0])
    repo_root  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    all_passed = True

    with tempfile.TemporaryDirectory(prefix="scarab-tileset-smoke-") as work:
        source_dir = os.path.join(work, "source")
        empty_dir  = os.path.join(work, "empty")       # cwd for every run: nothing loose reachable
        os.makedirs(empty_dir)
        build_source_dir(source_dir, repo_root)

        plain_zip = os.path.join(work, "plain.zip")
        make_plain_zip(source_dir, plain_zip)
        all_passed &= run_archive(scarab, plain_zip, empty_dir, "plain .zip, external .tsx tilesets")

        if "--require-encrypted" in flags:
            encrypted_zip = os.path.join(work, "encrypted.zip")
            pack_config   = os.path.join(work, "pack-config.json")

            with open(pack_config, "w", encoding="utf-8") as f:
                json.dump({"source_dir": source_dir, "output": encrypted_zip}, f)

            code, output = run([scarab, "--pack", pack_config], cwd=empty_dir)

            if code != 0 or not os.path.isfile(encrypted_zip):
                print(f"[FAIL] encrypted pack: `scarab --pack` exited {code} and did not produce the archive")
                print(output[-2000:])
                all_passed = False
            else:
                all_passed &= run_archive(scarab, encrypted_zip, empty_dir, "ENCRYPTED pack, external .tsx tilesets")

    print("tileset archive smoke test:", "PASSED" if all_passed else "FAILED")
    return 0 if all_passed else 1


if __name__ == "__main__":
    sys.exit(main())
