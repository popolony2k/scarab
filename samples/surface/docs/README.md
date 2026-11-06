# surface sample

Shapes in map units: every `shape_draw_*` function, drawn each frame over a map you can scroll and zoom, with a hitbox outline that stays on a walking sprite. Scarab's counterpart of sunlight's own `surface` sample. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/surface/project.json
```

## Controls

| Input | Action |
| --- | --- |
| Arrow keys / `WASD` | Pan the camera |
| `PageUp` / `PageDown`, `+` / `-` | Zoom in / out |
| `=` | Reset the zoom |
| `Home` | Reset the camera and the zoom |
| `M` | Unload the map / load it again |

## What it shows

- **Every shape:** a thick cyan square turning about its centre (`shape_draw_polygon` with a rotation), an orange ellipse ring turning more slowly (`shape_draw_ellipse`), a green rectangle (`shape_draw_rectangle`), a magenta sine wave travelling along a thick polyline (`shape_draw_polyline`), and a row of yellow points, each larger than the last (`shape_draw_point`).
- **Map units:** pan and zoom, and the shapes move and scale with the map's own tiles; the viewport clips them.
- **A hitbox that stays on its sprite:** Sunny walks back and forth with a red outline around it. The sprite is in world space (`sprite_set_world_space`), so its position is a map position, the same space as the shapes.
- **No map:** press `M`. Shapes follow the camera with or without a map, and unloading the map leaves the camera where it was, so the shapes stay where they were and only the map's tiles go. (Panning needs a map: with none, the camera keys do nothing.)
- **Screen space for contrast:** the HUD lines and the progress bar at the bottom (`draw_text`, `draw_filled_rectangle`) never move.

## Lua API reference

- [Shapes](https://popolony2k.github.io/scarab/lua-api/shape.html), [Sprites: world space](https://popolony2k.github.io/scarab/lua-api/sprite.html)
