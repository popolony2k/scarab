# game-engine

A C++17 monorepo containing the **sunlight** 2D tile-map game engine and the games built on top of it. Today the only game is **Caravellius**, a vertical-scrolling space shooter.

## Repository layout

```
game-engine/
├── CMakeLists.txt          # root build, adds games/caravellius
├── docs/                   # misc engineering notes (debug build flags, ...)
├── ide-setup/              # shared IDE config (Eclipse formatter, ...)
└── games/
    └── caravellius/
        ├── CMakeLists.txt  # fetches sunlight, raylib, tmx, Lua, nlohmann/json
        ├── src/
        │   ├── main.cpp / main.h      # entry point, window/viewport setup
        │   ├── world/                 # WorldEngine/WorldBase: engine state machine (game logic now lives in Lua)
        │   ├── sprite/                # legacy per-enemy-type movement processing (dead, all migrated to Lua)
        │   ├── lua/                   # Lua scripting engine glue
        │   ├── engine/                # game-agnostic sprite pool + Lua-callable primitives (camera/input/tilemap/sound/collision/JSON)
        │   └── config/                # legacy JSON config loading (dead, superseded by Lua's own load_json reads)
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

## Running

The built `caravellius` executable resolves resource paths (stages, sprites, audio, Lua scripts, JSON configs) relative to the game's `resources/` directory, so run it from `games/caravellius/` (or copy `resources/` next to the binary).

## Gameplay

Caravellius is a vertical shoot-'em-up: the player ship scrolls up a Tiled map while enemy ships (Satellite, Cylinder, Galileo, Nomad, Alien, Ovni, Octopus) spawn and move using data-driven patterns (sine waves, circular orbits, quadrant patrols) and fire straight-line or homing (Bresenham-seeking) bullets. Levels are orchestrated by Lua stage scripts that call into a `ScriptProcessor` command queue (`sp_move_sprites_to_screen`, `sp_wait`, `sp_play_song`, ...), while enemy stats/textures/sounds are defined in JSON config files under `resources/configs/` and read directly by Lua.

## Lua migration (complete through Phase 8)

A refactor has moved enemy/sprite/player game logic and config/stage bootstrap out of C++ and into Lua (`resources/scripts/`), leaving C++ as a thinner, reusable, game-agnostic engine layer. Every enemy type (Satellite, Cylinder, Alien, Octopus, Ovni, Galileo, Nomad), the player ship, and config/stage loading are fully migrated - only bulk-deleting the now-dead legacy C++ remains. See `CLAUDE.md` for the current architecture split.
