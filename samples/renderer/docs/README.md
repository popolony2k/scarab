# renderer sample

Choosing your own window/render settings with `renderer_create` instead of Scarab's defaults, reading them back with `renderer_get_config`, and driving the renderer's default **view** — zoom, zoom limits, camera — with the `view_*` functions, over the same map [tilemap](../../tilemap/docs/README.md) loads. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/renderer/project.json
```

Also works headless (no window, no display needed) — it just draws nothing:

```shell
./build/scarab --headless --fast --max-frames 300 --max-frames-ok samples/renderer/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Z` / `X` (held) | Zoom in/out (`view_zoom_in`/`view_zoom_out`) — one `ZOOM_STEP` (`0.0625`) per frame, within the limits `0.5 … 4.0` |
| `Arrow keys` (held) | Pan the view's camera (`view_move_camera_*`) |
| `R` | Reset the zoom to the preferred zoom and the camera to the origin (`view_zoom_reset`/`view_reset_camera`) |
| `Space` | Ask for an invalid zoom (`3.8`) and show the exact error `view_set_zoom` returns |

## What it shows

- [main.lua](../main.lua) — `renderer_create` is the **first** window-needing call, on purpose: the entry script runs *before* any window exists, and `renderer_create` opens the window the moment it succeeds. Any window-needing call before it (`app_set_name`, `set_font`, `tilemap_load_map`, …) creates the default renderer first, and `renderer_create` then fails with a message naming that call. Renderer-free calls (`load_json`, `sound_*`, `sp_*`, …) are fine before it.
- Every option is strictly checked, and every failure is `nil, "message"` — never a Lua error. Zoom is a **factor**, which must be a whole multiple of `ZOOM_STEP` (`0.0625`): `3.8125` is valid, `3.8` is rejected (with the nearest valid factor named), never silently rounded. The viewport must fit the render area.
- The on-screen text is **read back** from the renderer every frame (`renderer_get_config`, `view_get_zoom`, `view_get_zoom_limits`, `view_get_camera_position`, `view_get_dimension`, `view_get_scroll_step`), so it always reflects the real current state, not what was requested at creation.
- The older global `camera_*`/`zoom_*`/`viewport_*` functions keep working and act on this same default view (view `0`); the `view_*` functions take the view first so the same calls can address other views once they exist.

## Lua API reference

- [`renderer_create`/`renderer_get_config`/`renderer_get_default_view`/`renderer_get_backend`](https://popolony2k.github.io/scarab/lua-api/renderer.html)
- [`view_set_zoom`/`view_set_zoom_limits`/`view_zoom_in`/`view_move_camera_up`/`view_get_dimension`/…](https://popolony2k.github.io/scarab/lua-api/views.html)
