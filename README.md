# Scarab

A thin, game-agnostic C++17 host for 2D games scripted entirely in Lua, run from a script, project file, or single bundled `.zip` archive. Scarab owns the window/render loop, resource loading, and the Lua-callable primitive API; the game itself — every enemy, the player, config/stage bootstrap, everything — lives entirely in Lua, with no C++ game logic anywhere in this repo.

## Repository layout

```
game-engine/
├── CMakeLists.txt          # Scarab's own build: fetches sunlight/raylib/tmx/Lua/nlohmann_json/CLI11, builds the scarab executable from src/
├── src/                    # Scarab (the engine) - lives at the repo root deliberately, see below
│   ├── main.cpp / main.h      # entry point, window/viewport setup, CLI11 argument parsing
│   ├── host/                  # EngineHost: thin engine state machine only, all game logic lives in Lua
│   ├── lua/                   # Lua scripting engine glue - LuaEngine + every Lua-callable primitive (camera/input/tilemap/sound/sprite/collision/JSON/filesystem)
│   └── engine/                # game-agnostic sprite pool (SpritePool/SpriteHandle) - the one piece with no Lua dependency
├── docs/                   # misc engineering notes (debug build flags, ...)
│   ├── lua-api/            # full Lua engine API reference - see "Lua API reference" below
│   └── vscode/.vscode/     # tracked .vscode sample - see "VSCode setup" below
├── CMakePresets.json       # default/windows-vcpkg presets (only real platform-conditional CMake config)
└── caravellius/            # the game - a sibling of Scarab's own src/, not nested under anything Scarab-owned
    ├── CMakeLists.txt          # bundles Caravellius's own assets next to the already-built scarab executable, and the caravellius_archive packaging target
    ├── project.json            # entry-point descriptor read at launch - see "Running" below
    ├── src/                    # Lua: main.lua + core/ (bootstrap, player, camera, stage director, ...), enemies/ (per-enemy modules), bullets/ (shared projectile pools), ids/ (wave/stage/sound/layer id definitions), stages/ (stage scripts), debug/ (standing debug-only launch sessions - see "Running" below)
    ├── resources/              # runtime data - never Lua
    │   ├── configs/               # sprite sets, moving params, sound/sprite/stage files (JSON) - read directly by Lua
    │   ├── tilemap/                # Tiled (.tmx/.tsx) maps
    │   ├── sprites/                # game art, split into enemies/, bullets/, explosions/
    │   ├── audio/                  # sound assets
    │   ├── fonts/                  # bitmap/TTF fonts
    │   └── images/                 # storyboard/presentation art
    ├── assets/                 # non-runtime source art (Aseprite/SAI/vector) and the Tiled editor project - never read at runtime
    └── docs/
```

`src/` lives at the repo root, not under `caravellius/`, because it's Scarab's own code, not Caravellius's - see [CLAUDE.md](CLAUDE.md) for the full Scarab/Caravellius naming split. This also means when Caravellius eventually moves to its own repo (the stated long-term plan), only `caravellius/` needs to go - `src/`/the root `CMakeLists.txt` stay behind as Scarab, already in the right place.

The **sunlight** engine itself is not vendored here — it's pulled in via CMake `FetchContent` from `github.com/popolony2k/sunlight` (see the root [CMakeLists.txt](CMakeLists.txt)). It wraps [raylib](https://www.raylib.com/) (rendering/input/audio) and [libtmx](https://github.com/baylej/tmx.git) (Tiled map loading) behind its own `SunLight::*` namespaces (renderer, canvas/sprite, collision, input, sound, scripting).

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

The `scarab` target — the executable is engine-branded, not game-branded, since it's built entirely from Scarab's own C++ (the game it runs is determined at launch by the entry-point argument, not compiled in) — links against `sunlight`, `nlohmann_json`, and Lua, and a post-build step (`scarab_copy_binaries`) copies the raylib/tmx/libxml2/sunlight shared libraries next to the executable, since they're required at runtime.

### Lua backend option

By default Lua is built from `walterschell/Lua` (CMake-friendly fork). Pass `-DSCARAB_USE_OFFICIAL_LUA_FTP=ON` to instead build from the official Lua 5.4.6 FTP tarball (Unix-only, requires `make`).

On Windows, `libxml2`/`tmx` link vcpkg's `iconv`/`zlib` dynamically, so configure with `cmake --preset windows-vcpkg` instead (needs `VCPKG_ROOT` set in the environment) — see the `CMakePresets.json` preset and `CLAUDE.md`'s Build section for details.

### VSCode setup

The repo root's `.vscode` folder is **gitignored** — it's local, per-developer state. The tracked source of truth is [docs/vscode/.vscode](docs/vscode/README.md); copy its contents into a `.vscode` folder at the repo root to get started, and copy any `.vscode` change you want to keep back into that folder rather than leaving it only in your local copy. See [docs/vscode/README.md](docs/vscode/README.md) for the per-OS `settings.<os>.json` variants and the recommended extension that auto-swaps them in.

## Running

The built `scarab` executable **requires** a command-line argument naming the entry point — a `.json` project file, a `.lua` script directly, or (Phase 12, archive-distribution) a single `.zip` bundle — and refuses to start (printing a usage message, no window opened) without one. Argument parsing is CLI11, so `--help` is always available:

```shell
cd build
./scarab project.json
```

`project.json` (copied next to the executable at build time from `caravellius/project.json`) just names the first Lua file to run:

```json
{ "main_script": "src/main.lua" }
```

`main_script` resolves relative to the project file's own directory, so the whole project (executable + `resources/`/`src/` + `project.json`) stays relocatable as a unit. You can also skip the project file and point directly at a `.lua` entry script: `./scarab src/main.lua`.

A whole project can also be packaged into a single self-contained `.zip` (`cmake --build build --target caravellius_archive`, output `build/caravellius/caravellius.zip`) and run the same way: `./scarab build/caravellius/caravellius.zip`. `--entry`/`-e <path>` names the actual entry point when it's not simply `project.json` at the archive's own root — either another `.json` project file inside the archive, or a `.lua` path to run directly with no project-file indirection at all.

### Debug launch sessions

`src/debug/` holds standing debug-only entry points for testing an enemy/boss/sub-boss in isolation, without playing through the rest of the stage first — each runs the exact same bootstrap as a normal launch, just landing on a stripped-down stage instead of the real one:

```shell
./scarab src/debug/enemy_main.lua    # any regular wave-spawn enemy - edit DEBUG_ENEMY_STATE_NAME to pick which one
./scarab src/debug/subboss_main.lua  # the mid-stage Venusian sub-boss
./scarab src/debug/cephalon_main.lua # the final boss, Cephalon
```

All loop automatically once the encounter resolves, so repeated testing doesn't need relaunching the executable. Cephalon has two further debug variants beyond it's own base pair — `cephalon_collider_test_main.lua` (arms frozen/hidden/non-collidable, for tuning the head's own hitbox in isolation) and `cephalon_realstage_main.lua` (the real stage's own BGM/background rather than total isolation, for checking things — like the stage-to-boss BGM crossfade — that depend on real stage context).

## Gameplay

Caravellius is a vertical shoot-'em-up: the player ship scrolls up a Tiled map while enemy ships (Satellite, Cylinder, Galileo, Nomad, Alien, Ufo, Octopus) spawn and move using data-driven patterns (sine waves, circular orbits, quadrant patrols) and fire straight-line or homing (Bresenham-seeking) bullets, building toward a mid-stage sub-boss (the Venusian ship) with its own camera scroll-loop arena and an escalating bullet-hell attack pattern partway through the stage, and a final boss — **Cephalon**, an organic creature (not a ship) with a fixed head and two arm chains built from circular segments — right at the stage's end. Each arm has it's own health pool and must be destroyed (segment-by-segment, tip to socket, once it's own HP runs out) before the head can take any damage at all; once both arms are gone the head roams the screen firing corkscrew-trajectory bullets until it, too, is destroyed, telegraphing it's own death with a trembling beat first. Defeating it plays a stage-clear sequence (the ship exits, a banner shows, then the stage loops) with it's own BGM crossfade into and out of the fight. Wave pacing is driven by a condition-based `StageDirector` (a fixed authored sequence, then an endless random-pick filler loop once it completes) rather than a purely linear script; the underlying `ScriptProcessor` command queue (`sp_move_sprites_to_screen`, `sp_wait`, `sp_play_song`, ...) still exists as generic engine plumbing for genuinely sequential things like BGM start and screen fades. Enemy stats/textures/sounds are defined in JSON config files under `resources/configs/` and read directly by Lua.

## Lua migration (complete)

A refactor moved all enemy/sprite/player game logic and config/stage bootstrap out of C++ and into Lua (`caravellius/src/`), then deleted the C++ that became dead as a result, leaving C++ as a thin, reusable, game-agnostic engine layer. Every enemy type, the player ship, and config/stage loading are fully Lua-driven, and the legacy C++ they replaced (the old JSON config loader, per-enemy-type movement state machines, and the sprite queue/collision machinery built around them) has been deleted. See `CLAUDE.md` for the current architecture split.

## Lua API reference

Every Lua-callable primitive the engine exposes to game scripts — camera, input, tile map, sound, sprites, collision, JSON loading, script sequencing, timers, and the callbacks the engine calls back into Lua — is documented in [docs/lua-api/](docs/lua-api/README.md), one page per category with runnable examples. This is the engine's own generic API surface, not Caravellius's game-specific Lua modules (those live in `caravellius/src/` and are game content built on top of this API).

## Credits

Third-party assets (music, art, ...) and the people who made them are credited in [CREDITS.md](CREDITS.md).
