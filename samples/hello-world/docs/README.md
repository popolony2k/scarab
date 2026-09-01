# hello-world sample

The smallest possible Scarab game: opens a window, sets its title, and draws one line of text every frame — nothing else. No tilemap, no sprites, no sound. Every other sample under [samples/](../../) builds on this same shape (a `project.json` + a `main.lua`), one Lua API category at a time — see [samples/README.md](../../README.md) for the full list.

## Running

From the repo root:

```shell
./build/scarab samples/hello-world/project.json
```

**Not** `cd build && ./scarab ../samples/hello-world/project.json` — a `..`-relative entry path fails (`cannot open ../samples/hello-world/main.lua`), since the mount-based filesystem every resource load routes through (see the root [README.md](../../../README.md#running-rocket)) rejects `..` path segments. Run from the repo root as shown above, or pass an absolute path instead.

## What it shows

- [project.json](../project.json) — a minimal project file: just `main_script`, pointing at `main.lua` in the same directory. `main_script` resolves relative to the project file's own directory (not `APP_DIR`), so this whole sample folder is relocatable as a unit.
- [main.lua](../main.lua) — calls `app_set_name` once, queues a single trivial `sp_wait(1)` (see below), then defines `on_update(dt)`, the optional per-frame hook the engine calls every frame if it exists as a global function. Draws one line of text via `draw_text`, which has to be called every frame to keep showing — it's not "issue once and it persists". `set_font` is deliberately never called — until it is, `draw_text`/`measure_text` fall back to the engine's own built-in default font, so this sample needs zero font asset of its own.

## A real gotcha this sample exists to surface

Every entry script must queue **at least one** `ScriptProcessor` (`sp_*`) command before it finishes running — sunlight's own `ScriptProcessor::Compile()`, called right after the whole entry script executes, returns `false` if the queue is still empty, and a failed compile is a fatal engine-startup error (confirmed live: an earlier version of this exact sample with no `sp_*` call at all failed to start with `Error compiling script commands`). `hello-world` has nothing worth sequencing, so it queues a single harmless `sp_wait(1)` — a 1-millisecond, non-blocking pause — purely to satisfy this requirement. Every other sample in this tree needs at least one real or trivial `sp_*` call too, for the same reason.

## Lua API reference

- [`app_set_name`](https://popolony2k.github.io/scarab/lua-api/app.html)
- [`sp_wait`](https://popolony2k.github.io/scarab/lua-api/scripting.html)
- [`draw_text`](https://popolony2k.github.io/scarab/lua-api/text.html)
- [`on_update`](https://popolony2k.github.io/scarab/lua-api/callbacks.html)
