# scripting sample

A forever-repeating "wave" cycle built entirely from `ScriptProcessor`'s queued `sp_*` commands — no sprites, no map. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/scripting/project.json
```

## What it shows

- [main.lua](../main.lua) — the queue itself: `sp_add_label(1)` marks a position, `sp_move_sprites_to_screen(1)` dispatches to the optional `on_move_sprites_to_screen(stateId)` hook (which must return `true` to "claim" the id), `sp_wait(2000)` paces it, `sp_wait_queue_empty()` gates on "no active enemies" — this sample defines no `get_active_enemy_count()` at all, so `EngineHost::CheckSpritesQueueEmpty` treats the screen as clear, but only at its next periodic 2-second check, not the moment the gate is reached — then `sp_wait(500)` and `sp_goto_label(1)` loop it forever.
- `on_move_sprites_to_screen` fires once almost immediately (before the first `sp_wait` is even reached) and then every 4 seconds, once per full loop: 2 s of `sp_wait`, 1.5 s until the gate's next periodic check, 0.5 s of `sp_wait` (measured on the headless virtual clock: 0.03 s, 4.55 s, 8.55 s, 12.55 s, ...). Watch the on-screen counter.
- **`sp_load_stage` is deliberately not demonstrated** — it requires a real `on_load_stage(stageId)` hook that actually loads a map/script and returns `true`, or the engine logs an error every time the queue reaches it; out of scope for a small, focused sample.

## Lua API reference

- [`sp_add_label`/`sp_goto_label`/`sp_move_sprites_to_screen`/`sp_wait`/`sp_wait_queue_empty`](https://popolony2k.github.io/scarab/lua-api/scripting.html)
- [`on_move_sprites_to_screen`](https://popolony2k.github.io/scarab/lua-api/callbacks.html)
