# Lua integration: current state, findings, and a future plan

Written 2026-09-19. The findings below were verified by reading the real
sources and by a real configure attempt (not assumed); the plan at the end
is a **future intent only — nothing here is scheduled or in progress.**

## Current state

Scarab has two Lua backends, selected by `SCARAB_USE_OFFICIAL_LUA_FTP`
in the root `CMakeLists.txt`:

| | Default (`OFF`) | Official (`ON`) |
|---|---|---|
| Source | `walterschell/Lua` via `FetchContent`, `GIT_TAG master` | `lua-5.4.6.tar.gz` from lua.org via `ExternalProject_Add`, built by the tarball's own Makefile |
| Lua version | whatever the fork packages today (`5.4.7` at the time of writing) | `5.4.6` |
| Build system | CMake (a wrapper around the unmodified Lua source) | GNU `make`, driven with no explicit platform target |
| Link target | `lua_static` | an `IMPORTED` target pointing at `src/liblua.a` |
| Tested in CI | yes, every platform | **no** — neither `ci.yml` nor `release.yml` ever builds it |

The official option is a deliberately kept future direction (see the plan
below). It is **not currently usable on any platform** — see the findings.

## Findings

### Readline

`readline` is only ever relevant to the standalone `lua` *interpreter
executable* (`lua.c`'s interactive prompt/history; without it `lua.c`
falls back to `fgets`). It never touches `liblua.a` or `luac`, so it is
irrelevant to embedding Lua in Scarab itself.

- **Default (fork) path:** `lua-5.4.7/CMakeLists.txt` runs
  `CHECK_INCLUDE_FILE("readline/readline.h")` and defines
  `LUA_USE_READLINE` inside `if(LUA_BUILD_BINARY)` only. That option
  defaults `OFF` when the fork is a subproject and Scarab never sets it,
  so it is inert for Scarab. (A consumer that forces `LUA_BUILD_BINARY ON`
  — Caravellius does, to get a standalone `lua` for its unit tests —
  activates it; it links `readline` when the header is found.)
  `LUA_BUILD_COMPILER` (the `luac` target) defaults `ON` even as a
  subproject.
- **Official path:** readline is decided entirely by the tarball's own
  Makefile, not by any CMake variable. Scarab runs `make` with no target,
  which runs `all: $(PLAT)` with `PLAT=guess`, which runs `uname` and
  re-invokes make on the result:
  - **Linux** → `linux-noreadline`: `-DLUA_USE_LINUX`, no readline.
  - **macOS (Darwin)** → `macosx`: hard-codes `-DLUA_USE_MACOSX
    -DLUA_USE_READLINE -lreadline`. So on macOS the official build **does**
    enable readline (for the `lua` executable), without Scarab defining
    anything.
  - **FreeBSD/NetBSD/OpenBSD** → the same, using `libedit`.

  The tempting reading "readline is off on the official path because
  Scarab never defines the macro" is therefore only true on Linux.

### The official option does not work today

Verified by a real configure on Windows with
`-DSCARAB_USE_OFFICIAL_LUA_FTP=ON`, plus reading the code:

- **`find_package(Lua REQUIRED)` sits inside the same `else()` as the
  `ExternalProject_Add` block** (a block commented "based on FindPackage"
  that looks like it was meant to be a separate third option). Configure
  fails there (`Could NOT find Lua (missing: LUA_INCLUDE_DIR)`) on any
  machine with no system Lua development package — on Windows, and on
  Linux/macOS too unless one is installed.
- `SOURCE_DIR ${lua_engine_SOURCE_DIR}` references a variable defined
  nowhere in the project.
- The download URL is plain `http://` with no `URL_HASH`.
- **Windows has no usable make target.** The tarball's Makefile has a
  `mingw` target only (producing `lua54.dll`/`lua.exe`, not `liblua.a`);
  there is nothing for MSVC, and `find_program(MAKE_EXE NAMES gmake nmake
  make)` can pick `nmake`, which cannot read a GNU Makefile.
- Nothing in CI builds this option, so none of the above was ever caught.

### What the official build produces

The tarball has **no CMake support**, so `lua` and `luac` would *not* be
CMake targets (they are with the fork: `lua` when `LUA_BUILD_BINARY` is
on, `luac` by default). The official Makefile builds `src/liblua.a`,
`src/lua` and `src/luac` in the source tree (with `.exe`/`.dll` variants
under the `mingw` target). Where that source tree lands for Scarab's
`ExternalProject` is not currently well defined (see the undefined
`SOURCE_DIR` above) and has **not** been verified.

Downstream consequence: a game repo that discovers `lua`/`luac` through
the fork's CMake targets or build tree (as Caravellius's
`check_project.py`/`run_lua_tests.py` do) would find nothing under an
official-only integration, and would need pointing at wherever the official
build's binaries end up.

## Future plan (intent only — not started, not scheduled)

The project owner's intent is for the **official Lua from lua.org to
eventually be the only way Lua is integrated with Scarab**, replacing the
`walterschell/Lua` fork. This is a future direction, not current work.
**When it does start, the requirement is that it works on macOS, Linux
and Windows** — not "Unix-only" as the option is described today.

A sketch of what that will need, to be refined when work actually begins
(none of this is verified or decided):

1. **Windows is the real design question.** The tarball's Makefile does
   not cover MSVC. Rather than driving `make`, one likely approach is a
   small in-repo `CMakeLists.txt` that compiles the tarball's own
   unmodified `src/*.c` (everything except `lua.c`/`luac.c`) into a static
   library and builds `lua`/`luac` as normal CMake targets — the same shape
   `walterschell/Lua` already provides, but built from the official
   source rather than a third-party fork. That would also fix the
   "no `lua`/`luac` CMake targets" problem for every platform at once.
2. Pin the exact Lua release deliberately (the fork currently floats on
   `master`), download over `https://` with `URL_HASH`, and record the
   version in one place, the same way `SCARAB_SUNLIGHT_VERSION` is.
3. Decide readline explicitly rather than inheriting it from the
   Makefile's per-platform choice (the official path enables it on macOS
   implicitly).
4. Remove the misplaced `find_package(Lua REQUIRED)` and the undefined
   `SOURCE_DIR`.
5. Add the option to `ci.yml` (and the release smoke tests) for all four
   platforms *before* making it the default, so it stops being untested.
6. Give downstream games a stable, documented way to find the built
   `lua`/`luac` (a CMake target name or a fixed output path), and tell
   them (Caravellius) before switching.
