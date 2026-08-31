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
Shared scanning helpers for scripts/check_lua_api_docs.py (Phase 1) and
scripts/generate_lua_api_docs.py (Phase 2/3 pilot) - both need the same
"which src/lua/*api.cpp file maps to which docs/lua-api/*.md category" and
"find every lua_register() call" logic, and keeping that in one place is the
whole point of either script existing (a mapping table that drifted between
two copies would defeat the purpose).
"""

import re
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
# here explicitly when a new src/lua/*api.cpp file is added.
SOURCE_TO_DOC = {
    "luaappapi.cpp":        "app.md",
    "luacameraapi.cpp":     "camera.md",
    "luacollisionapi.cpp":  "collision.md",
    "luaengine.cpp":        "callbacks.md",
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

# luaengine.cpp is the one file scanned for the REVERSE direction - Lua
# globals the engine calls INTO (lua_getglobal), not primitives Lua calls
# into the engine (lua_register). Its own dispatch methods (CallOnUpdate,
# TryDispatchLoadStage, ...) don't share lua_register()'s call shape at
# all, so generate_lua_api_docs.py's collect_callback_primitives() scans
# it separately - this constant just marks which SOURCE_TO_DOC entry that
# applies to, so find_registrations() (the lua_register()-only scanner)
# is never asked to look at it.
CALLBACK_SOURCE_FILE = "luaengine.cpp"

# Matches this codebase's consistent, single-line lua_register() call shape,
# e.g. `lua_register( pLuaState, "app_get_platform", LuaAppApi :: GetPlatform );`
REGISTER_RE = re.compile(
    r'lua_register\(\s*pLuaState\s*,\s*"([A-Za-z_][A-Za-z0-9_]*)"\s*,\s*'
    r'([A-Za-z_]\w*)\s*::\s*([A-Za-z_]\w*)'
)


def find_registrations(src_files=None):
    """Yields (name, class_name, method_name, source_path, line_no) for
    every lua_register() call under src/lua/*api.cpp (or just `src_files`,
    if given), in deterministic (sorted-by-filename) order."""

    files = src_files if src_files is not None else sorted(SRC_LUA_DIR.glob("*api.cpp"))

    for src_file in files:
        for line_no, line in enumerate(src_file.read_text().splitlines(), start=1):
            match = REGISTER_RE.search(line)
            if match:
                yield match.group(1), match.group(2), match.group(3), src_file, line_no
