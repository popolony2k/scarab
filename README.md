# game-engine

A C++17 monorepo containing the **sunlight** 2D tile-map game engine and the games built on top of it. Today the only game is **Caravellius**, a vertical-scrolling space shooter.

## Repository layout

```
game-engine/
├── CMakeLists.txt          # root build, adds games/caravellius
├── docs/                   # misc engineering notes (debug build flags, ...)
│   ├── lua-api/            # full Lua engine API reference - see "Lua API reference" below
│   └── vscode/.vscode/     # tracked .vscode sample - see "VSCode setup" below
├── CMakePresets.json       # windows-vcpkg preset (only real platform-conditional CMake config)
├── ide-setup/              # shared IDE config (Eclipse formatter, ...)
└── games/
    └── caravellius/
        ├── CMakeLists.txt  # fetches sunlight, raylib, tmx, Lua, nlohmann/json
        ├── project.json    # entry-point descriptor read at launch - see "Running" below
        ├── src/
        │   ├── main.cpp / main.h      # entry point, window/viewport setup
        │   ├── host/                  # EngineHost: thin engine state machine only, all game logic lives in Lua
        │   ├── lua/                   # Lua scripting engine glue - LuaEngine + every Lua-callable primitive (camera/input/tilemap/sound/sprite/collision/JSON)
        │   └── engine/                # game-agnostic sprite pool (SpritePool/SpriteHandle) - the one piece with no Lua dependency
        ├── resources/
        │   ├── configs/               # sprite sets, moving params, sound/sprite/stage files (JSON) - read directly by Lua
        │   ├── scripts/                # Lua: main.lua, bootstrap.lua, spriteconfig.lua, player.lua, enemies/ (per-enemy modules), stages/ (stage scripts)
        │   ├── tilemap/                # Tiled (.tmx/.tsx) maps
        │   ├── sprites/, audio/        # game art and sound assets
        │   └── projects/               # source art projects (Aseprite, SAI, vector)
        └── docs/
```

The **sunlight** engine itself is not vendored here — it's pulled in via CMake `FetchContent` from `github.com/popolony2k/sunlight` (see [games/caravellius/CMakeLists.txt](games/caravellius/CMakeLists.txt)). It wraps [raylib](https://www.raylib.com/) (rendering/input/audio) and [libtmx](https://github.com/baylej/tmx.git) (Tiled map loading) behind its own `SunLight::*` namespaces (renderer, canvas/sprite, collision, input, sound, scripting).

## Prerequisites

- CMake 3.24+
- A C++17 compiler (Clang on macOS, GCC on Linux, MSVC on Windows)
- System zlib
- On Linux, raylib's own system dependencies (see raylib's wiki) must be installed first

All other dependencies (sunlight, raylib, libtmx, libxml2, Lua, nlohmann/json) are fetched automatically at configure time.

## Building

```shell
cmake -B build -S .
cmake --build build -j 4
```

For a debug build with symbols:

```shell
cmake -B build -S . -DCMAKE_C_FLAGS="-g2" -DCMAKE_CXX_FLAGS="-g2"
```

The `caravellius` target links against `sunlight`, `nlohmann_json`, and Lua, and a post-build step (`caravellius_copy_binaries`) copies the raylib/tmx/libxml2/sunlight shared libraries next to the executable, since they're required at runtime.

### Lua backend option

By default Lua is built from `walterschell/Lua` (CMake-friendly fork). Pass `-DSCARAB_USE_OFFICIAL_LUA_FTP=ON` to instead build from the official Lua 5.4.6 FTP tarball (Unix-only, requires `make`).

On Windows, `libxml2`/`tmx` link vcpkg's `iconv`/`zlib` dynamically, so configure with `cmake --preset windows-vcpkg` instead (needs `VCPKG_ROOT` set in the environment) — see the `CMakePresets.json` preset and `CLAUDE.md`'s Build section for details.

### VSCode setup

The repo root's `.vscode` folder is **gitignored** — it's local, per-developer state. The tracked source of truth is [docs/vscode/.vscode](docs/vscode/README.md); copy its contents into a `.vscode` folder at the repo root to get started, and copy any `.vscode` change you want to keep back into that folder rather than leaving it only in your local copy. See [docs/vscode/README.md](docs/vscode/README.md) for the per-OS `settings.<os>.json` variants and the recommended extension that auto-swaps them in.

## Running

The built `caravellius` executable **requires** a command-line argument naming the entry point — either a `.json` project file or a `.lua` script directly — and refuses to start (printing a usage message, no window opened) without one:

```shell
cd build/games/caravellius
./caravellius project.json
```

`project.json` (copied next to the executable at build time from `games/caravellius/project.json`) just names the first Lua file to run:

```json
{ "main_script": "resources/scripts/main.lua" }
```

`main_script` resolves relative to the project file's own directory, so the whole project (executable + `resources/` + `project.json`) stays relocatable as a unit. You can also skip the project file and point directly at a `.lua` entry script: `./caravellius resources/scripts/main.lua`.

## Gameplay

Caravellius is a vertical shoot-'em-up: the player ship scrolls up a Tiled map while enemy ships (Satellite, Cylinder, Galileo, Nomad, Alien, Ovni, Octopus) spawn and move using data-driven patterns (sine waves, circular orbits, quadrant patrols) and fire straight-line or homing (Bresenham-seeking) bullets. Levels are orchestrated by Lua stage scripts that call into a `ScriptProcessor` command queue (`sp_move_sprites_to_screen`, `sp_wait`, `sp_play_song`, ...), while enemy stats/textures/sounds are defined in JSON config files under `resources/configs/` and read directly by Lua.

## Lua migration (complete)

A refactor moved all enemy/sprite/player game logic and config/stage bootstrap out of C++ and into Lua (`resources/scripts/`), then deleted the C++ that became dead as a result, leaving C++ as a thin, reusable, game-agnostic engine layer. Every enemy type, the player ship, and config/stage loading are fully Lua-driven, and the legacy C++ they replaced (the old JSON config loader, per-enemy-type movement state machines, and the sprite queue/collision machinery built around them) has been deleted. See `CLAUDE.md` for the current architecture split.

## Lua API reference

Every Lua-callable primitive the engine exposes to game scripts — camera, input, tile map, sound, sprites, collision, JSON loading, script sequencing, timers, and the callbacks the engine calls back into Lua — is documented in [docs/lua-api/](docs/lua-api/README.md), one page per category with runnable examples. This is the engine's own generic API surface, not Caravellius's game-specific Lua modules (those live in `resources/scripts/` and are game content built on top of this API).
