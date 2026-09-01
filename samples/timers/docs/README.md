# timers sample

Two independent background timers ticking at different rates, plus a key to stop/restart one of them. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/timers/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `F1` | Stop or restart the 1000ms timer (`reset_timer`/`set_timer`) — the 250ms timer keeps running regardless |

## What it shows

- [main.lua](../main.lua) — two `set_timer` calls (1000ms and 250ms), each just incrementing a plain Lua counter from its own background-thread callback.
- **The gotcha this sample exists to demonstrate correctly**: a `set_timer` callback runs on the timer's own background thread, not the main thread — per root `CLAUDE.md`'s own "Known gotchas", it's only safe to touch plain Lua data from inside one. This sample's callbacks do nothing but `counter = counter + 1`; `on_update` (always the main thread) is what reads those counters and calls `draw_text` to show them — the callback itself never calls an engine primitive.
- `reset_timer` stopping and `set_timer` re-registering the *same* id from a key press — safe because both calls happen from `on_update`, the main thread, not from inside a timer callback.

## Lua API reference

- [`set_timer`/`reset_timer`](https://popolony2k.github.io/scarab/lua-api/timers.html)
