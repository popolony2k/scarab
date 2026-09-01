# scripting sample

A forever-repeating "wave" cycle built entirely from `ScriptProcessor`'s queued `sp_*` commands — no sprites, no map. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/scripting/project.json
```

## What it shows

- [main.lua](../main.lua) — the queue itself: `sp_add_label(1)` marks a position, `sp_move_sprites_to_screen(1)` dispatches to the optional `on_move_sprites_to_screen(stateId)` hook (which must return `true` to "claim" the id), `sp_wait(2000)` paces it, `sp_wait_queue_empty()` gates on "no active enemies" — resolving almost immediately here, since this sample defines no `get_active_enemy_count()` at all, so `EngineHost::CheckSpritesQueueEmpty` treats the screen as already clear on its next ~2s check — then `sp_wait(500)` and `sp_goto_label(1)` loop it forever.
- `on_move_sprites_to_screen` fires once almost immediately (before the first `sp_wait` is even reached) and then again roughly every ~4.5 seconds thereafter, once per full loop — watch the on-screen counter.
- **`sp_load_stage` is deliberately not demonstrated** — it requires a real `on_load_stage(stageId)` hook that actually loads a map/script and returns `true`, or the engine logs an error every time the queue reaches it; out of scope for a small, focused sample.

## Lua API reference

- [`sp_add_label`/`sp_goto_label`/`sp_move_sprites_to_screen`/`sp_wait`/`sp_wait_queue_empty`](https://popolony2k.github.io/scarab/lua-api/scripting.html)
- [`on_move_sprites_to_screen`](https://popolony2k.github.io/scarab/lua-api/callbacks.html)
