# Scarab samples

Small, focused Lua scripts that each demonstrate one corner of Scarab's Lua API at a time — the way sunlight's own [samples](https://github.com/popolony2k/sunlight/tree/main/samples) work, adapted for an engine where the entire game is Lua rather than C++: every sample here is just a `project.json` + `.lua` script (+ any resources it needs), run directly against the already-built `scarab` executable — nothing to compile, no build step of its own. See the [Lua API reference](https://popolony2k.github.io/scarab/lua-api/) for the full primitive documentation each sample only shows a slice of.

## Running a sample

From the repo root:

```shell
./build/scarab samples/<sample-name>/project.json
```

**Not** `cd build && ./scarab ../samples/<sample-name>/project.json` — a `..`-relative entry path fails outright (the mount-based filesystem every resource load routes through, see the root [README.md](../README.md#running-rocket), rejects `..` path segments). Run from the repo root as shown above, or pass an absolute path instead.

## Getting started

* [hello-world](hello-world/docs/README.md) — the smallest possible Scarab game: opens a window and draws one line of text every frame. No tilemap, no sprites, no sound. Start here.

## By Lua API category

Everything past `hello-world` is grouped by exactly the categories the [Lua API reference](https://popolony2k.github.io/scarab/lua-api/) itself is split into — each sample links back to its own category page for the full primitive docs. `callbacks` has no sample of its own since it isn't a primitive surface to demo in isolation — it's the `on_update`/etc. mechanism every sample above already runs on.

* [app](app/docs/README.md) — window/app-level basics: fullscreen, the FPS counter, window resizing, letterbox vs. stretch-to-fill, target FPS, a startup fade, a simple HUD bar
* [text](text/docs/README.md) — HUD-style text layout: `measure_text`-based right-alignment, `screen_get_width`/`height`, `set_font`'s documented graceful-failure behavior, and a real custom-font swap (Press Start 2P, OFL — see [resources/fonts/](../resources/fonts/README.md))
* [timers](timers/docs/README.md) — two independent background timers, and the "callbacks run on a background thread" gotcha handled correctly
* [input](input/docs/README.md) — keyboard and gamepad reading: a rectangle moved with the keyboard, live gamepad axis/button state
* `tilemap/` — loading a Tiled `.tmx` map — *planned*
* `camera/` — scrolling/zooming the camera over a loaded map — *planned*
* `sprite/` — sprite acquire/configure/animate, built on `tilemap/` — *planned*
* `collision/` — collision rules/handlers, built on `sprite/` — *planned*
* `scripting/` — `ScriptProcessor`'s `sp_*` queued-command sequencing — *planned*
* `sound/` — direct, queued, and one-shot sound/song playback — *planned*
* `json/` — `load_json` — *planned*

Built one category at a time; this list is updated as each one lands.
