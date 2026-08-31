# Lua Engine API Reference

This is the reference for every Lua-callable primitive Scarab (the C++ engine layer) exposes to game scripts. It documents the **engine surface only** — generic, game-agnostic functions registered from C++ (`src/lua/Lua*Api.cpp`, `src/lua/luaengine.cpp`). It does **not** document any specific game's own Lua modules built on top of this API — those are game content, not part of the engine itself.

Every function listed here is available in any Lua script the engine runs, from the moment `LuaEngine::Init` finishes (before `main.lua`'s first line executes) — there is no separate "require" step.

## Pages

Generated straight from `@luaname`/`@luadoc`/`@luaexample` tags on each primitive's own C++ doc comment in `src/lua/` (see `scripts/generate_lua_api_docs.py`) — this page is the one hand-written exception (see the note at the very bottom).

| Page | Covers |
|---|---|
| [Scripting](https://popolony2k.github.io/scarab/lua-api/scripting.html) | The `ScriptProcessor` command queue — `sp_wait`, `sp_clear`, `sp_wait_queue_empty`, `sp_move_sprites_to_screen`, `sp_add_label`/`sp_goto_label`, `sp_load_stage` |
| [Sound](https://popolony2k.github.io/scarab/lua-api/sound.html) | Loading and playing sounds — `sound_*` (direct playback state) and the queued/direct song commands (`sp_play_song`/`play_song` and friends) |
| [Timers](https://popolony2k.github.io/scarab/lua-api/timers.html) | `set_timer`/`reset_timer` — background-thread periodic/delayed callbacks |
| [App](https://popolony2k.github.io/scarab/lua-api/app.html) | Application-level primitives — `app_set_name` (window title), `app_set_fullscreen`/`app_get_fullscreen`, `app_set_draw_fps`/`app_get_draw_fps`, `app_set_window_resizeable`/`app_get_window_resizeable` |
| [Text](https://popolony2k.github.io/scarab/lua-api/text.html) | Font loading and screen-space text rendering — `set_font`, `draw_text`, `measure_text`, `screen_get_width`/`screen_get_height` |
| [Camera](https://popolony2k.github.io/scarab/lua-api/camera.html) | Camera panning and zoom — `camera_*`, `zoom_*`, `viewport_get_*` |
| [Input](https://popolony2k.github.io/scarab/lua-api/input.html) | Keyboard and gamepad polling — `input_*`, plus the `KEY_*`/`GAMEPAD_BUTTON_*`/`GAMEPAD_AXIS_*`/`CONTROLLER_*` constants |
| [Tile map](https://popolony2k.github.io/scarab/lua-api/tilemap.html) | Loading maps and reading/writing layers and tiles — `tilemap_*`, plus `MAP_ALIGNMENT_*` |
| [Sprites](https://popolony2k.github.io/scarab/lua-api/sprite.html) | The sprite handle pool — `pool_register_type`, `sprite_*`, plus `TEXTURE_ANIMATION_MODE_*` |
| [Collision](https://popolony2k.github.io/scarab/lua-api/collision.html) | Collision rules and handlers — `collision_*` |
| [JSON](https://popolony2k.github.io/scarab/lua-api/json.html) | `load_json` — the generic JSON→Lua config bridge |
| [Engine callbacks](https://popolony2k.github.io/scarab/lua-api/callbacks.html) | The Lua-side hooks the engine calls **into** — `on_update`, `on_move_sprites_to_screen`, `on_load_stage`, `get_active_enemy_count` |

## Globals

Besides the functions above, `LuaEngine::Init` (`luaengine.cpp`) sets one plain Lua global directly, before any game script runs:

### `APP_DIR`

A string: the directory the running executable actually lives in (via `IEngine::GetApplicationDirectory()`, which wraps raylib's own `GetApplicationDirectory()`). Set once, read-only in practice (nothing re-sets it, but nothing stops a script from overwriting it either — don't).

This is deliberately the *only* path decision Scarab makes — where a game's own resources actually live relative to it is the game's decision, not the engine's. Caravellius's own `main.lua` uses it to define its own convention:

```lua
-- main.lua, first line
BASE_PATH = APP_DIR .. "resources/"
```

Every other Caravellius module reads `BASE_PATH`, never `APP_DIR` directly — a different game could lay its resources out completely differently and only this one line would need to change.

A few other globals exist purely as internal plumbing between `LuaEngine`/the `Lua*Api` classes and are **not** meant to be read or written by game scripts directly: `scriptProcessorPtr`, `timerMapPtr`, `tileMapPtr`, `soundManagerPtr`, `spritePoolPtr` (opaque light-userdata pointers), and `__collision_handler`/`__collision_tile_handler` (the `__`-prefixed callback storage `collision_set_handler`/`collision_set_tile_handler` install). Interact with these through the documented functions, not by naming the globals themselves.

## Conventions used in this reference

- **Signature line** shows the Lua call shape: `function_name(param1, param2) -> return1, return2`. A function with no return value shows no `->`.
- **Handle** means an opaque integer (`SpriteHandle`) returned by `sprite_acquire` — never construct one by hand, and `0` always means "invalid" (`sprite_acquire` returning `0` means the pool is exhausted).
- Every example is a minimal, runnable snippet — most assume a sprite pool type has already been registered and a map is already loaded, since that's the real order engine code runs in.
- Parameter types follow Lua's own dynamic typing — "int"/"number"/"string"/"bool"/"function" describe what the engine expects, not a Lua-enforced type.

---

This file is the one page in this directory kept hand-written on purpose — it has no single-primitive or single-class C++ anchor to hang tags on (it's an index/globals-list/conventions guide about the whole Lua environment, not about any one entity), unlike every page linked above, which is generated from source. It also doubles as the source for [popolony2k.github.io/scarab/lua-api/](https://popolony2k.github.io/scarab/lua-api/)'s own landing page (`scripts/generate_lua_api_docs.py`'s `render_readme_html`), rebuilt on every push to `main` alongside the rest of the site.
