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
Verifies every Lua-callable primitive registered via lua_register() in
src/lua/*api.cpp has a matching documentation heading in docs/lua-api/*.md.

This is Phase 1 of the "Lua API docs" plan (see project memory) - a cheap,
mechanical CI gate against the exact failure mode it exists to catch: a new
or renamed Lua primitive shipping with no doc entry at all. It deliberately
does NOT check prose/example quality, only that `name(` appears *somewhere*
in the primitive's mapped docs/lua-api/*.md file - see CLAUDE.md's own
"Whenever a new Lua-callable primitive is added..." rule, which this makes
mechanically enforced instead of relying on whoever wrote the change
remembering it by hand.
"""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_LUA_DIR = REPO_ROOT / "src" / "lua"
DOCS_DIR = REPO_ROOT / "docs" / "lua-api"

# Explicit source-file -> doc-file mapping. Deliberately NOT auto-derived
# from filenames (e.g. by stripping "lua"/"api.cpp") - two real exceptions
# exist today: luafilesystemapi.cpp's one primitive (dofile) is folded into
# scripting.md rather than getting its own filesystem.md, and
# luatimerapi.cpp maps to the plural timers.md, not timer.md. A naming-
# convention guess would silently mis-map either of those. Add new files
# here explicitly when a new src/lua/*api.cpp file is added - a KeyError
# below (surfaced as a "missing mapping" finding) is the intended failure
# mode for "nobody updated this table yet", not a silent skip.
SOURCE_TO_DOC = {
    "luaappapi.cpp":        "app.md",
    "luacameraapi.cpp":     "camera.md",
    "luacollisionapi.cpp":  "collision.md",
    "luafilesystemapi.cpp": "scripting.md",
    "luainputapi.cpp":      "input.md",
    "luajsonapi.cpp":       "json.md",
    "luascriptingapi.cpp":  "scripting.md",
    "luasoundapi.cpp":      "sound.md",
    "luaspriteapi.cpp":     "sprite.md",
    "luatextapi.cpp":       "text.md",
    "luatilemapapi.cpp":    "tilemap.md",
    "luatimerapi.cpp":      "timers.md",
}

# Matches this codebase's consistent, single-line lua_register() call shape,
# e.g. `lua_register( pLuaState, "app_get_platform", LuaAppApi :: GetPlatform );`
REGISTER_RE = re.compile(r'lua_register\(\s*pLuaState\s*,\s*"([A-Za-z_][A-Za-z0-9_]*)"\s*,')


def find_registrations():
    """Yields (name, source_path, line_no) for every lua_register() call
    under src/lua/*api.cpp, in deterministic (sorted-by-filename) order."""

    for src_file in sorted(SRC_LUA_DIR.glob("*api.cpp")):
        for line_no, line in enumerate(src_file.read_text().splitlines(), start=1):
            match = REGISTER_RE.search(line)
            if match:
                yield match.group(1), src_file, line_no


def is_documented(name, doc_text):
    """Whether `name` appears as a call anywhere in a docs/lua-api/*.md
    file's raw text - a `## `name(...)`` heading (most primitives), a
    `### `name(...)`` sub-heading (e.g. sound.md/tilemap.md's topic-grouped
    sections), or `name(...)` inside a fenced ```lua example (e.g.
    camera.md/input.md/sprite.md's own multi-primitive-per-topic sections).
    Deliberately loose - this only guards against a primitive being entirely
    absent from its doc file, not against prose/example quality, so it has
    to tolerate every heading convention already in real use across these
    12 files rather than assuming one single style."""

    return re.search(r'\b' + re.escape(name) + r'\s*\(', doc_text) is not None


def main():
    missing = []
    doc_text_by_file = {}
    total_checked = 0

    for name, src_file, line_no in find_registrations():
        total_checked += 1
        doc_filename = SOURCE_TO_DOC.get(src_file.name)

        if doc_filename is None:
            missing.append(
                f"{src_file.relative_to(REPO_ROOT)}:{line_no}: '{name}' is "
                f"registered, but {src_file.name} has no entry in this "
                f"script's SOURCE_TO_DOC mapping - add one."
            )
            continue

        if doc_filename not in doc_text_by_file:
            doc_path = DOCS_DIR / doc_filename
            doc_text_by_file[doc_filename] = doc_path.read_text() if doc_path.exists() else ""

        if not is_documented(name, doc_text_by_file[doc_filename]):
            missing.append(
                f"{src_file.relative_to(REPO_ROOT)}:{line_no}: '{name}' is "
                f"registered but '{name}(' doesn't appear anywhere in "
                f"docs/lua-api/{doc_filename}"
            )

    if missing:
        print("Lua API documentation check FAILED:\n", file=sys.stderr)
        for finding in missing:
            print(f"  - {finding}", file=sys.stderr)
        print(
            f"\n{len(missing)} Lua-callable primitive(s) are missing a doc "
            "entry. See CLAUDE.md's \"Whenever a new Lua-callable primitive "
            "is added...\" rule - add a matching entry to the doc file "
            "named above in the same change.",
            file=sys.stderr,
        )
        return 1

    print(f"Lua API documentation check passed ({total_checked} primitives verified).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
