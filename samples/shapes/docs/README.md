# shapes sample

Tiled **shape objects** (rectangle, ellipse, polyline, polygon), **rotated objects** and **point objects**, on three maps from sunlight's own `shapes` sample ([resources/tilemap/shapes/](../../../resources/tilemap/shapes/README.md)), plus **per-object opacity** on sunlight's `opacity` map ([resources/tilemap/objects/](../../../resources/tilemap/objects/README.md)). Keys `1` to `4` switch maps. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/shapes/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `1` / `2` / `3` / `4` | Show `shapes.tmx` / `rotation.tmx` / `thick.tmx` / `opacity.tmx` |
| `Arrow keys` / `WASD` (held) | Pan the camera (same direction as [camera](../../camera/docs/README.md)) |
| `Page Up` / `Page Down`, or `+` / `-` (held, main keyboard or keypad) | Zoom in/out |
| `=` (main keyboard or keypad) | Reset the zoom |
| `Home` | Reset the zoom and the camera (to the map's starting position) |
| `Esc` | Quit |

## What to look for

- **1, `shapes.tmx`**: the four shapes, each labelled. Pan or zoom in: a shape crossing the viewport's edge is cut cleanly at the edge, never drawn outside it.
- **2, `rotation.tmx`**: every object is turned **clockwise about its own stored point**, the small square marked "pivot": a rectangle, polyline and polygon at 90 and 45 degrees, an ellipse at 45, Sunny's tile objects at 90 and 45, and the text objects "turned text" (90, reads vertically) and "text 45". Before sunlight's master plan the rotation was ignored and everything was drawn upright.
- **3, `thick.tmx`**: outlines with a `line_width` (1, 2, 3, 5, a fractional 2.5, a rectangle, a polygon, a sharp corner, an ellipse) and point objects with a `point_size`, drawn as filled squares. Zoom in and out: the widths and the points grow and shrink with the zoom.
- **4, `opacity.tmx`** (the camera starts at (100, 100), so the green frame fills the viewport): four labelled rows. Tile objects, rectangle outlines and text objects each fade 1.0, 0.75, 0.5, 0.25, 0.0 left to right (the last one invisible), each object's own `opacity` property overriding its layer's. In row 4, two tiles on a layer at 0.4, one with no property and one with an invalid `5`, look the same. Before sunlight's master plan, the rectangles and the text in rows 2 and 3 were all drawn fully opaque.
- Every label is a text object in Caravellius 8x8; there must be no `??????` on any map.

## What it shows

- [main.lua](../main.lua) — the layout of sunlight's own sample: `renderer_create{ viewport = { x = 40, y = 40, w = 1000, h = 800 }, zoom = 1.0, ... }`, each map loaded `MAP_ALIGNMENT_TOP_LEFT` with the camera at the origin (`opacity.tmx`: at (100, 100), as in sunlight's `opacity` sample).
- **Switching maps** calls `tilemap_load_map` over the map already loaded, which sunlight no longer leaks.
- **One font, one load**: `load_font` loads Caravellius 8x8 once and `register_font( "Sans", sans )` gives it to every map's text objects. The registration belongs to the renderer, not to a map, so it survives every switch. The same handle draws the HUD.

## Lua API reference

- [`load_font`/`register_font`/`draw_text`/`measure_text`](https://popolony2k.github.io/scarab/lua-api/text.html)
- [`renderer_create`](https://popolony2k.github.io/scarab/lua-api/renderer.html)
- [`tilemap_load_map`](https://popolony2k.github.io/scarab/lua-api/tilemap.html), [`camera_set_position`/`camera_move_*`/`zoom_*`](https://popolony2k.github.io/scarab/lua-api/camera.html)
