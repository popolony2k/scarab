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

C++ is a thin, game-agnostic engine layer; all game logic (every enemy type, the player ship, config/stage bootstrap) lives in Lua. This is the end state of a completed 9-phase refactor (see "Lua migration" below) — there is no legacy per-sprite C++ state-machine path anymore.

- `src/main.cpp` — entry point. Builds a `SunLight::Renderer::TileMapRenderer` (window/game loop owner) and a `Caravellius::World::WorldEngine`, wires the latter as a tile-map listener, then `Start()`/`Run()`/`Stop()`.
- `src/world/worldengine.{h,cpp}` — `WorldEngine`: sound playback, the `ScriptProcessor`/`LuaEngine` wiring, the engine's own state machine (init/run-stage/fatal-error handlers), and `OnCommand` dispatch. Owns no game logic at all — `InitEngineStateHandler` just runs `main.lua` and transitions to `STATE_STAGE_RUNNING`; `OnCommand`'s `LOAD_STAGE_CMD`/`MOVE_SPRITES_TO_SCREEN_CMD` cases both give Lua first (and only) refusal via `LuaEngine::TryDispatchLoadStage`/`TryDispatchMoveSpritesToScreen`, reporting an error if Lua doesn't claim a valid id (there's no C++ fallback left to drop to). `CheckSpritesQueueEmpty` is the one piece of per-frame bookkeeping that isn't state-machine or Lua-dispatch — a periodic (2000ms) check that resolves `sp_wait_queue_empty()` via `LuaEngine::GetActiveEnemyCount()`. Not a collision listener — `Engine::Lua::LuaCollisionListener` (registered by `LuaEngine::Init`) is the sole collision authority now that every sprite is Lua/`SpritePool`-owned. (This used to be split across a `WorldBase` base class + `WorldEngine` subclass; merged since nothing else ever subclassed `WorldBase`.) Sprite layers map to fixed Tiled layer IDs (`LAYER_PLAYER_SHIP=1`, `LAYER_ENEMIES_SHIPS=2`, `LAYER_PLAYER_SHIP_BULLETS=3`, `LAYER_ENEMY_BULLETS=4`) — these **must** exist in any Tiled map used.
- `src/world/worlddefs.h` — just the three enums still referenced by name from Lua: `SpriteMovingStateHandlers` (wave-spawn state ids, e.g. `STATE_MOVE_OVNI_SHIP_CIRCULAR_TO_SCREEN`), `SoundUniqueId` (`ID_*` sound ids), `StageFile` (`STAGE_FIRST`/`STAGE_LAST`) — all registered as Lua globals by `LuaEngine::RegisterEnumsToLuaAsGlobals`.
- `src/lua/luaengine.{h,cpp}` — Lua ↔ `ScriptProcessor` bridge. Exposes script-callable functions (`sp_move_sprites_to_screen`, `sp_wait`, `sp_wait_queue_empty`, `sp_play_song`, `sp_load_stage`, `set_timer`, ...) and registers the three enums above as Lua globals. `TryDispatchMoveSpritesToScreen`/`TryDispatchLoadStage` are Lua's sole dispatch path for wave-spawn/stage-load queue commands. Every entry point that touches the shared `lua_State*` is serialized behind `LuaEngine::s_LuaMutex`, because `SunLight::Concurrent::Timer` (used by `set_timer`) invokes its callback on its own background thread — see "Known gotchas" below. `RunFile` prints the actual Lua error message on failure.
- `src/engine/` — the game-agnostic `Engine::` namespace (deliberately **not** in the `sunlight` engine repo — see `games/caravellius/CMakeLists.txt` and sunlight's own `CLAUDE.md` for that boundary), the runtime backing for every sprite in the game. `SpritePool`/`SpriteHandle` (packed generation+index handle pool), `LuaSpriteApi` (sprite CRUD + `sprite_set_animation_mode` for runtime ANIMATE_LEFT/RIGHT/CENTER switching — the player ship is the only thing that needs this, enemies set their mode once at configure time), `LuaCameraApi`/`LuaInputApi` (incl. `input_add_gamepad`)/`LuaTilemapApi`/`LuaSoundApi`/`LuaCollisionApi`/`LuaJsonApi` (the Lua-callable primitive surface), `LuaCollisionListener` (decodes packed handles from `Collider::GetPtrData()` — the sole collision listener in the game now).

### Data flow for a stage

1. `resources/scripts/main.lua` runs `Bootstrap.load_sounds()` (reads `resources/configs/soundfile.json` via `load_json`, calls `sound_load` for each entry), dofiles every Lua module, then calls `sp_load_stage(STAGE_FIRST)`.
2. That queues a `LOAD_STAGE_CMD`; `WorldEngine::OnCommand` calls `LuaEngine::TryDispatchLoadStage`, which invokes Lua's `on_load_stage(stageId)` (`bootstrap.lua`) — it reads `resources/configs/stagefile.json`, calls `tilemap_load_map(...)`, then `dofile`s the stage's own Lua script (e.g. `1st_stage_corsair.lua`). C++ still owns `ScriptProcessor::Compile()` and the state-machine transition to `STATE_STAGE_RUNNING` afterward (neither is reachable from Lua).
3. The stage's Lua script drives the `ScriptProcessor` command queue — spawning enemy waves via `sp_move_sprites_to_screen(STATE_MOVE_..._TO_SCREEN)`, timing via `sp_wait`/`set_timer`, and music via `sp_play_song`.
4. `WorldEngine::OnCommand`'s `MOVE_SPRITES_TO_SCREEN_CMD` case calls `LuaEngine::TryDispatchMoveSpritesToScreen`, which calls the optional Lua global `on_move_sprites_to_screen(stateId)` — every valid wave-spawn state id is claimed by some enemy module's `register_wave_handler`.
5. Enemy/texture/sound parameters (velocity, hit count, textures, explosion/shoot overrides) come from `resources/configs/spriteset.json` and `spritefile.json`, read directly by Lua's `spriteconfig.lua`.

**Adding a new enemy type** (or changing an existing one): everything is Lua. Add/change a `resources/scripts/enemies/<type>.lua` module (following the shape of the existing ones — `common.lua`'s `Enemies` registry for update/collision/wave-spawn dispatch, `spriteconfig.lua` for velocity/hit-count/texture/sound config, `movement.lua`/`shooting.lua`/`linebullets.lua`/`seekerbullets.lua` for shared math/bullet pools), plus config entries in `resources/configs/spriteset.json`/`spritefile.json`/`sprite-moving-parms.json`. No C++ changes needed.

### Lua migration (complete, all 9 phases — `feat/lua-refactor` branch)

A refactor moved all enemy/sprite/player game logic and config/stage bootstrap out of C++ and into Lua, then deleted the C++ that became dead as a result, leaving C++ as a thin, reusable, game-agnostic engine layer (see `src/engine/` above). Every phase of the original 9-phase plan is done: Satellite, Cylinder, Alien, Octopus, Ovni, Galileo, Nomad, the player ship (`player.lua`), config/stage loading (`spriteconfig.lua`/`bootstrap.lua`), and (Phase 9) the bulk deletion of `src/config/`, `src/sprite/spritemoveprocessing.{h,cpp}`, the legacy sprite queues and collision listener that used to live in a separate `WorldBase` class (since merged into `WorldEngine`, see below), and `WorldEngine`'s config/stage/input/move-to-screen methods.

Enemy migrations were self-contained vertical slices — the old C++ path for a type wasn't touched, it just went dead (unreferenced but still compiled) once its Lua module claimed the dispatch, until Phase 9 deleted it. The player ship and the config/stage bootstrap were **not** vertical-sliced this way — both are singular, non-per-type mechanisms where the old and new paths can't coexist (e.g. C++ input handlers and Lua polling would both drive the same physical ship), so those were full switch-overs: implement the Lua side, verify it works, then unwire the old C++ call sites in the same change (with the dead bodies deleted later, in Phase 9).

- `resources/scripts/spriteconfig.lua` — generic reader over `spriteset.json`/`spritefile.json` (via `load_json`) that every enemy module and `player.lua` use for velocity/max-hit-count/texture-setup/sound-ids, replacing what used to be hardcoded Lua literals kept in sync with the JSON by hand.
- `resources/scripts/bootstrap.lua` — sound loading (`soundfile.json`) and the `on_load_stage(stageId)` hook (`stagefile.json` + `tilemap_load_map` + `dofile`) described above.
- `resources/scripts/player.lua` — the player ship: movement (keyboard + gamepad button *and* left-analog-stick with deadzone), shooting, and hit/explosion/blink, all via the same generic sprite pool/collision primitives every enemy uses.
- `resources/scripts/enemies/` — the enemy-side modules:
  - `common.lua` — the shared `Enemies` registry (`register_update`, `claim`/`release` for collision dispatch, `register_wave_handler`, `register_count`) so each module can hook the single global `on_update`/`collision_set_handler`/`on_move_sprites_to_screen` slots without clobbering each other. Also `get_active_enemy_count` (feeds `WorldEngine::CheckSpritesQueueEmpty`) and the `collision_set_handler` callback that mirrors the old mutual-immunity check (see gotchas below).
  - `movement.lua` — shared sine/straight-line/vertical-sine movement math and viewport-bounds computation.
  - `shooting.lua` — shared shoot-height threshold + periodic (not per-bullet) reload sweep.
  - `linebullets.lua` / `seekerbullets.lua` — the shared straight-line and Bresenham-seeker bullet pools.
  - `satellite.lua`, `cylinder.lua`, `alien.lua`, `octopus.lua`, `ovni.lua`, `galileo.lua`, `nomad.lua` — the per-type modules themselves.
- Full plan, phase-by-phase status, and gotchas found along the way are tracked outside this repo (Claude's own project memory) since this was a Claude-assisted refactor done on a local-only branch.

## Code style (existing convention — match it, don't "fix" it)

- Scope resolution operator is always **space-padded**: `Caravellius :: World :: WorldEngine`, `SunLight :: Renderer :: TileMapRenderer`, not `Caravellius::World::WorldEngine`.
- Member functions/types: PascalCase (`RunStageStateHandler`, `LuaEngine`). Member variables: `m_` prefix + Hungarian-ish tags (`m_nCenterWidth` = int, `m_pMainSpritePos` = pointer, `m_str...` = string, `m_bOverride...` = bool).
- Struct-of-data types are prefixed `st`. Enums are UPPER_SNAKE with a type-prefixed member name (`SPRITE_MODE_CIRCULAR`) and end with a `_LAST` (and sometimes `_UNLISTED`) sentinel — keep sentinels last when extending. Watch for a sentinel that doubles as a real value (see the `StageFile::STAGE_LAST` gotcha below) before assuming a registration/iteration loop's bound should be exclusive.
- Doxygen-style `/** @brief ... */` comments precede non-trivial methods; this repo does use them (unlike the terser default) — follow the existing pattern when touching documented methods, but don't add commentary the surrounding code doesn't already have.
- Physical constants/magic numbers for movement math are local Lua constants inside each `resources/scripts/enemies/*.lua` module (or shared ones in `movement.lua`), following the same intent the old C++ `__`-prefixed `#define`s had (this repo's Lua ports keep a `-- matches __OVNI_SHIP_STEP` style comment pointing back to the original C++ name where relevant) — don't inline unexplained magic numbers.

## Known gotchas

- `SunLight::Input::KeyboardKey` vs raylib's own global `KeyboardKey` is a real footgun in engine code (see sunlight's own `CLAUDE.md`) — always fully qualify if you touch input handling.
- The `SPRITE_MODE_SINE_*_FAST` cases use `STEP_360 * RADS_180`, while `*_SLOW` uses `STEP_180 * RADS_360` (`movement.lua`) — easy to transcribe backwards (it was, independently, in two different Lua ports before `movement.lua` consolidated the fix) since the constants look superficially similar; FAST should oscillate roughly 4x faster than SLOW.
- `stCoordinate2D`/sprite position is top-left anchored, not center-anchored (confirmed via both the render clip rect and the AABB collision code in the original C++) — a formula like `pos - height/2` doesn't mean "half a sprite-height above center", it means "half a sprite-height above the sprite's own top edge". Worth checking against before assuming any position math matches visual intuition.
- `SunLight::Concurrent::Timer::Start()` runs it's callback on it's own background thread; `lua_State` isn't thread-safe, so any code calling into Lua from both the main thread and a timer callback needs to serialize via a mutex (see `LuaEngine::s_LuaMutex`). Watch for deadlock if a mutex-holding call can trigger `Timer::Stop()` (which `std::thread::join()`s) — the timer thread must not block on the same mutex, or a `join()` from the mutex-holding thread deadlocks against it (fixed via `try_lock` in the timer callback rather than a blocking lock).
- Acquiring/positioning/`sprite_add_to_layer`-ing a sprite from a Lua module's `init()` fails silently and leaves it invisible, if `init()` runs before the map is actually loaded — `main.lua`'s `dofile`/`init()` calls all happen synchronously *before* `sp_load_stage`'s queued `LOAD_STAGE_CMD` has run `LoadMap` (that only happens on the first `STATE_STAGE_RUNNING` tick). Every enemy module sidesteps this by only ever acquiring/adding sprites from a wave-spawn handler triggered later by the stage script; `player.lua` has no such natural trigger, so it defers real spawn to the first `Player.update()` call instead (see it's own comments).
- The player's original C++ shoot cooldown (now ported faithfully in `player.lua`) stamps it's "last shot" timestamp on *every* poll, not just when a bullet actually fires — since it's polled every render frame while the key is held, this means holding the key down only ever fires once (the ~16ms per-frame gap never exceeds the cooldown threshold again), and only releasing/re-pressing fires again. Looks like a bug but is what gives the felt semi-auto rate limit; a version that only stamps the timestamp on an actual shot produces true full-auto instead (reported during this refactor as "machine gun").
- `player.lua` drives movement from *both* the physical gamepad D-pad buttons *and* the left analog stick (translated to virtual D-pad presses with a 0.1 deadzone, matching the original C++'s `TileMapRenderer::HandleUserInput`) — polling only `input_is_gamepad_button_down` misses stick-only controllers entirely; also poll `input_get_gamepad_axis` with the same deadzone.
- The pure-Lua `collision_set_handler` (`common.lua`) must OR immunity across *both* colliding sides (skip the whole collision if either side is already exploding/blinking) — without it, two sprites overlapping for several consecutive frames (e.g. the player ramming a multi-hit enemy) re-trigger `on_hit` on the not-yet-maxed side every one of those frames, racking up it's full hit count within a single touch instead of requiring separate encounters over time.
- `RegisterEnumsToLuaAsGlobals`'s per-enum registration loops assume every `*_LAST` sentinel is a pure bound never used as a real value — true for `SpriteMovingStateHandlers`/`SoundUniqueId`, but **not** for `StageFile::STAGE_LAST`, which doubles as an actual, loadable stage id (real `stagefile.json` entry). The registration loop must include it (`<=`, not `<`) or that Lua global silently never gets set — a real bug only surfaced once Lua code (`bootstrap.lua`) needed to reference `STAGE_LAST` by name for the first time in the game's history.
