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
Smoke test for the input constants Lua sees (KEY_*, GAMEPAD_BUTTON_*, GAMEPAD_AXIS_*, CONTROLLER_*).

Two parts:

1. Coverage, before running anything: every constant of sunlight's input enums (KeyboardKey,
   GamepadButton, GamepadAxis, ControllerType in the fetched sunlight's
   src/input/iinputhandler.h) must be in src/lua/luainputapi.cpp's matching array, and that
   array must hold nothing else - so a key sunlight adds and Scarab forgets fails CI. The
   header is found in the FetchContent checkout (build/_deps/sunlight-src by default, or
   --sunlight-src DIR, eg. when configured with FETCHCONTENT_SOURCE_DIR_SUNLIGHT); if it is
   not there the test fails, it never silently skips this part.
2. Values, headless: scripts/input_constants_smoke/main.lua gets the names from Scarab's
   arrays and checks every constant exists, no two in a group share a value, and KEY_MENU is 5
   (raylib's), not 82 - which up to sunlight v0.38.0 was also KEY_R.

See the Lua script's own header, and doc/MASTER_PLAN.md item V4.

The script quits itself only if every check passed (exit 0); otherwise it prints each
failure and never quits, so --max-frames runs out and scarab exits 3.

Usage: input_constants_smoke.py <path-to-scarab-executable> [--sunlight-src DIR]
"""

import argparse
import os
import re
import subprocess
import sys

MAX_FRAMES          = 200
RUN_TIMEOUT_SECONDS = 120
PROJECT             = "scripts/input_constants_smoke/project.json"
OK_MARKER           = "INPUT CONSTANTS SMOKE OK"
SOURCE              = "src/lua/luainputapi.cpp"
SUNLIGHT_DEFAULT    = "build/_deps/sunlight-src"
SUNLIGHT_HEADER     = "src/input/iinputhandler.h"
# (array in SOURCE, sunlight enum it mirrors, prefix every constant has) - in the order the
# Lua script expects its groups
ARRAYS              = [("s_aKeyboardKeys", "KeyboardKey", "KEY_"),
                       ("s_aGamepadButtons", "GamepadButton", "GAMEPAD_BUTTON_"),
                       ("s_aGamepadAxis", "GamepadAxis", "GAMEPAD_AXIS_"),
                       ("s_aControllerTypes", "ControllerType", "CONTROLLER_")]


def constant_names(source_text, array, prefix):
    """The constant names inside one `static const stNamedConstant <array>[] = { ... };`."""
    match = re.search(r"\b" + array + r"\[\]\s*=\s*\{(.*?)\};", source_text, re.S)
    if not match:
        raise SystemExit(f"cannot find array {array} in {SOURCE}")
    names = re.findall(r"\b(" + prefix + r"[A-Z0-9_]+)\b", match.group(1))
    if not names:
        raise SystemExit(f"array {array} in {SOURCE} has no {prefix}* constants")
    return list(dict.fromkeys(names))      # keep source order, drop repeats inside one entry


def enum_names(header_text, enum, prefix):
    """The enumerator names of one `enum <enum> { ... };` in sunlight's header."""
    match = re.search(r"\benum\s+" + enum + r"\s*\{(.*?)\};", header_text, re.S)
    if not match:
        raise SystemExit(f"cannot find enum {enum} in sunlight's {SUNLIGHT_HEADER}")
    body  = re.sub(r"//[^\n]*", "", match.group(1))       # the comments name other keys
    names = re.findall(r"\b(" + prefix + r"[A-Z0-9_]+)\b", body)
    if not names:
        raise SystemExit(f"enum {enum} in sunlight's {SUNLIGHT_HEADER} has no {prefix}* enumerators")
    return list(dict.fromkeys(names))


def check_coverage(groups, header_text):
    """Every sunlight enumerator is registered by Scarab, and Scarab registers nothing else."""
    ok = True
    for names, (array, enum, prefix) in zip(groups, ARRAYS):
        expected = enum_names(header_text, enum, prefix)
        missing  = [n for n in expected if n not in names]
        extra    = [n for n in names if n not in expected]
        print(f"[{'PASS' if not missing and not extra else 'FAIL'}] coverage {prefix}*: "
              f"sunlight {enum} {len(expected)}, Scarab {array} {len(names)}")
        if missing:
            print(f"    missing in Scarab ({SOURCE} {array}): {', '.join(missing)}")
        if extra:
            print(f"    not in sunlight's {enum}: {', '.join(extra)}")
        ok &= not missing and not extra
    return ok


def main():
    parser = argparse.ArgumentParser(description="Input constants smoke test (see the module docstring).")
    parser.add_argument("scarab", help="path to the scarab executable")
    parser.add_argument("--sunlight-src", help=f"sunlight's source checkout (default: {SUNLIGHT_DEFAULT})")
    args = parser.parse_args()

    scarab    = os.path.abspath(args.scarab)
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    sunlight  = os.path.abspath(args.sunlight_src or os.path.join(repo_root, SUNLIGHT_DEFAULT))
    header    = os.path.join(sunlight, SUNLIGHT_HEADER)

    with open(os.path.join(repo_root, SOURCE), encoding="utf-8") as f:
        source_text = f.read()

    if not os.path.isfile(header):
        print(f"[FAIL] cannot find sunlight's {SUNLIGHT_HEADER} under {sunlight} - configure first, "
              "or pass --sunlight-src DIR")
        print("input constants smoke test: FAILED")
        return 1

    with open(header, encoding="utf-8") as f:
        header_text = f.read()

    groups   = [constant_names(source_text, array, prefix) for array, _, prefix in ARRAYS]
    covered  = check_coverage(groups, header_text)
    env      = dict(os.environ, INPUT_SMOKE_NAMES=";".join(",".join(g) for g in groups))

    result = subprocess.run([scarab, "--headless", "--fast", "--max-frames", str(MAX_FRAMES), PROJECT],
                            cwd=repo_root, env=env, capture_output=True, text=True,
                            timeout=RUN_TIMEOUT_SECONDS)
    output = (result.stdout or "") + (result.stderr or "")
    ran    = result.returncode == 0 and OK_MARKER in output
    ok     = covered and ran

    print(f"[{'PASS' if ran else 'FAIL'}] input constants values: exit={result.returncode}")

    if not ran:
        print("---- scarab output (last 30 lines) ----")
        print("\n".join(output.splitlines()[-30:]))
        print("---------------------------------------")

    print("input constants smoke test:", "PASSED" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
