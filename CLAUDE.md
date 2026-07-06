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
- `src/sprite/spritemoveprocessing.{h,cpp}` — one `Update<ShipType>Ship(stSpriteData&)` method per **not-yet-migrated** enemy type (see "Lua migration" below — migrated types' logic lives in Lua instead and this C++ path goes dead but stays compiled), each a small state machine keyed off `sprite.data.mode` / `sprite.data.fCounter` / `sprite.data.direction`. `UpdateEnemyBullet` handles both straight-line and Bresenham-seeker bullets.
- `src/lua/luaengine.{h,cpp}` — Lua ↔ `ScriptProcessor` bridge. Exposes script-callable functions (`sp_move_sprites_to_screen`, `sp_wait`, `sp_wait_queue_empty`, `sp_play_song`, `sp_load_stage`, `set_timer`, ...) and registers the C++ enums as Lua globals so scripts can reference e.g. `STATE_MOVE_OVNI_SHIP_CIRCULAR_TO_SCREEN` or `ID_FIRST_STAGE_BGM` by name. Every entry point that touches the shared `lua_State*` is serialized behind `LuaEngine::s_LuaMutex`, because `SunLight::Concurrent::Timer` (used by `set_timer`) invokes its callback on its own background thread — see "Known gotchas" below.
- `src/config/config.{h,cpp}` — loads JSON (via `nlohmann/json`, `NLOHMANN_DEFINE_TYPE_INTRUSIVE[_WITH_DEFAULT]`) describing sprite sets, sprite-moving parameters, sprite files, sound files, and stage files from `resources/configs/*.json`.

### Data flow for a stage

1. `resources/scripts/main.lua` calls `sp_load_stage(STAGE_FIRST)`.
2. `WorldEngine` loads the matching `.tmx` map and Lua script from `resources/configs/stagefile.json`.
3. The stage's Lua script (e.g. `1st_stage_corsair.lua`) drives the `ScriptProcessor` command queue — spawning enemy waves via `sp_move_sprites_to_screen(STATE_MOVE_..._TO_SCREEN)`, timing via `sp_wait`/`set_timer`, and music via `sp_play_song`.
4. `WorldEngine::OnCommand`'s `MOVE_SPRITES_TO_SCREEN_CMD` case tries Lua first (`LuaEngine::TryDispatchMoveSpritesToScreen`, which calls the optional Lua global `on_move_sprites_to_screen(stateId)`) and only falls back to the legacy `WorldEngine::MoveSpritesToScreen` + `SpriteMoveProcessing::Update<X>Ship` C++ path if Lua doesn't claim that state id — this is what lets enemy types migrate to Lua one at a time with zero stage-script changes.
5. Enemy/texture/sound parameters (velocity, hit count, textures, explosion/shoot overrides) come from `resources/configs/spriteset.json` and `sprite-moving-parms.json` either way, not hardcoded in C++.

**When adding a new enemy type that hasn't been migrated to Lua yet**: add to `SpriteType`/`SpriteFile` in `worlddefs.h`, add an `Update<X>Ship` in `spritemoveprocessing.{h,cpp}`, wire dispatch in `WorldBase`/`WorldEngine`, add moving-mode entries to `SpriteMovingStateHandlers`, and add config entries in the relevant `resources/configs/*.json` — C++ enum changes and JSON config changes go together. **For a type already migrated to Lua** (see below), add/change behavior in it's `resources/scripts/enemies/<type>.lua` module instead — no C++ changes needed.

### Lua migration (in progress, `feat/lua-refactor` branch)

A refactor is underway to move all enemy/sprite game logic out of C++ and into Lua, leaving C++ as a thin, reusable, game-agnostic engine layer. Migrated so far: **Satellite, Cylinder, Alien**. Still on the legacy C++ path above: Galileo, Nomad, Ovni, Octopus, and the player ship. Each migration is a self-contained vertical slice — the old C++ path for a type isn't touched, it just goes dead (unreferenced but still compiled) once it's Lua module claims the dispatch.

- `src/engine/` — new, game-agnostic `Engine::` namespace (deliberately **not** in the `sunlight` engine repo — see `games/caravellius/CMakeLists.txt` and sunlight's own `CLAUDE.md` for that boundary). Holds the generic pieces migrated enemies need: `SpritePool`/`SpriteHandle` (packed generation+index handle pool, replacing per-type C++ sprite stock queues), `LuaSpriteApi`/`LuaCameraApi`/`LuaInputApi`/`LuaTilemapApi`/`LuaSoundApi`/`LuaCollisionApi`/`LuaJsonApi` (the Lua-callable primitive surface), `LuaCollisionListener` (decodes packed handles from `Collider::GetPtrData()` for Lua-vs-Lua collisions only — `WorldBase::OnCollision` stays the single authority for any collision with at least one legacy side, calling into Lua directly for that side to avoid a listener-ordering race).
- `resources/scripts/enemies/` — the Lua-side modules:
  - `common.lua` — the shared `Enemies` registry (`register_update`, `claim`/`release` for collision dispatch, `register_wave_handler`, `register_count`) so each enemy module can hook the single global `on_update`/`collision_set_handler`/`on_move_sprites_to_screen` slots without clobbering each other. Also the `is_enemy_exploding`/`trigger_enemy_hit`/`get_active_enemy_count` bridges `WorldBase` calls into whichever module owns a given handle.
  - `movement.lua` — shared sine/straight-line movement math (`SpriteMoveProcessing::UpdateSprite`'s branches) and viewport-bounds computation, used by every migrated type that falls back to this movement after it's own approach phase.
  - `shooting.lua` / `linebullets.lua` — shared shoot-height threshold + reload-timing (`m_nShootMultiplierCounter` and the periodic, not-per-bullet, reload sweep) and the shared straight-line bullet pool (`SPRITE_TYPE_ENEMY_LINE_BULLET`), used by every migrated type that shoots straight-line bullets (Cylinder, Alien so far).
  - `satellite.lua`, `cylinder.lua`, `alien.lua` — the per-type modules themselves.
- Full plan, phase-by-phase status, and gotchas found along the way are tracked outside this repo (Claude's own project memory) since this is a Claude-assisted refactor in progress on a local-only branch.

## Code style (existing convention — match it, don't "fix" it)

- Scope resolution operator is always **space-padded**: `Caravellius :: World :: WorldEngine`, `SunLight :: Renderer :: TileMapRenderer`, not `Caravellius::World::WorldEngine`.
- Member functions/types: PascalCase (`UpdateOvniShip`, `stSpriteData`). Member variables: `m_` prefix + Hungarian-ish tags (`m_nCenterWidth` = int, `m_pMainSpritePos` = pointer, `m_str...` = string, `m_bOverride...` = bool).
- Struct-of-data types are prefixed `st` (`stSpriteData`, `stSpriteConfig`). Enums are UPPER_SNAKE with a type-prefixed member name (`SPRITE_MODE_CIRCULAR`) and end with a `_LAST` (and sometimes `_UNLISTED`) sentinel — keep sentinels last when extending.
- Doxygen-style `/** @brief ... */` comments precede non-trivial methods; this repo does use them (unlike the terser default) — follow the existing pattern when touching documented methods, but don't add commentary the surrounding code doesn't already have.
- Physical constants/magic numbers for movement math are `#define`d at the top of `spritemoveprocessing.cpp` with a `__` prefix (`__OVNI_SHIP_STEP`, `__STEP_360`) — follow that pattern rather than inlining new magic numbers.

## Known gotchas

- `SunLight::Input::KeyboardKey` vs raylib's own global `KeyboardKey` is a real footgun in engine code (see sunlight's own `CLAUDE.md`) — always fully qualify if you touch input handling.
- Sprite movement math in `spritemoveprocessing.cpp` assumes sprites share the SATELLITE sprite's aspect ratio (`__VIEWPORT_ASPECT_RATIO_W/H` are hardcoded); differently-sized sprites can show small displacement distortions at viewport borders (tracked as a TODO in that file).
- The `SPRITE_MODE_SINE_*_FAST` cases use `__STEP_360 * __ctRads180`, while `*_SLOW` uses `__STEP_180 * __ctRads360` — easy to transcribe backwards (it was, independently, in two different Lua ports before `movement.lua` consolidated the fix) since the constants look superficially similar; FAST should oscillate roughly 4x faster than SLOW.
- `stCoordinate2D`/`pPos` is top-left anchored, not center-anchored (confirmed via both the render clip rect and the AABB collision code) — a formula like `pos - height/2` doesn't mean "half a sprite-height above center", it means "half a sprite-height above the sprite's own top edge". `WorldEngine::UpdateScreenSprites`' initial child-bullet position (`pos + width/2, pos - height/2`) is never actually rendered in the original; it's an invisible placeholder later overwritten in `UpdateEnemyBullet` to `parentPos + (3,3)` at the moment the bullet actually becomes visible - worth checking against before assuming any position math involving `pPos` matches visual intuition.
- `SunLight::Concurrent::Timer::Start()` runs it's callback on it's own background thread; `lua_State` isn't thread-safe, so any code calling into Lua from both the main thread and a timer callback needs to serialize via a mutex (see `LuaEngine::s_LuaMutex`). Watch for deadlock if a mutex-holding call can trigger `Timer::Stop()` (which `std::thread::join()`s) — the timer thread must not block on the same mutex, or a `join()` from the mutex-holding thread deadlocks against it (fixed via `try_lock` in the timer callback rather than a blocking lock).
