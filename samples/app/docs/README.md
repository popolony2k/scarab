# app sample

Every window/app-level toggle `LuaAppApi` exposes, in one place — fullscreen, the FPS counter, window resizing, letterbox vs. stretch-to-fill, target FPS, a startup fade, and a simple HUD bar — driven live by the keys listed on screen. Builds on [hello-world](../../hello-world/docs/README.md); see [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/app/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `F1` | Toggle the on-screen FPS counter (`app_set_draw_fps`) |
| `F2` | Toggle whether the window can be resized (`app_set_window_resizeable`) |
| `F3` | Toggle fullscreen (`app_set_fullscreen`) — macOS uses `FULLSCREEN_STRATEGY_REAL`, everything else `FULLSCREEN_STRATEGY_BORDERLESS_WINDOWED` (see [What it shows](#what-it-shows)) |
| `F4` | Toggle stretch-to-fill vs. letterboxing (`app_set_stretch_to_fill`) |
| `Up` / `Down` (held) | Raise/lower the target FPS (`app_set_target_fps`/`app_get_target_fps`) |

## What it shows

- [main.lua](../main.lua) — every `LuaAppApi` primitive except `app_set_name` (already covered in `hello-world`):
  - `app_get_platform()` picks a fullscreen strategy per-platform, mirroring the primitive's own doc example — `FULLSCREEN_STRATEGY_REAL` is kept macOS-only project-wide (a real, measured Linux performance regression; Windows itself isn't affected but stays consistent with Linux by deliberate choice), so this sample makes the same choice a real game should.
  - `screen_fade` fades in from black over the first second, the same ramp-over-several-frames pattern the primitive's own doc example uses — not a jump straight to fully visible.
  - `draw_filled_rectangle` draws a simple two-rectangle "meter" bar (a dark background plate, then a colored fill) — the fill tracks the startup fade purely so there's something visibly animated on screen even after the fade itself ends.
  - Every toggle's *current* state is read back every frame (`app_get_draw_fps()`, etc.) and drawn as on-screen text, so the HUD always reflects reality rather than an assumed local variable.

## Lua API reference

Full docs for everything demonstrated here: [popolony2k.github.io/scarab/lua-api/app.html](https://popolony2k.github.io/scarab/lua-api/app.html) (and [text.html](https://popolony2k.github.io/scarab/lua-api/text.html)/[input.html](https://popolony2k.github.io/scarab/lua-api/input.html) for `draw_text`/the key-reading primitives).
