# Scripting — the `ScriptProcessor` command queue

*Implemented in* `src/lua/luascriptingapi.cpp` *and* `SunLight::Scripting::ScriptProcessor` *(sunlight engine).*

Stage scripts don't drive gameplay by calling functions directly and waiting for them to return — they push commands onto a queue, and the engine drains that queue one command at a time, once per frame, from `EngineHost::RunStageStateHandler`. This is what lets `sp_wait(2000)` mean "pause the *queue*, not the whole engine" — sprites keep moving, input keeps being read, only the next queued command waits.

Every `sp_*` function in this file just appends to that queue; none of them block or take effect immediately.

## `sp_wait(milliseconds)`

Pause queue processing for `milliseconds` before the next queued command runs. Purely queue-side — rendering, physics, and every other per-frame system keep running.

```lua
sp_wait(2000)  -- 2 second pause, non-blocking
```

## `sp_clear()`

Empty the entire command queue immediately, discarding everything not yet processed. Used once at startup (`main.lua`) before the first `sp_load_stage`, to guarantee a clean queue regardless of what earlier `dofile`s may have queued.

```lua
sp_clear()
```

## `sp_wait_queue_empty()`

Queue a command that blocks all *later* queued commands until the engine reports the screen is clear of active enemies (`EngineHost::CheckSpritesQueueEmpty`, driven by the game's own `get_active_enemy_count()` — see [callbacks.md](callbacks.md)). The classic "don't spawn wave 2 until wave 1 is dead" gate.

```lua
sp_move_sprites_to_screen(STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM)
sp_wait_queue_empty()
sp_move_sprites_to_screen(STATE_MOVE_CYLINDER_SHIP_TO_SCREEN_LEFT_SIDE)  -- only starts once wave 1 is gone
```

## `sp_move_sprites_to_screen(stateId)`

Queue a wave-spawn command. `stateId` is an opaque integer — Caravellius defines its own meaningful names for these in `src/wavestates.lua` (`STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM`, etc.); the engine itself doesn't know or care what any specific id means; it's forwarded verbatim to the game's own `on_move_sprites_to_screen(stateId)` hook (see [callbacks.md](callbacks.md)).

```lua
sp_move_sprites_to_screen(STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM)
```

## `sp_add_label(id)` / `sp_goto_label(id)`

Mark a position in the queue (`sp_add_label`) and later jump the queue's read position back (or forward) to it (`sp_goto_label`) — the queue's only looping construct, since it's a linear command sequence otherwise. `id` is any integer you choose; it just has to match between the label and the goto.

```lua
sp_add_label(1)

sp_move_sprites_to_screen(STATE_MOVE_SATELLITES_TO_SCREEN_RANDOM)
sp_wait_queue_empty()

sp_goto_label(1)  -- repeats the wave forever
```

## `sp_load_stage(stageId)`

Queue a stage load. `stageId` is opaque to the engine the same way wave-spawn ids are — Caravellius defines `STAGE_FIRST`/`STAGE_LAST` in `src/stageids.lua`. When the queue reaches this command, the engine calls the game's `on_load_stage(stageId)` hook (see [callbacks.md](callbacks.md)), which is expected to load a map and `dofile` a stage script. There is no built-in stage concept beyond this — the engine only ferries the id through.

```lua
sp_load_stage(STAGE_FIRST)
```

## Song commands via the queue

`sp_play_song`/`sp_pause_song`/`sp_stop_song`/`sp_resume_song` are documented in [sound.md](sound.md) alongside their non-queued (`play_song`/`pause_song`/...) counterparts, since the two forms only differ in *when* they take effect (queued vs. immediate), not in what they do.
