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
@luaname/@luagroup/@luaheading/@luadoc/@luaexample tags in each primitive's
own C++ doc comment in src/lua/*api.cpp (plus @luacategory/@luadoc/
@luaoutro on the owning class's comment in the matching .h file), instead
of the file being hand-written.

This does NOT yet replace docs/lua-api/*.md or wire into doxygen.yml's
publish step - see the plan for why. It hard-fails on any registered
primitive with no @luaname tag, so running it against a not-yet-migrated
file correctly fails rather than silently skipping. Use --source to scope
a run to one or more source files during migration, e.g.:

    python3 scripts/generate_lua_api_docs.py --source luaappapi.cpp

Tag grammar, read directly from the de-prefixed comment body (NOT via
Doxygen itself - Doxyfile's ALIASES only exist so Doxygen's own unrelated
C++ reference build doesn't warn about unrecognized commands; this script
parses the raw comment text itself):

    @luaname{<lua call signature>}      required on every tagged primitive
    @luagroup{<group key>}              optional; see below
    @luaheading{<heading text>}         optional override for a group's
                                         rendered heading (see below)
    @luadoc                             starts Lua-facing prose (Markdown),
                                         runs until the next @lua* tag
    @luaexample                         starts a Lua code example, runs
                                         until the next @lua* tag or end of
                                         comment; wrapped in a ```lua fence
    @luaoutro                           class-level only (on the .h file's
                                         class comment, alongside
                                         @luacategory/@luadoc) - trailing
                                         page content appended AFTER every
                                         primitive section (e.g. timers.md's
                                         "Thread safety" section, which
                                         isn't about any one primitive)

A get/set pair documented together (e.g. app_set_fullscreen/
app_get_fullscreen) shares one @luagroup key. Only the first group member
encountered (source order) needs @luadoc/@luaexample/@luaheading - later
members contribute just their @luaname. By default the rendered heading is
every member's @luaname joined with " / "; @luaheading on the primary
overrides that entirely with arbitrary text (e.g. sound.md's "Song
commands — queued vs. direct", covering 9 members under one heading with a
comparison table, not a list of 9 joined signatures).

Every docs/lua-api/*.md file maps to exactly one or more src/lua/*api.cpp
files via SOURCE_TO_DOC (see _lua_api_shared.py) - most map 1:1, but
scripting.md is fed by BOTH luascriptingapi.cpp and luafilesystemapi.cpp
(dofile). This script groups by DOC FILE, not by source file: it gathers
every contributing source's primitives onto one page, in source order
with the @luacategory-carrying "primary" source's primitives first,
followed by any other contributor(s) - the primary's own @luacategory/
@luadoc/@luaoutro become the page's title/intro/outro.

Deliberate simplification, not a bug: this only ever emits a flat,
single-level heading per primitive/group (matching how most of these
files already read) - it does not model a heading with its own nested
sub-headings underneath it (e.g. sound.md's "Loading and direct playback
state" parent with ### sound_load()/etc. children, or tilemap.md's
"Layers"). Migrating those flattens that one parent/child relationship
into independent top-level headings; the content itself carries over
unchanged either way.
"""

import argparse
import re
import sys
from pathlib import Path

from _lua_api_shared import (
    CALLBACK_SOURCE_FILE,
    DOCS_DIR,
    REPO_ROOT,
    SOURCE_TO_DOC,
    SRC_LUA_DIR,
    find_registrations,
)

# A category display title per source file, for the generated page's H1 -
# not derivable from the filename alone (e.g. "App", not "Luaapp"). Only
# needed as a fallback for a source file whose class has no @luacategory
# tag yet (an un-migrated file) - every migrated file's real title comes
# from @luacategory itself.
CATEGORY_TITLE_FALLBACK = {
    "luaappapi.cpp": "App",
}

DEFINITION_RE = re.compile(
    r'^\s*int\s+([A-Za-z_]\w*)\s*::\s*([A-Za-z_]\w*)\s*\(\s*lua_State\s*\*\s*\w+\s*\)'
)
CLASS_RE = re.compile(r'^\s*class\s+([A-Za-z_]\w*)\b')
COMMENT_LINE_RE = re.compile(r'^\s*\*\s?(.*)$')
BRACE_TAG_RE = re.compile(r'^@lua(name|group|category|heading)\{([^}]*)\}\s*$')
BARE_TAG_RE = re.compile(r'^@lua(doc|example|outro)\s*$')


class GeneratorError(Exception):
    """A hard-fail condition - e.g. a registered primitive with no
    @luaname tag. Deliberately fatal: a generator that silently skips an
    untagged primitive would just relocate Phase 1's original "forgot to
    document it" risk one layer down instead of closing it."""


class DuplicateTagError(Exception):
    """Raised by parse_lua_tags() when a block tag (@luadoc/@luaexample/
    @luaoutro) appears more than once in the same comment - found live
    while migrating luatextapi.cpp's DrawText: a second @luadoc after an
    @luaexample silently merged into the first one's collected text
    instead of raising, reordering the actual page output (both @luadoc
    blocks ended up concatenated *before* the example, not interleaved
    around it as intended) rather than erroring. Each block tag may only
    be used once per primitive - put everything (including a fenced
    ```lua example, if it needs to sit *between* two paragraphs) inside
    one @luadoc instead."""

    def __init__(self, tag):
        super().__init__(f"'@{tag}' appears more than once in the same comment block")
        self.tag = tag


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
    """Parses @luaname/@luagroup/@luacategory/@luaheading/@luadoc/
    @luaexample/@luaoutro out of a de-prefixed comment body. Returns a
    dict; the block tags (luadoc/luaexample/luaoutro) are None if the tag
    wasn't present at all (distinct from present-but-empty)."""

    result = {"luaname": None, "luagroup": None, "luacategory": None, "luaheading": None,
              "luadoc": None, "luaexample": None, "luaoutro": None}
    collecting = None  # None | "luadoc" | "luaexample" | "luaoutro"
    collected = {"luadoc": [], "luaexample": [], "luaoutro": []}
    already_seen = set()

    for raw_line in doc_lines:
        line = raw_line.strip()

        brace_match = BRACE_TAG_RE.match(line)
        if brace_match:
            collecting = None
            result["lua" + brace_match.group(1)] = brace_match.group(2)
            continue

        bare_match = BARE_TAG_RE.match(line)
        if bare_match:
            tag = "lua" + bare_match.group(1)
            if tag in already_seen:
                raise DuplicateTagError(tag)
            already_seen.add(tag)
            collecting = tag
            continue

        if collecting:
            collected[collecting].append(raw_line)

    for key in ("luadoc", "luaexample", "luaoutro"):
        if collected[key]:
            result[key] = "\n".join(collected[key]).strip("\n")

    return result


def find_class_category(header_path):
    """Scans a src/lua/*api.h file for a `class X { ... }` whose preceding
    comment carries @luacategory - returns a dict with title/intro/outro,
    or None if not tagged yet (an un-migrated file)."""

    if not header_path.exists():
        return None

    lines = header_path.read_text().splitlines()

    for idx, line in enumerate(lines):
        if not CLASS_RE.match(line):
            continue
        doc_lines = extract_preceding_comment(lines, idx)
        if doc_lines is None:
            continue
        try:
            tags = parse_lua_tags(doc_lines)
        except DuplicateTagError as err:
            raise GeneratorError(f"{header_path.relative_to(REPO_ROOT)}:{idx + 1}: {err}")
        if tags["luacategory"]:
            return {"title": tags["luacategory"], "intro": tags["luadoc"], "outro": tags["luaoutro"]}

    return None


def collect_primitives(src_file):
    """Returns an ordered list of dicts (one per lua_register()'d
    primitive in `src_file`, in source-definition order), each with
    name/class_name/method_name/luaname/luagroup/luaheading/luadoc/
    luaexample/line_no. Raises GeneratorError if a registered primitive
    has no matching definition, or a definition with no @luaname tag."""

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
        try:
            tags = parse_lua_tags(doc_lines) if doc_lines is not None else {}
        except DuplicateTagError as err:
            raise GeneratorError(f"{src_file.relative_to(REPO_ROOT)}:{idx + 1}: {err}")

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
            "luaheading": tags.get("luaheading"),
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


# luaengine.cpp's reverse-direction scan: it has NO lua_register() calls at
# all (that's the whole point - these are globals the engine looks up and
# calls INTO Lua, via lua_getglobal, not primitives Lua calls into the
# engine). GENERIC_DEFINITION_RE is deliberately looser than DEFINITION_RE
# above - a dispatch method like `void LuaEngine :: CallOnUpdate( uint32_t
# nDeltaMilli )` shares no signature shape with `int Class :: Method(
# lua_State *pLuaState )` at all (different return type, no lua_State*
# parameter), and this also has to match the constructor/destructor forms
# (`LuaEngine :: LuaEngine(...)`, `LuaEngine :: ~LuaEngine(...)`) so their
# bodies are correctly excluded as "no lua_getglobal call inside" rather
# than mis-attributing some other method's callback to them.
GENERIC_DEFINITION_RE = re.compile(r'^\s*(?:[\w:&*<>]+\s+)*LuaEngine\s*::\s*(~?\w+)\s*\(')
GETGLOBAL_RE = re.compile(r'lua_getglobal\(\s*\w+\s*,\s*"([A-Za-z_]\w*)"\s*\)')


def collect_callback_primitives(src_file):
    """Like collect_primitives(), but for the reverse direction: scans
    `src_file` (luaengine.cpp) for lua_getglobal("name") calls, resolves
    each to its ENCLOSING LuaEngine method (by body range between one
    method definition and the next), and reads that method's own existing
    doc comment for @luaname/@luadoc/@luaexample - the same tag grammar,
    just anchored differently since there's no lua_register() call to
    anchor to instead. Returns the same list-of-dicts shape
    collect_primitives() does, so render_page() needs no changes at all.
    A LuaEngine method with no lua_getglobal call inside it (RunFile,
    Init, RegisterCalls, the ctor/dtor, ...) is simply not a callback
    dispatcher and is silently skipped - only methods that actually call
    lua_getglobal are candidates for the hard-fail-if-untagged check."""

    lines = src_file.read_text().splitlines()

    definitions = []  # (line_idx, method_name)
    for idx, line in enumerate(lines):
        match = GENERIC_DEFINITION_RE.match(line)
        if match:
            definitions.append((idx, match.group(1)))

    def enclosing_method(line_idx):
        enclosing = None
        for def_idx, name in definitions:
            if def_idx <= line_idx:
                enclosing = (def_idx, name)
            else:
                break
        return enclosing

    # Which method each lua_getglobal("name") call belongs to, in the
    # order the *methods* were defined (not call order) - so the
    # generated page reads in the same order as the source file, same as
    # every other category page.
    dispatchers = {}  # method_name -> (global_name, def_idx)
    for idx, line in enumerate(lines):
        get_match = GETGLOBAL_RE.search(line)
        if not get_match:
            continue
        enclosing = enclosing_method(idx)
        if enclosing is None:
            raise GeneratorError(
                f"{src_file.relative_to(REPO_ROOT)}:{idx + 1}: "
                f"lua_getglobal(\"{get_match.group(1)}\") found with no "
                f"enclosing LuaEngine method - GENERIC_DEFINITION_RE "
                f"didn't match whatever contains this call."
            )
        def_idx, method_name = enclosing
        dispatchers[method_name] = (get_match.group(1), def_idx)

    primitives = []
    for def_idx, method_name in sorted(definitions, key=lambda d: d[0]):
        if method_name not in dispatchers:
            continue  # not a callback-dispatch method - nothing to check

        global_name, _get_idx = dispatchers[method_name]
        doc_lines = extract_preceding_comment(lines, def_idx)
        try:
            tags = parse_lua_tags(doc_lines) if doc_lines is not None else {}
        except DuplicateTagError as err:
            raise GeneratorError(f"{src_file.relative_to(REPO_ROOT)}:{def_idx + 1}: {err}")

        if not tags.get("luaname"):
            raise GeneratorError(
                f"{src_file.relative_to(REPO_ROOT)}:{def_idx + 1}: "
                f"'LuaEngine :: {method_name}' calls lua_getglobal("
                f"\"{global_name}\") but has no @luaname tag in its own "
                f"doc comment - add one before this can be generated."
            )

        primitives.append({
            "registered_name": global_name,
            "class_name": "LuaEngine",
            "method_name": method_name,
            "luaname": tags["luaname"],
            "luagroup": tags.get("luagroup"),
            "luaheading": tags.get("luaheading"),
            "luadoc": tags.get("luadoc"),
            "luaexample": tags.get("luaexample"),
        })

    return primitives


def render_page(category, src_files, primitives):
    impl_list = " and ".join(f"`{f.relative_to(REPO_ROOT)}`" for f in src_files)
    lines = [f"# {category['title']}", "", f"*Implemented in* {impl_list}.", ""]

    if category["intro"]:
        lines.append(category["intro"])
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

        heading = primary["luaheading"] or " / ".join(f"`{m['luaname']}`" for m in members)
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

    if category["outro"]:
        lines.append(category["outro"])
        lines.append("")

    return "\n".join(lines).rstrip("\n") + "\n"


# Bare "name.md" links (docs/lua-api/*.md's own convention throughout,
# e.g. "see scripting.md" or "[tilemap.md](tilemap.md)") need to become
# "name.html" once these pages are actually published as HTML siblings of
# each other, not Markdown files on GitHub. Deliberately narrow - only
# rewrites a target ending in exactly ".md" (never touches ".md" appearing
# mid-word, a URL fragment, or a non-doc-page link) since every one of
# these pages' own cross-references is always a bare same-directory
# filename with no path segments.
MD_LINK_TARGET_RE = re.compile(r'(\]\()([A-Za-z0-9_-]+)\.md(\))')

PAGE_HTML_TEMPLATE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>{title} — Scarab Lua API</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  :root {{ color-scheme: light dark; }}
  body {{
    max-width: 860px;
    margin: 0 auto;
    padding: 24px 20px 64px;
    font: 16px/1.55 -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
  }}
  nav.crumbs {{ font-size: 14px; margin-bottom: 24px; }}
  nav.crumbs a {{ text-decoration: none; }}
  h1 {{ font-size: 1.8em; margin-bottom: 0.2em; }}
  h2 {{ margin-top: 2em; border-bottom: 1px solid light-dark(#ddd, #444); padding-bottom: 0.3em; }}
  code, pre {{ font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace; }}
  code {{ background: light-dark(#f2f2f2, #2a2a2a); padding: 0.1em 0.35em; border-radius: 4px; font-size: 0.9em; }}
  pre {{ background: light-dark(#f2f2f2, #2a2a2a); padding: 12px 14px; border-radius: 6px; overflow-x: auto; }}
  pre code {{ background: none; padding: 0; }}
  table {{ border-collapse: collapse; margin: 1em 0; }}
  th, td {{ border: 1px solid light-dark(#ddd, #444); padding: 6px 10px; text-align: left; }}
  a {{ color: light-dark(#0969da, #58a6ff); }}
</style>
</head>
<body>
<nav class="crumbs"><a href="../index.html">Scarab C++ reference</a> &middot; <a href="index.html">Lua API</a></nav>
{body}
</body>
</html>
"""


def render_page_html(category, src_files, primitives):
    """Same content as render_page(), converted to a standalone HTML page.
    Reuses render_page()'s own Markdown output rather than a second,
    parallel renderer - runs it through the `markdown` package (tables +
    fenced_code extensions, since sound.md's comparison table and every
    page's ```lua examples both need to survive the conversion), then
    rewrites bare "name.md" cross-reference links to "name.html" (these
    pages' own convention throughout) since the published siblings are
    HTML, not Markdown. Lazy-imports `markdown` so plain Markdown-mode
    generation (the default) never requires the dependency at all."""

    import markdown as markdown_lib

    md_text = render_page(category, src_files, primitives)
    md_text = MD_LINK_TARGET_RE.sub(r'\1\2.html\3', md_text)
    body_html = markdown_lib.markdown(md_text, extensions=["tables", "fenced_code"])

    return PAGE_HTML_TEMPLATE.format(title=category["title"], body=body_html)


def doc_file_to_sources(requested_source_names):
    """Groups SOURCE_TO_DOC by doc filename, restricted to doc files that
    have at least one of `requested_source_names` as a contributor.
    Returns an ordered dict: {doc_filename: [source_filename, ...]},
    sources within each list in SOURCE_TO_DOC's own definition order."""

    all_doc_to_sources = {}
    for source_name, doc_filename in SOURCE_TO_DOC.items():
        all_doc_to_sources.setdefault(doc_filename, []).append(source_name)

    wanted_doc_files = {
        doc_filename
        for source_name, doc_filename in SOURCE_TO_DOC.items()
        if source_name in requested_source_names
    }

    return {doc: sources for doc, sources in all_doc_to_sources.items() if doc in wanted_doc_files}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source", action="append", metavar="FILENAME",
                         help="Scope generation to the docs/lua-api page(s) this "
                              "src/lua/*api.cpp basename maps to (repeatable) - "
                              "ALL contributing sources for that page are still "
                              "included (e.g. naming luascriptingapi.cpp also "
                              "pulls in luafilesystemapi.cpp's dofile for "
                              "scripting.md). Default: every file in SOURCE_TO_DOC.")
    parser.add_argument("--out-dir", type=Path, default=None,
                         help="Write generated pages here instead of printing to "
                              "stdout (one file per category, named like the "
                              "mapped docs/lua-api/*.md file).")
    parser.add_argument("--format", choices=["markdown", "html"], default="markdown",
                         help="Output format. 'markdown' (default) matches "
                              "docs/lua-api/*.md's own shape exactly, for local "
                              "fidelity-checking against the hand-written pages. "
                              "'html' is what actually gets published (see the "
                              "doxygen.yml merge) - requires the `markdown` "
                              "package (pip install markdown), only imported "
                              "when this mode is actually used.")
    args = parser.parse_args()

    requested = set(args.source) if args.source else set(SOURCE_TO_DOC)
    for name in requested:
        if name not in SOURCE_TO_DOC:
            print(f"error: '{name}' has no SOURCE_TO_DOC mapping", file=sys.stderr)
            return 1

    doc_to_sources = doc_file_to_sources(requested)

    for doc_filename in sorted(doc_to_sources):
        source_names = doc_to_sources[doc_filename]
        src_files = [SRC_LUA_DIR / name for name in source_names]

        categories = []
        all_primitives = []
        try:
            for src_file in src_files:
                category = find_class_category(src_file.with_suffix(".h"))
                categories.append((src_file, category))
                collector = collect_callback_primitives if src_file.name == CALLBACK_SOURCE_FILE else collect_primitives
                all_primitives.append((src_file, category, collector(src_file)))
        except GeneratorError as err:
            print(f"Lua API doc generation FAILED:\n\n  - {err}\n", file=sys.stderr)
            return 1

        primaries = [(f, c) for f, c in categories if c is not None]
        if not primaries:
            title = CATEGORY_TITLE_FALLBACK.get(source_names[0], source_names[0])
            category = {"title": title, "intro": None, "outro": None}
        elif len(primaries) > 1:
            offenders = ", ".join(f.name for f, _c in primaries)
            print(
                f"Lua API doc generation FAILED:\n\n  - {doc_filename} is fed by "
                f"more than one source with an @luacategory tag ({offenders}) - "
                f"only one contributor to a shared doc page may carry the page's "
                f"title/intro/outro.\n",
                file=sys.stderr,
            )
            return 1
        else:
            category = primaries[0][1]

        # Primary source's primitives first (matches its own @luacategory
        # ownership of the page), then any other contributor(s), each in
        # their own source-definition order.
        primary_file = primaries[0][0] if primaries else src_files[0]
        ordered = sorted(all_primitives, key=lambda item: item[0] != primary_file)
        primitives = [p for _f, _c, prims in ordered for p in prims]

        if args.format == "html":
            page = render_page_html(category, src_files, primitives)
            out_name = Path(doc_filename).with_suffix(".html").name
        else:
            page = render_page(category, src_files, primitives)
            out_name = doc_filename

        if args.out_dir:
            args.out_dir.mkdir(parents=True, exist_ok=True)
            out_path = args.out_dir / out_name
            out_path.write_text(page)
            print(f"wrote {out_path}")
        else:
            print(f"--- {out_name} (from {', '.join(source_names)}) ---\n")
            print(page)

    return 0


if __name__ == "__main__":
    sys.exit(main())
