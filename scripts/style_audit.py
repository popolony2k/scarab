"""
Style audit - counts how far Scarab's code is from docs/CODE_STYLE.md (plans/MASTER_PLAN.md item S1).

C++ rules run over src/ and tests/, Lua rules over samples/, scripts/ and tools/. Every rule
reads the source with comments and string literals blanked out (newlines kept, so line numbers
stay right), the way Caravellius' tools/lua_lint.py does, so a rule never fires on text inside a
comment or a message. The rules are heuristics over text, not a parser: each one is checked
against sunlight's own src/ (the code the C++ guide describes), where it must find (almost)
nothing - see --cpp-root.

Usage:
    style_audit.py                         # summary: count per rule, then per file
    style_audit.py --list RULE             # every finding of one rule, with its line
    style_audit.py --cpp-root DIR          # C++ rules only, over another tree (eg. sunlight's src/)
"""

import glob
import os
import re
import sys
from collections import defaultdict

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER_FILE = os.path.join(REPO_ROOT, "docs", "HEADER.txt")
LICENSE_MARKER = "Copyright (c) since 2021 by PopolonY2k"

CPP_DIRS = ["src", "tests"]
LUA_DIRS = ["samples", "scripts", "tools"]
EXAMPLES_PER_RULE = 2

RULES = {
    # C++ - docs/CODE_STYLE.md, "C++"
    "cpp-license":          "C++ file without the zlib license header",
    "cpp-guard":            "include guard not __FILE_H__ / closing #endif not '#endif /* __FILE_H__ */'",
    "cpp-own-header-first": ".cpp whose first #include is not its own header, by its path from src/",
    "cpp-tab":              "tab character",
    "cpp-using-namespace":  "using namespace",
    "cpp-using-namespace-test": "using namespace in a test (the guide allows it for the class under test)",
    "cpp-oneline-namespace":"one-line nested namespace (namespace A :: B)",
    "cpp-forward-decl":     "forward declaration (class X; / struct X;)",
    "cpp-scope-spacing":    "'::' without a space on each side",
    "cpp-arrow-spacing":    "'->' without a space on each side",
    "cpp-empty-params":     "declaration/definition with '()' instead of '( void )'",
    "cpp-paren-spacing":    "no space after '(' or before ')'",
    "cpp-keyword-paren":    "space between a keyword and its '(' (if (, for (, while (, switch ()",
    "cpp-brace-spacing":    "line ending '{' without exactly two spaces before it",
    "cpp-brace-own-line":   "opening brace alone on its own line",
    "cpp-else-same-line":   "'} else' on one line",
    "cpp-body-blank-line":  "function body not starting with a blank line",
    "cpp-case-colon":       "case/default label without a space before its colon",
    "cpp-pointer-side":     "'*' or '&' with the name, not the type (Type *pName)",
    "cpp-hungarian":        "variable or member whose prefix doesn't match its type (p, sz, b, n, f, str)",
    "cpp-private-const":    ".cpp constant that is not a '#define __NAME' (static const / #define without __)",
    "cpp-raw-new-delete":   "raw new/delete (ownership should be a unique_ptr)",
    "cpp-param-semicolon":  "@param line not ending with ';'",
    # Lua - docs/CODE_STYLE.md, "Lua"
    "lua-license":          "Lua file without the zlib license header",
    "lua-tab":              "tab character",
    "lua-paren-spacing":    "no space after '(' or before ')' in a call or definition",
    "lua-const-prefix":     "top-level private constant without the __ prefix (local NAME = ...)",
    "lua-const-attrib":     "private constant without <const> (local __NAME = ...)",
    "lua-function-name":    "function name not snake_case",
    "lua-local-name":       "local variable in snake_case (should be lowerCamel)",
    "lua-single-quotes":    "single-quoted string that holds no double quote",
    "lua-magic-number":     "number literal other than 0, 1, -1, 2 outside a constant (samples and tools)",
    "lua-magic-number-test":"the same, in a smoke test (expected values are allowed there)",
}

# ---------------------------------------------------------------------------------------------
# Blanking comments and strings


def blank_cpp(text):
    """Comments and string/char literals replaced by spaces, newlines kept."""
    out, i, n = list(text), 0, len(text)

    def blank(a, b):
        for k in range(a, b):
            if out[k] != "\n":
                out[k] = " "

    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            blank(i, j)
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            blank(i, j)
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            blank(i + 1, j)          # keep the quotes, so "( "x" )" still shows its spacing
            i = j + 1
        else:
            i += 1
    return "".join(out)


def blank_lua(text):
    """Comments and strings replaced by spaces, newlines kept; also returns each short string as
    (line, quote, content) for the quote rule."""
    out, i, n, strings = list(text), 0, len(text), []

    def blank(a, b):
        for k in range(a, b):
            if out[k] != "\n":
                out[k] = " "

    def long_bracket(at):
        m = re.match(r"\[(=*)\[", text[at:])
        if not m:
            return None
        close = "]" + m.group(1) + "]"
        j = text.find(close, at + len(m.group(0)))
        return n if j < 0 else j + len(close)

    while i < n:
        c = text[i]
        if text.startswith("--", i):
            end = long_bracket(i + 2)
            if end is None:
                end = text.find("\n", i)
                end = n if end < 0 else end
            blank(i, end)
            i = end
        elif c == "[" and long_bracket(i) is not None:
            end = long_bracket(i)
            blank(i, end)
            i = end
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            strings.append((text.count("\n", 0, i) + 1, c, text[i + 1:j]))
            blank(i + 1, j)
            i = j + 1
        else:
            i += 1
    return "".join(out), strings


# ---------------------------------------------------------------------------------------------
# C++ rules

# A declaration: a type, then a name, then = ; , or ) - one per line is enough to judge.
DECLARATION = re.compile(r"^\s*(?:const\s+|static\s+|unsigned\s+|signed\s+)*([\w:<>\s,]*?[\w>])\s*([*&]*)\s*([*&]*)\s*((?:m_|s_)?)([A-Za-z_]\w*)\s*(=|;|\[)")
INT_TYPES = r"(int|short|long|size_t|u?int\d+_t|unsigned|lua_Integer|uint32_t|uint16_t|SpriteHandle|FontHandle)"


def hungarian_mismatch(line):
    """The prefix a declared name should have, when its type says so and the name lacks it;
    None when the line is no plain declaration or the name fits."""
    m = DECLARATION.match(line)
    if not m or re.match(r"\s*(return|else|case|delete|new|goto|using|typedef|throw)\b", line) or "operator" in line:
        return None
    base, star1, star2, name = re.sub(r"\s+", " ", m.group(1)).strip(), m.group(2), m.group(3), m.group(5)
    stars = star1 + star2
    if base in ("return", "else") or name.isupper() or name.startswith("__") or len(name) < 2:
        return None
    if "*" in stars:
        want = ("sz", "p", "a") if base.endswith("char") else ("p", "a")     # a: a C array of pointers
    elif "&" in stars:
        return None                                   # a reference takes its referee's prefix
    elif base == "bool":
        want = ("b",)
    elif re.fullmatch(r"(unsigned )?" + INT_TYPES, base) or base == "unsigned":
        want = ("n", "h", "it")
    elif base in ("float", "double", "lua_Number"):
        want = ("f",)
    elif base in ("std :: string", "string"):
        want = ("str",)
    else:
        return None
    if m.group(6) == "[":
        want = want + ( "a", )                        # a C array: aName
    def fits(text):
        return any(text.startswith(w) and ( len(text) == len(w) or text[len(w)].isupper() ) for w in want)

    if fits(name) or ( m.group(6) == "[" and name.startswith("a") and fits(name[1:]) ):     # aName, anCount, afInset
        return None
    return "want " + "/".join(want)


# A function definition's head: an optional return type, a name (Class :: Name too), '(' -
# never a line that continues an expression or a condition.
FUNCTION_HEAD = re.compile(r"\s*(?![(&|!])(?!(if|for|while|switch|catch|return|else|do)\b)[\w:<>,~*&\s]*[\w~]\s*\(")


def scope_misspaced(line):
    """'::' needs a space on each side - except a leading '::' (the global namespace, as in
    ::PHYSFS_init), which has nothing on its left."""
    for m in re.finditer(r"::", line):
        before = line[:m.start()]
        after = line[m.end():]
        prev = before.rstrip()[-1:] if before.rstrip() else ""
        if not prev or not ( prev.isalnum() or prev in "_>" ):
            continue
        if re.search(r"\b(return|static|case|new|else|throw|const|inline|virtual|typedef)\s*$", before):
            continue                          # a keyword, then a leading (global) scope
        if not before.endswith(" ") or not after.startswith(" "):
            return True
    return False


def is_block_open(line):
    """A line that opens a block (function, control statement, lambda, namespace, class,
    struct, enum) - not a braced initializer like 'stRectangle {' or '= {'."""
    text = line.rstrip()
    if not text.endswith("{") or text.strip() == "{":
        return False
    head = text[:-1].rstrip()
    return head.endswith(")") or head.endswith("]") or head.endswith("const") or head.endswith("override") \
        or re.match(r"\s*(namespace|class|struct|enum|union|else|do|try)\b", text) is not None


CPP_KEYWORDS_BEFORE_STAR = r"(return|case|throw|delete|sizeof|else|do|new|and|or|not)"
CONTROL = r"\b(if|for|while|switch|catch)\b"


def cpp_findings(path, rel, add):
    raw = open(path, encoding="utf-8", errors="replace").read()
    code = blank_cpp(raw)
    lines = code.split("\n")
    raw_lines = raw.split("\n")
    is_header = path.endswith(".h")
    base = os.path.splitext(os.path.basename(path))[0]

    if LICENSE_MARKER not in "\n".join(raw_lines[:5]):
        add("cpp-license", rel, 1, raw_lines[0])

    if is_header:
        guard = "__" + base.upper() + "_H__"
        if not re.search(r"^#ifndef " + guard + r"\s*$", raw, re.M) or not re.search(r"^#define " + guard + r"\s*$", raw, re.M):
            add("cpp-guard", rel, 1, "expected #ifndef/#define " + guard)
        endifs = [k for k, l in enumerate(raw_lines) if l.startswith("#endif")]
        if endifs and raw_lines[endifs[-1]].strip() != "#endif /* " + guard + " */":
            add("cpp-guard", rel, endifs[-1] + 1, raw_lines[endifs[-1]])
    else:
        includes = [(k, l) for k, l in enumerate(raw_lines) if l.startswith("#include")]
        own = os.path.join(os.path.dirname(path), base + ".h")
        if includes and os.path.exists(own):
            src_root = path[:path.index("/src/") + 5] if "/src/" in path else os.path.dirname(path) + "/"
            want = '#include "' + os.path.relpath(own, src_root) + '"'
            if includes[0][1].strip() != want:
                add("cpp-own-header-first", rel, includes[0][0] + 1, includes[0][1] + "   (want " + want + ")")

    for k, line in enumerate(lines):
        n = k + 1
        rawl = raw_lines[k]
        stripped = line.strip()
        if "\t" in rawl:
            add("cpp-tab", rel, n, rawl)
        if stripped.startswith("#include") or stripped.startswith("#pragma"):
            continue
        if re.search(r"\busing\s+namespace\b", line):
            add("cpp-using-namespace-test" if rel.startswith("tests/") else "cpp-using-namespace", rel, n, rawl)
        if re.match(r"\s*namespace\s+\w+\s*::", line):
            add("cpp-oneline-namespace", rel, n, rawl)
        if re.match(r"\s*(class|struct)\s+\w+\s*;\s*$", line):
            add("cpp-forward-decl", rel, n, rawl)
        if scope_misspaced(line):
            add("cpp-scope-spacing", rel, n, rawl)
        if re.search(r"\S->|->\S", line):
            add("cpp-arrow-spacing", rel, n, rawl)
        if re.match(r"\s*(?!return\b|else\b|new\b|delete\b)(virtual\s+|static\s+|inline\s+|explicit\s+)*[\w:<>,\s*&~]*[\w>*&]\s+[*&]?[~\w:]+\s*\(\)\s*(const\s*)?(override\s*)?(=\s*0\s*)?[;{]", line) \
                and not re.match(r"\s*(return|else|case)\b", line):
            add("cpp-empty-params", rel, n, rawl)
        if not stripped.startswith("#"):
            if re.search(r"\((?![\s)]|$)|(?<![\s(])\)", line.rstrip()):        # '(' may end a line
                add("cpp-paren-spacing", rel, n, rawl)
        if re.search(CONTROL + r"\s+\(", line):
            add("cpp-keyword-paren", rel, n, rawl)
        if is_block_open(line) and not re.search(r"\S  \{$", line.rstrip()):
            add("cpp-brace-spacing", rel, n, rawl)
        if stripped == "{":
            add("cpp-brace-own-line", rel, n, rawl)
        if re.search(r"\}\s*else\b", line):
            add("cpp-else-same-line", rel, n, rawl)
        if re.search(r"\)\s*(const\s*)?(override\s*)?\s+\{$", line.rstrip()) and not re.search(CONTROL, line) \
                and not re.search(r"\]\s*\(", line) and k + 1 < len(lines) and lines[k + 1].strip() not in ("", "}"):
            # a function definition (not a control statement, not a lambda) whose next line is code
            if FUNCTION_HEAD.match(line):
                add("cpp-body-blank-line", rel, n, rawl)
        if re.match(r"\s*(case\b.*[^\s:]|default):(?!:)", line):
            add("cpp-case-colon", rel, n, rawl)
        if re.search(r"(?<![\w])(?!" + CPP_KEYWORDS_BEFORE_STAR + r"\b)[\w>]+\s+[*&]+[A-Za-z_]\w*\s*[,;)=\[]", line) \
                and not re.search(r"[=+\-/<>!|&?:]\s*\w+\s+[*&]", line):
            add("cpp-pointer-side", rel, n, rawl)
        hungarian = hungarian_mismatch(line)
        if hungarian:
            add("cpp-hungarian", rel, n, rawl + "   (" + hungarian + ")")
        if not is_header:
            if re.match(r"\s*(static\s+)?const(expr)?\s+[\w:<>\s*]+\s+[A-Z][A-Z0-9_]+\s*(\[\])?\s*=", line):
                add("cpp-private-const", rel, n, rawl)
            m = re.match(r"\s*#define\s+(\w+)(\s+\S|\()", rawl)          # a define with no value is a switch, not a constant
            if m and not m.group(1).startswith("__"):
                add("cpp-private-const", rel, n, rawl)
        if re.search(r"\bnew\s+[A-Za-z_]", line) or re.search(r"\bdelete\b(?!\s*;)", line):
            add("cpp-raw-new-delete", rel, n, rawl)

    for k, rawl in enumerate(raw_lines):
        if "@param" in rawl:
            last = k                          # a @param may continue on the next comment lines
            while last + 1 < len(raw_lines) and re.match(r"\s*\*\s+[^@\s/]", raw_lines[last + 1]) \
                    and not raw_lines[last].rstrip().endswith(";"):
                last += 1
            if not raw_lines[last].rstrip().endswith(";"):
                add("cpp-param-semicolon", rel, k + 1, rawl)


# ---------------------------------------------------------------------------------------------
# Lua rules


def lua_findings(path, rel, add):
    raw = open(path, encoding="utf-8", errors="replace").read()
    code, strings = blank_lua(raw)
    lines = code.split("\n")
    raw_lines = raw.split("\n")
    is_test = rel.startswith("scripts/")

    if LICENSE_MARKER not in "\n".join(raw_lines[:5]):
        add("lua-license", rel, 1, raw_lines[0])

    for k, line in enumerate(lines):
        n = k + 1
        rawl = raw_lines[k]
        if "\t" in rawl:
            add("lua-tab", rel, n, rawl)
        if re.search(r"[\w\])]\((?![\s)])|(?<![\s(])\)", line):
            add("lua-paren-spacing", rel, n, rawl)
        m = re.match(r"local\s+([A-Za-z_]\w*)\s*(<const>)?\s*=(?!=)", line)          # top level only
        if m:
            name, const = m.group(1), m.group(2)
            if re.fullmatch(r"[A-Z][A-Z0-9_]*", name):
                add("lua-const-prefix", rel, n, rawl)
            elif re.fullmatch(r"__[A-Z][A-Z0-9_]*", name) and not const:
                add("lua-const-attrib", rel, n, rawl)
        m = re.match(r"\s*(local\s+)?function\s+([\w.:]+)\s*\(", line)
        if m:
            name = re.split(r"[.:]", m.group(2))[-1]
            if re.search(r"[A-Z]", name):
                add("lua-function-name", rel, n, rawl)
        m = re.match(r"\s*local\s+([a-z][a-z0-9]*_[a-z0-9_]*)\s*[=,]", line)
        if m:
            add("lua-local-name", rel, n, rawl)
        if not re.match(r"local\s+(__)?[A-Z][A-Z0-9_]*\b", line):          # a constant's own definition names it
            nums = [x for x in re.findall(r"(?<![\w.])\d+(?:\.\d+)?(?![\w.])", line) if float(x) not in ( 0.0, 1.0, 2.0 )]
            if nums:
                add("lua-magic-number-test" if is_test else "lua-magic-number", rel, n, rawl)

    for n, quote, content in strings:
        if quote == "'" and '"' not in content:
            add("lua-single-quotes", rel, n, raw_lines[n - 1])


# ---------------------------------------------------------------------------------------------


def main():
    args = sys.argv[1:]
    list_rule = args[args.index("--list") + 1] if "--list" in args else None
    cpp_root = args[args.index("--cpp-root") + 1] if "--cpp-root" in args else None

    findings = defaultdict(list)          # rule -> [(file, line, text)]

    def add(rule, rel, n, text):
        findings[rule].append((rel, n, text.strip()[:110]))

    if cpp_root:
        roots = [(cpp_root, os.path.abspath(cpp_root))]
        for base, root in roots:
            for path in sorted(glob.glob(os.path.join(root, "**", "*.[ch]pp"), recursive=True) +
                               glob.glob(os.path.join(root, "**", "*.h"), recursive=True)):
                cpp_findings(path, os.path.relpath(path, root), add)
    else:
        for d in CPP_DIRS:
            for path in sorted(glob.glob(os.path.join(REPO_ROOT, d, "**", "*.cpp"), recursive=True) +
                               glob.glob(os.path.join(REPO_ROOT, d, "**", "*.h"), recursive=True)):
                cpp_findings(path, os.path.relpath(path, REPO_ROOT), add)
        for d in LUA_DIRS:
            for path in sorted(glob.glob(os.path.join(REPO_ROOT, d, "**", "*.lua"), recursive=True)):
                lua_findings(path, os.path.relpath(path, REPO_ROOT), add)

    if list_rule:
        for rel, n, text in findings.get(list_rule, []):
            print(f"{rel}:{n}: {text}")
        return 0

    print(f"{'count':>6}  {'files':>5}  rule")
    for rule, desc in RULES.items():
        items = findings.get(rule, [])
        if cpp_root and not rule.startswith("cpp-"):
            continue
        files = len({rel for rel, _, _ in items})
        print(f"{len(items):6}  {files:5}  {rule}: {desc}")
        for rel, n, text in items[:EXAMPLES_PER_RULE]:
            print(f"{'':15}e.g. {rel}:{n}: {text}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
