# Engine callbacks — hooks the engine calls *into*

Every other page in this reference documents functions your Lua script calls *into* the engine. This page is the reverse direction: global Lua functions the engine looks up and calls *from* C++, if you define them. Every one of these is optional — the engine checks whether the global exists and is a function before calling it, and silently no-ops if it isn't defined (except `on_load_stage`, see below).

Define these as plain global functions (not table members) — the engine looks them up by exact name via `lua_getglobal`.

## `on_update(deltaMilli)`

Called once per frame, after the engine's own per-frame bookkeeping (script queue processing, sprite pool update). This is the main per-frame hook — virtually all real-time game logic (movement, input polling, timers you're tracking by hand) lives here.

Caravellius never defines a single monolithic `on_update` — instead `src/enemies/common.lua` defines the *only* real `on_update`, which just fans out to every module that registered itself via `Enemies.register_update(fn)`. If you're adding a new per-frame system, register with that dispatcher rather than defining a second `on_update` (which would silently replace, not stack with, the first).

```lua
function on_update(dt)
  print("frame delta: " .. dt .. "ms")
end
```

## `on_move_sprites_to_screen(stateId) -> handled`

Called when the `ScriptProcessor` queue reaches an `sp_move_sprites_to_screen(stateId)` command (see [scripting.md](scripting.md)). Return `true` if you handled this `stateId` (spawned something); returning `false`/nothing tells the engine this particular wave-spawn id wasn't recognized.

```lua
function on_move_sprites_to_screen(stateId)
  if stateId == STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM then
    -- spawn logic here
    return true
  end
  return false
end
```

## `on_load_stage(stageId) -> success`

Called when the queue reaches an `sp_load_stage(stageId)` command. Unlike the other hooks, this one is expected to always be defined and to always claim a valid `stageId` — there's no C++-side fallback if it isn't. Typically loads a map (`tilemap_load_map`) and `dofile`s a stage script.

```lua
function on_load_stage(stageId)
  if stageId == STAGE_FIRST then
    tilemap_load_map(BASE_PATH .. "tilemap/corsair/corsair.tmx", MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM)
    dofile(BASE_PATH .. "scripts/stages/1st_stage_corsair.lua")
    return true
  end
  return false
end
```

## `get_active_enemy_count() -> count`

Called periodically by the engine to resolve `sp_wait_queue_empty()` (see [scripting.md](scripting.md)) — the queue only unblocks once this returns `0`. If you never define this global at all, the engine treats it as always reporting `0` active enemies, so `sp_wait_queue_empty()` unblocks immediately rather than actually waiting — define this accurately if your stage scripts rely on `sp_wait_queue_empty` to gate wave pacing.

```lua
function get_active_enemy_count()
  local total = 0
  for _, list in pairs(activeEnemiesByType) do
    total = total + #list
  end
  return total
end
```

## The collision handlers are documented separately

`collision_set_handler`/`collision_set_tile_handler` follow the same "engine calls into a Lua global" shape as everything on this page, but since they're registered via an explicit function call rather than just defining a magic global name, they're documented alongside the rest of the collision API in [collision.md](collision.md).
