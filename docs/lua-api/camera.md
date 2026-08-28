# Camera

*Implemented in* `src/lua/luacameraapi.cpp`.

All camera functions act on the currently loaded map's viewport (`tilemap_load_map` — see [tilemap.md](tilemap.md)) — there's no separate "camera object" to create or select.

## Panning

Each call moves the camera one fixed step in that direction (the step size is configured on the C++ renderer, not from Lua — see `TileMapRenderer::SetScrollStepSize` in `main.cpp`).

```lua
camera_move_up()
camera_move_down()
camera_move_left()
camera_move_right()
```

Caravellius drives a constant downward auto-scroll from `src/camera.lua`'s `Camera.update`, called every frame via `Enemies.register_update` — that's ordinary game script using `camera_move_down()`, not a separate engine mechanism.

```lua
-- src/camera.lua (simplified)
local MAX_FPS_PER_SCROLL = 2
local fpsCount = 1

function Camera.update(dt)
  if fpsCount == MAX_FPS_PER_SCROLL then
    camera_move_down()
    fpsCount = 1
  else
    fpsCount = fpsCount + 1
  end
end
```

## `camera_reset()`

Reset the camera to the map's default position.

## `camera_set_position(x, y)`

Jump the camera so the given world-space coordinate (pixels) is shown at the top-left of the viewport — an absolute move, unlike `camera_move_*`'s incremental one-step nudges, and unlike `camera_reset()` (which only returns to the map's original load-time position). Does **not** clamp to map boundaries — the caller is responsible for passing a valid target, e.g. a position read via `tilemap_get_object_by_name` (see [tilemap.md](tilemap.md)).

```lua
local x, y, w, h = tilemap_get_object_by_name("SubBossIntervalStart")
camera_set_position(x, y)
```

## `camera_get_position() -> x, y`

Read the world-space coordinate currently shown at the top-left of the viewport — the exact inverse of `camera_set_position`, and the only reliable way to know the camera's current scroll position (there's no way to derive it purely from map/viewport size — the load-time alignment math isn't part of the public contract).

```lua
local x, y = camera_get_position()
```

## Zoom

```lua
zoom_in()
zoom_out()
zoom_reset()
```

## `viewport_get_dimension() -> x, y, width, height`

Read the viewport's current position and size (screen-space pixels).

```lua
local x, y, w, h = viewport_get_dimension()
print(("viewport: %d,%d %dx%d"):format(x, y, w, h))
```

## `viewport_get_zoom_factor() -> factor`

Read the current zoom multiplier (`1.0` = no zoom).

```lua
local zoom = viewport_get_zoom_factor()
```
