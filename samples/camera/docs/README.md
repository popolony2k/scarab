# camera sample

Scrolling and zooming the camera over the same map [tilemap](../../tilemap/docs/README.md) loads — panning/zoom via incremental keyboard-driven steps, an absolute jump via `camera_set_position`, and every piece of camera/viewport state read back and shown live. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/camera/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Pan the camera (`camera_move_up`/`down`/`left`/`right`) |
| `Page Up` / `Page Down` (held) | Zoom in/out (`zoom_in`/`zoom_out`) |
| `+` / `-` (held, main keyboard or keypad) | Zoom in/out, the same as `Page Up` / `Page Down` (the zoom keys every sunlight/Scarab sample shares) |
| `=` (main keyboard or keypad) | Reset the zoom only (`zoom_reset`) |
| `Home` | Reset camera position and zoom (`camera_reset`/`zoom_reset`) |
| `Space` | Jump the camera to a fixed point via `camera_set_position` (an absolute move, not a pan step) |

## What it shows

- [main.lua](../main.lua) — `camera_move_*`/`zoom_in`/`zoom_out` are each one step per call (the scroll step — `renderer_create`'s `scroll_step`, 1 pixel by default; panning stops exactly on the map's edges) — calling one every frame the matching key is held gives continuous panning/zooming, the same per-frame-call pattern Caravellius's own `camera.lua` uses for its auto-scroll.
- The zoom has two settings: the *current* zoom and the *preferred* zoom, the one `zoom_reset` returns to. They are separate, and the preferred zoom defaults to `1.0`, not to the zoom the game starts at, so this sample calls `view_set_preferred_zoom(0, viewport_get_zoom_factor())` right after loading the map to make `Home`/`=` reset to its own starting zoom (Scarab's default, `3.8125`).
- `camera_set_position` is an **absolute** jump, unlike the incremental `camera_move_*` nudges, and unlike `camera_reset()` (which only returns to the map's original load-time position) — it does **not** clamp to map boundaries, so this sample computes a coordinate safely inside the map from `tilemap_get_map_info()` rather than hardcoding one.
- `camera_get_position`/`viewport_get_dimension`/`viewport_get_zoom_factor` are read and drawn every frame, so the HUD always reflects the camera's real current state.

## Lua API reference

- [`camera_move_up`/`camera_set_position`/`camera_get_position`/`zoom_in`/`viewport_get_dimension`](https://popolony2k.github.io/scarab/lua-api/camera.html)
