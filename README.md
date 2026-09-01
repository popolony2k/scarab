# Scarab

A thin, game-agnostic C++17 host for 2D games scripted entirely in Lua, run from a script, project file, or single bundled `.zip` archive. Scarab owns the window/render loop, resource loading, and the Lua-callable primitive API; the game itself — every enemy, the player, config/stage bootstrap, everything — lives entirely in Lua, with no C++ game logic anywhere in this repo.

Licensed under the [zlib License](LICENSE), the same license `sunlight` (the engine Scarab is built on) uses.

## Motivation :bulb:

Scarab started as a personal project with a simple goal: build a game called **Caravellius**. Months into that game's own development, the parts that had nothing to do with any specific game — the window/render loop, tile-map rendering, sprites, collision, sound, input — were split out into their own library, [**sunlight**](https://github.com/popolony2k/sunlight). About a year later, a second idea became clear: that same game-agnostic split could go one step further, into a thin host with *no* game-specific C++ left at all, where an entire game is scripted in Lua on top of sunlight instead. That idea became Scarab, split out by its author into a project of its own.

Read the full story, in the author's own words: [*From a Transforming Ship to a Transforming Engine*](https://www.popolony2k.com.br/from-a-transforming-ship-to-a-transforming-engine/).

## Table of Contents :pushpin:
* [Lua API reference](#lua-api-reference-book) :book:
* [Scarab Class Documentation](#scarab-class-documentation-gear) :gear:
* [Repository layout](#repository-layout-package) :package:
* [Prerequisites](#prerequisites-memo) :memo:
* [Building](#building-hammer) :hammer:
    - [Lua backend option](#lua-backend-option)
    - [VSCode setup](#vscode-setup)
* [Running](#running-rocket) :rocket:
* [License](#license-scroll) :scroll:

## Lua API reference :book:

Every Lua-callable primitive the engine exposes to game scripts — camera, input, tile map, sound, sprites, collision, JSON loading, script sequencing, timers, and the callbacks the engine calls back into Lua — is published at **[popolony2k.github.io/scarab/lua-api/](https://popolony2k.github.io/scarab/lua-api/)**, one page per category with runnable examples, rebuilt on every push to `main`. This is the engine's own generic API surface — it documents nothing about any specific game's own Lua modules, since none live in this repo. Generated straight from `@luaname`/`@luadoc`/`@luaexample` tags on each primitive's own C++ doc comment in [src/lua/](src/lua/) (see [scripts/generate_lua_api_docs.py](scripts/generate_lua_api_docs.py)), not hand-written separately — [docs/lua-api/README.md](docs/lua-api/README.md) is the one page still hand-written (an index with no single-primitive anchor to tag), also serving as that site's own landing page.

## Scarab Class Documentation :gear:

The C++ source itself — every class/method/param under `src/`, generated from its own `/** @brief */` comments via Doxygen (see [Doxyfile](Doxyfile)) — is published at **[popolony2k.github.io/scarab](https://popolony2k.github.io/scarab/)**, rebuilt on every push to `main` ([.github/workflows/doxygen.yml](.github/workflows/doxygen.yml)), alongside the Lua API reference above at the same site (`/lua-api/`). Unlike `sunlight`, Scarab isn't consumed as a library — nothing links against these classes from outside this repo — so this is a structure reference for working on Scarab's own C++, not a public API surface. It also doesn't (and can't) know that a method like `LuaAppApi::GetPlatform` is registered into Lua under a different name (`app_get_platform`), since that mapping only exists in a runtime `lua_register()` call; the Lua API reference above is the one built specifically to document that mapping.

## Repository layout :package:

```
scarab/
├── CMakeLists.txt          # fetches sunlight/raylib/tmx/Lua/nlohmann_json/CLI11, builds the scarab executable from src/
├── src/
│   ├── main.cpp / main.h      # entry point, window/viewport setup, CLI11 argument parsing
│   ├── host/                  # EngineHost: thin engine state machine only, all game logic lives in Lua
│   ├── lua/                   # Lua scripting engine glue - LuaEngine + every Lua-callable primitive (camera/input/tilemap/sound/sprite/collision/JSON/filesystem)
│   └── engine/                # game-agnostic sprite pool (SpritePool/SpriteHandle) - the one piece with no Lua dependency
├── docs/
│   ├── lua-api/            # the one hand-written page (README.md) - see "Lua API reference" above
│   ├── vscode/.vscode/     # tracked .vscode sample - see "VSCode setup" below
│   └── README-DEBUG.txt   # debug build flags
└── CMakePresets.json       # default/windows-vcpkg presets (only real platform-conditional CMake config)
```

Scarab is a standalone engine — it has no bundled game and can't render anything on its own without one pointed at it via the entry-point argument (see "Running" below). A real game built on Scarab (for example, **Caravellius**, a vertical-scrolling space shooter, in its own separate repo) depends on it via CMake `FetchContent`, the same way Scarab itself depends on `sunlight` below.

The **sunlight** engine itself is not vendored here — it's pulled in via CMake `FetchContent` from `github.com/popolony2k/sunlight` (see the root [CMakeLists.txt](CMakeLists.txt)). It wraps [raylib](https://www.raylib.com/) (rendering/input/audio) and [libtmx](https://github.com/baylej/tmx.git) (Tiled map loading) behind its own `SunLight::*` namespaces (renderer, canvas/sprite, collision, input, sound, scripting).

## Prerequisites :memo:

- CMake 3.24+
- A C++17 compiler (Clang on macOS, GCC on Linux, MSVC on Windows)
- System zlib
- On Linux, raylib's own system dependencies (see raylib's wiki) must be installed first

All other dependencies (sunlight, raylib, libtmx, libxml2, Lua, nlohmann/json, CLI11) are fetched automatically at configure time.

## Building :hammer:

```shell
cmake -B build -S .
cmake --build build -j 4
```

For a debug build with symbols:

```shell
cmake -B build -S . -DCMAKE_C_FLAGS="-g2" -DCMAKE_CXX_FLAGS="-g2"
```

The `scarab` target — the executable is engine-branded, not game-branded, since it's built entirely from Scarab's own C++ (the game it runs is determined at launch by the entry-point argument, not compiled in) — links against `sunlight`, `nlohmann_json`, `CLI11`, and Lua, and a post-build step (`scarab_copy_binaries`) copies the raylib/tmx/libxml2/sunlight shared libraries next to the executable, since they're required at runtime.

### Lua backend option

By default Lua is built from `walterschell/Lua` (CMake-friendly fork). Pass `-DSCARAB_USE_OFFICIAL_LUA_FTP=ON` to instead build from the official Lua 5.4.6 FTP tarball (Unix-only, requires `make`).

On Windows, `libxml2`/`tmx` link vcpkg's `iconv`/`zlib` dynamically, so configure with `cmake --preset windows-vcpkg` instead (needs `VCPKG_ROOT` set in the environment) — see the `CMakePresets.json` preset and `CLAUDE.md`'s Build section for details.

### VSCode setup

The repo root's `.vscode` folder is **gitignored** — it's local, per-developer state. The tracked source of truth is [docs/vscode/.vscode](docs/vscode/README.md); copy its contents into a `.vscode` folder at the repo root to get started, and copy any `.vscode` change you want to keep back into that folder rather than leaving it only in your local copy. See [docs/vscode/README.md](docs/vscode/README.md) for the per-OS `settings.<os>.json` variants and the recommended extension that auto-swaps them in.

## Running :rocket:

The built `scarab` executable **requires** a command-line argument naming the entry point — a `.json` project file, a `.lua` script directly, or a single bundled `.zip` archive — and refuses to start (printing a usage message, no window opened) without one. Argument parsing is CLI11, so `--help` is always available, and so is `--version`/`-v` (prints both Scarab's own version and the exact pinned sunlight version, e.g. `scarab v0.1.2 (sunlight v0.17.1)`):

```shell
cd build
./scarab project.json
```

A project file just names the first Lua file to run:

```json
{ "main_script": "src/main.lua" }
```

`main_script` resolves relative to the project file's own directory, so a whole project (executable + its own resources + `project.json`) stays relocatable as a unit. You can also skip the project file and point directly at a `.lua` entry script: `./scarab src/main.lua`.

A whole project can also be packaged into a single self-contained `.zip` and run the same way: `./scarab game.zip`. `--entry`/`-e <path>` names the actual entry point when it's not simply `project.json` at the archive's own root — either another `.json` project file inside the archive, or a `.lua` path to run directly with no project-file indirection at all.

Every resource read (Lua `dofile`/`load_json`, texture/sound/tilemap loading, the entry script/project-file reads themselves) routes through a mount-based filesystem abstraction (PhysFS-backed) — a loose directory or a real archive read identically, so the same project runs unchanged either way.

## License :scroll:

Scarab is licensed under the [zlib License](LICENSE) — permissive, no attribution required at runtime, alterations must be marked as such. Every `.cpp`/`.h` file under `src/` carries the license notice at its top (see [docs/HEADER.txt](docs/HEADER.txt) for the exact text new files should start with).
