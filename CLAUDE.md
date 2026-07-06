# CLAUDE.md

This file provides guidance to Claude Code when working in this repository.

## Project overview

This repo is a monorepo distributing the **sunlight** 2D game engine and the games built on it. Only game today: **Caravellius** (`games/caravellius`), a vertical-scrolling space shooter (Galaga/1943-like) in C++17 using raylib for rendering/input/audio and Tiled (`.tmx`) maps for stages.

`sunlight` itself is **not** in this repo — it's fetched via CMake `FetchContent` from `github.com/popolony2k/sunlight` (`GIT_TAG main`). If you need to read or modify engine internals rather than game code, that's a separate repo; a cached checkout may exist locally under `build/_deps/sunlight-src` after a configure, and it ships its own `CLAUDE.md`.

## Build

```shell
cmake -B build -S .
cmake --build build -j 4
```

Debug symbols: `cmake -B build -S . -DCMAKE_C_FLAGS="-g2" -DCMAKE_CXX_FLAGS="-g2"`.

Lua backend defaults to `walterschell/Lua` (CMake-friendly). `-DSCARAB_USE_OFFICIAL_LUA_FTP=ON` switches to the official Lua 5.4.6 FTP tarball build (Unix-only).

`games/caravellius/CMakeLists.txt` globs sources with `GLOB_RECURSE` — re-run `cmake -B build -S .` after adding new `.cpp` files so the glob picks them up.

No test suite exists for Caravellius. There is no linter/formatter config; match the existing style (see below).

## Architecture (games/caravellius)

- `src/main.cpp` — entry point. Builds a `SunLight::Renderer::TileMapRenderer` (window/game loop owner) and a `Caravellius::World::WorldEngine`, wires the latter as a tile-map listener, then `Start()`/`Run()`/`Stop()`.
- `src/world/worldbase.{h,cpp}` — `WorldBase`: sprite lifecycle (stock → active → inactive → destroyed queues), collision listener implementation, sound playback, and the `ScriptProcessor`/`LuaEngine` wiring. Sprite layers map to fixed Tiled layer IDs (`LAYER_PLAYER_SHIP=1`, `LAYER_ENEMIES_SHIPS=2`, `LAYER_PLAYER_SHIP_BULLETS=3`, `LAYER_ENEMY_BULLETS=4`) — these **must** exist in any Tiled map used.
- `src/world/worldengine.{h,cpp}` — `WorldEngine` extends `WorldBase`: the actual game state machine (init/run-stage/fatal-error handlers), stage/config loading, and player input handlers (shoot, 4-directional move, gamepad init).
- `src/world/worlddefs.h` — the shared vocabulary: `SpriteType`, `SpriteMovingMode`, `SpriteMovingStateHandlers`, `SoundUniqueId`, and `stSpriteData` (the per-sprite runtime struct: position/size pointers into the engine's own sprite storage, moving-mode counter/direction, Bresenham line-path and circle-path state, hit/explosion/blink timers).
- `src/sprite/spritemoveprocessing.{h,cpp}` — one `Update<ShipType>Ship(stSpriteData&)` method per enemy type, each a small state machine keyed off `sprite.data.mode` / `sprite.data.fCounter` / `sprite.data.direction`. This is where new enemy movement patterns get added. `UpdateEnemyBullet` handles both straight-line and Bresenham-seeker bullets.
- `src/lua/luaengine.{h,cpp}` — Lua ↔ `ScriptProcessor` bridge. Exposes script-callable functions (`sp_move_sprites_to_screen`, `sp_wait`, `sp_wait_queue_empty`, `sp_play_song`, `sp_load_stage`, `set_timer`, ...) and registers the C++ enums as Lua globals so scripts can reference e.g. `STATE_MOVE_OVNI_SHIP_CIRCULAR_TO_SCREEN` or `ID_FIRST_STAGE_BGM` by name.
- `src/config/config.{h,cpp}` — loads JSON (via `nlohmann/json`, `NLOHMANN_DEFINE_TYPE_INTRUSIVE[_WITH_DEFAULT]`) describing sprite sets, sprite-moving parameters, sprite files, sound files, and stage files from `resources/configs/*.json`.

### Data flow for a stage

1. `resources/scripts/main.lua` calls `sp_load_stage(STAGE_FIRST)`.
2. `WorldEngine` loads the matching `.tmx` map and Lua script from `resources/configs/stagefile.json`.
3. The stage's Lua script (e.g. `1st_stage_corsair.lua`) drives the `ScriptProcessor` command queue — spawning enemy waves via `sp_move_sprites_to_screen(STATE_MOVE_..._TO_SCREEN)`, timing via `sp_wait`/`set_timer`, and music via `sp_play_song`.
4. Each active `stSpriteData` gets updated per-frame through `WorldBase`'s update loop, which dispatches into the matching `SpriteMoveProcessing::Update<X>Ship` based on `sprite.type`.
5. Enemy/texture/sound parameters (velocity, hit count, textures, explosion/shoot overrides) come from `resources/configs/spriteset.json` and `sprite-moving-parms.json`, not hardcoded in C++.

**When adding a new enemy type**: add to `SpriteType`/`SpriteFile` in `worlddefs.h`, add an `Update<X>Ship` in `spritemoveprocessing.{h,cpp}`, wire dispatch in `WorldBase`/`WorldEngine`, add moving-mode entries to `SpriteMovingStateHandlers`, and add config entries in the relevant `resources/configs/*.json` — C++ enum changes and JSON config changes go together.

## Code style (existing convention — match it, don't "fix" it)

- Scope resolution operator is always **space-padded**: `Caravellius :: World :: WorldEngine`, `SunLight :: Renderer :: TileMapRenderer`, not `Caravellius::World::WorldEngine`.
- Member functions/types: PascalCase (`UpdateOvniShip`, `stSpriteData`). Member variables: `m_` prefix + Hungarian-ish tags (`m_nCenterWidth` = int, `m_pMainSpritePos` = pointer, `m_str...` = string, `m_bOverride...` = bool).
- Struct-of-data types are prefixed `st` (`stSpriteData`, `stSpriteConfig`). Enums are UPPER_SNAKE with a type-prefixed member name (`SPRITE_MODE_CIRCULAR`) and end with a `_LAST` (and sometimes `_UNLISTED`) sentinel — keep sentinels last when extending.
- Doxygen-style `/** @brief ... */` comments precede non-trivial methods; this repo does use them (unlike the terser default) — follow the existing pattern when touching documented methods, but don't add commentary the surrounding code doesn't already have.
- Physical constants/magic numbers for movement math are `#define`d at the top of `spritemoveprocessing.cpp` with a `__` prefix (`__OVNI_SHIP_STEP`, `__STEP_360`) — follow that pattern rather than inlining new magic numbers.

## Known gotchas

- `SunLight::Input::KeyboardKey` vs raylib's own global `KeyboardKey` is a real footgun in engine code (see sunlight's own `CLAUDE.md`) — always fully qualify if you touch input handling.
- Sprite movement math in `spritemoveprocessing.cpp` assumes sprites share the SATELLITE sprite's aspect ratio (`__VIEWPORT_ASPECT_RATIO_W/H` are hardcoded); differently-sized sprites can show small displacement distortions at viewport borders (tracked as a TODO in that file).
