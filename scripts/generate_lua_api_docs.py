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
PILOT / Phase 2-3 of the "Lua API docs" plan (see project memory and
CLAUDE.md) - generates a docs/lua-api/*.md-shaped page straight from the
@luaname/@luagroup/@luadoc/@luaexample tags in each primitive's own C++ doc
comment in src/lua/*api.cpp (plus @luacategory on the owning class's
comment in the matching .h file), instead of the file being hand-written.

This does NOT yet replace docs/lua-api/*.md or wire into doxygen.yml's
publish step - see the plan for why (only luaappapi.cpp/.h are migrated so
far; every other category file still only has hand-written docs, and this
script hard-fails on any registered primitive with no @luaname tag, so
running it un-scoped today would correctly fail on every un-migrated file).
Use --source to scope a run to one or more source files during migration,
e.g.:

    python3 scripts/generate_lua_api_docs.py --source luaappapi.cpp

Tag grammar, read directly from the de-prefixed comment body (NOT via
Doxygen itself - Doxyfile's ALIASES only exist so Doxygen's own unrelated
C++ reference build doesn't warn about unrecognized commands; this script
parses the raw comment text itself):

    @luaname{<lua call signature>}      required on every tagged primitive
    @luagroup{<group key>}              optional; see below
    @luadoc                             starts Lua-facing prose (Markdown),
                                         runs until the next @lua* tag
    @luaexample                         starts a Lua code example, runs
                                         until the next @lua* tag or end of
                                         comment; wrapped in a ```lua fence

A get/set pair documented together (e.g. app_set_fullscreen/
app_get_fullscreen) shares one @luagroup key. Only the first group member
encountered (source order) needs @luadoc/@luaexample - later members
contribute just their @luaname to the combined heading, exactly matching
how docs/lua-api/*.md already documents these pairs today with one shared
prose block.
"""

import argparse
import re
import sys
from pathlib import Path

from _lua_api_shared import DOCS_DIR, REPO_ROOT, SOURCE_TO_DOC, SRC_LUA_DIR, find_registrations

# A category display title per source file, for the generated page's H1 -
# not derivable from the filename alone (e.g. "App", not "Luaapp").
CATEGORY_TITLE_FALLBACK = {
    "luaappapi.cpp": "App",
}

DEFINITION_RE = re.compile(
    r'^\s*int\s+([A-Za-z_]\w*)\s*::\s*([A-Za-z_]\w*)\s*\(\s*lua_State\s*\*\s*\w+\s*\)'
)
CLASS_RE = re.compile(r'^\s*class\s+([A-Za-z_]\w*)\b')
COMMENT_LINE_RE = re.compile(r'^\s*\*\s?(.*)$')
BRACE_TAG_RE = re.compile(r'^@lua(name|group|category)\{([^}]*)\}\s*$')
BARE_TAG_RE = re.compile(r'^@lua(doc|example)\s*$')


class GeneratorError(Exception):
    """A hard-fail condition - e.g. a registered primitive with no
    @luaname tag. Deliberately fatal: a generator that silently skips an
    untagged primitive would just relocate Phase 1's original "forgot to
    document it" risk one layer down instead of closing it."""


def extract_preceding_comment(lines, def_line_idx):
    """Returns the de-prefixed body lines of the /** ... */ block
    immediately preceding `lines[def_line_idx]`, or None if there isn't
    one directly above it (blank lines in between are tolerated)."""

    i = def_line_idx - 1
    while i >= 0 and lines[i].strip() == "":
        i -= 1
    if i < 0 or lines[i].strip() != "*/":
        return None

    body_raw = []
    i -= 1
    while i >= 0:
        stripped = lines[i].strip()
        if stripped == "/**":
            body_raw.reverse()
            return [COMMENT_LINE_RE.match(l).group(1) if COMMENT_LINE_RE.match(l) else l.strip()
                    for l in body_raw]
        body_raw.append(lines[i])
        i -= 1
    return None  # no matching "/**" found


def parse_lua_tags(doc_lines):
    """Parses @luaname/@luagroup/@luacategory/@luadoc/@luaexample out of a
    de-prefixed comment body. Returns a dict; luadoc/luaexample are None if
    the tag wasn't present at all (distinct from present-but-empty)."""

    result = {"luaname": None, "luagroup": None, "luacategory": None,
              "luadoc": None, "luaexample": None}
    collecting = None  # None | "luadoc" | "luaexample"
    collected = {"luadoc": [], "luaexample": []}

    for raw_line in doc_lines:
        line = raw_line.strip()

        brace_match = BRACE_TAG_RE.match(line)
        if brace_match:
            collecting = None
            result["lua" + brace_match.group(1)] = brace_match.group(2)
            continue

        bare_match = BARE_TAG_RE.match(line)
        if bare_match:
            collecting = "lua" + bare_match.group(1)
            continue

        if collecting:
            collected[collecting].append(raw_line)

    if collected["luadoc"]:
        result["luadoc"] = "\n".join(collected["luadoc"]).strip("\n")
    if collected["luaexample"]:
        result["luaexample"] = "\n".join(collected["luaexample"]).strip("\n")

    return result


def find_class_category(header_path):
    """Scans a src/lua/*api.h file for a `class X { ... }` whose preceding
    comment carries @luacategory/@luadoc - returns (title, intro_markdown)
    or (None, None) if not tagged yet (an un-migrated file)."""

    if not header_path.exists():
        return None, None

    lines = header_path.read_text().splitlines()

    for idx, line in enumerate(lines):
        if not CLASS_RE.match(line):
            continue
        doc_lines = extract_preceding_comment(lines, idx)
        if doc_lines is None:
            continue
        tags = parse_lua_tags(doc_lines)
        if tags["luacategory"]:
            return tags["luacategory"], tags["luadoc"]

    return None, None


def collect_primitives(src_file):
    """Returns an ordered list of dicts (one per lua_register()'d
    primitive in `src_file`, in source-definition order), each with
    name/class_name/method_name/luaname/luagroup/luadoc/luaexample/line_no.
    Raises GeneratorError if a registered primitive has no matching
    definition, or a definition with no @luaname tag."""

    registrations = {
        (class_name, method_name): (name, line_no)
        for name, class_name, method_name, _f, line_no in find_registrations([src_file])
    }

    lines = src_file.read_text().splitlines()
    primitives = []
    seen = set()

    for idx, line in enumerate(lines):
        definition_match = DEFINITION_RE.match(line)
        if not definition_match:
            continue

        class_name, method_name = definition_match.group(1), definition_match.group(2)
        key = (class_name, method_name)
        if key not in registrations:
            continue  # a helper method with this shape but never lua_register()'d

        registered_name, register_line_no = registrations[key]
        seen.add(key)

        doc_lines = extract_preceding_comment(lines, idx)
        tags = parse_lua_tags(doc_lines) if doc_lines is not None else {}

        if not tags.get("luaname"):
            raise GeneratorError(
                f"{src_file.relative_to(REPO_ROOT)}:{idx + 1}: "
                f"'{class_name} :: {method_name}' is registered as "
                f"'{registered_name}' ({src_file.relative_to(REPO_ROOT)}:"
                f"{register_line_no}) but has no @luaname tag in its own "
                f"doc comment - add one before this can be generated."
            )

        primitives.append({
            "registered_name": registered_name,
            "class_name": class_name,
            "method_name": method_name,
            "luaname": tags["luaname"],
            "luagroup": tags.get("luagroup"),
            "luadoc": tags.get("luadoc"),
            "luaexample": tags.get("luaexample"),
        })

    missing_definitions = set(registrations) - seen
    if missing_definitions:
        offenders = ", ".join(f"{c} :: {m}" for c, m in sorted(missing_definitions))
        raise GeneratorError(
            f"{src_file.relative_to(REPO_ROOT)}: registered but no matching "
            f"`int Class :: Method( lua_State *pLuaState )` definition "
            f"found for: {offenders}"
        )

    return primitives


def render_page(title, intro_markdown, src_file, primitives):
    lines = [f"# {title}", "", f"*Implemented in* `{src_file.relative_to(REPO_ROOT)}`.", ""]

    if intro_markdown:
        lines.append(intro_markdown)
        lines.append("")

    idx = 0
    while idx < len(primitives):
        primary = primitives[idx]
        group = primary["luagroup"]
        members = [primary]
        idx += 1

        if group:
            while idx < len(primitives) and primitives[idx]["luagroup"] == group:
                members.append(primitives[idx])
                idx += 1

        heading = " / ".join(f"`{m['luaname']}`" for m in members)
        lines.append(f"## {heading}")
        lines.append("")

        if primary["luadoc"]:
            lines.append(primary["luadoc"])
            lines.append("")

        if primary["luaexample"]:
            lines.append("```lua")
            lines.append(primary["luaexample"])
            lines.append("```")
            lines.append("")

    return "\n".join(lines).rstrip("\n") + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source", action="append", metavar="FILENAME",
                         help="Scope generation to this src/lua/*api.cpp basename "
                              "(repeatable). Default: every file in SOURCE_TO_DOC.")
    parser.add_argument("--out-dir", type=Path, default=None,
                         help="Write generated pages here instead of printing to "
                              "stdout (one file per category, named like the "
                              "mapped docs/lua-api/*.md file).")
    args = parser.parse_args()

    source_names = args.source if args.source else sorted(SOURCE_TO_DOC)

    for source_name in source_names:
        if source_name not in SOURCE_TO_DOC:
            print(f"error: '{source_name}' has no SOURCE_TO_DOC mapping", file=sys.stderr)
            return 1

        src_file = SRC_LUA_DIR / source_name
        header_file = src_file.with_suffix(".h")

        try:
            primitives = collect_primitives(src_file)
        except GeneratorError as err:
            print(f"Lua API doc generation FAILED:\n\n  - {err}\n", file=sys.stderr)
            return 1

        title, intro = find_class_category(header_file)
        if title is None:
            title = CATEGORY_TITLE_FALLBACK.get(source_name, source_name)

        page = render_page(title, intro, src_file, primitives)

        if args.out_dir:
            args.out_dir.mkdir(parents=True, exist_ok=True)
            out_path = args.out_dir / SOURCE_TO_DOC[source_name]
            out_path.write_text(page)
            print(f"wrote {out_path.relative_to(REPO_ROOT) if REPO_ROOT in out_path.parents else out_path}")
        else:
            print(f"--- {SOURCE_TO_DOC[source_name]} (from {source_name}) ---\n")
            print(page)

    return 0


if __name__ == "__main__":
    sys.exit(main())
