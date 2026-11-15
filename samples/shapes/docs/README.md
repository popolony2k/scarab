# shapes sample

Tiled **shape objects** (rectangle, ellipse, polyline, polygon), **rotated objects** and **point objects**, on three maps from sunlight's own `shapes` sample ([resources/tilemap/shapes/](../../../resources/tilemap/shapes/README.md)), plus **per-object opacity** on sunlight's `opacity` map ([resources/tilemap/objects/](../../../resources/tilemap/objects/README.md)) **layer tint** on Scarab's `tint` map ([resources/tilemap/tint/](../../../resources/tilemap/tint/README.md)), and **tile animation** on Scarab's `animation` map ([resources/tilemap/animation/](../../../resources/tilemap/animation/README.md)). Keys `1` to `6` switch maps. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/shapes/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `1` / `2` / `3` / `4` / `5` / `6` | Show `shapes.tmx` / `rotation.tmx` / `thick.tmx` / `opacity.tmx` / `tint.tmx` / `animation.tmx` |
| `T` (map 5) | Cycle layer 2's tint at runtime (`tilemap_set_layer_tint`): yellow, green, white, cyan |
| `G` (map 5) | Step the group's opacity at runtime (`tilemap_set_layer`): 1, 0.25, 0.6 |
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
- **5, `tint.tmx`**: Tiled layer tint, row by row: a tile layer tinted cyan next to an untinted one; an image layer untinted, tinted blue and at opacity 0.5; tile objects at their own size, turned and stretched, untinted and tinted green (the stretched one included), with a white rectangle and text on the green layer that stay untinted; a tint with an alpha that looks just like row 1 (the alpha is ignored) next to opacity 0.5; a group tinted blue at opacity 0.6 whose tile layers turn bluish and faded, and whose rectangle and text only fade. `T` and `G` change a tint and the group's opacity live; the HUD shows both. Loading the map again (another key, then `5`) restores the authored values.
- **6, `animation.tmx`**: one tile animating over three monkeys for 100, 50 and 300 ms, in two cells, flipped, far to the right (pan right to reach it) and as two tile objects. Every copy changes face at the same moment, and the far-right one is in step as soon as it scrolls into view: a Tiled animation runs by time from `tilemap_load_map`, as Tiled shows it, and never pauses while off screen. A static monkey and an all-0 ms animation (its first face) never change. The HUD reads the frame back with `tilemap_get_tile_frame` / `tilemap_get_object_tile_frame` (gids 1 / 6 / 11; the static tile is 16). Before sunlight's master plan, an animation advanced only while drawn, showed its first frame twice each cycle, and held a frame that wasn't a whole number of frames too long.
- Every label is a text object in Caravellius 8x8; there must be no `??????` on any map.

## What it shows

- [main.lua](../main.lua) — the layout of sunlight's own sample: `renderer_create{ viewport = { x = 40, y = 40, w = 1000, h = 800 }, zoom = 1.0, ... }`, each map loaded `MAP_ALIGNMENT_TOP_LEFT` with the camera at the origin (`opacity.tmx`: at (100, 100), as in sunlight's `opacity` sample).
- **Switching maps** calls `tilemap_load_map` over the map already loaded, which sunlight no longer leaks.
- **One font, one load**: `load_font` loads Caravellius 8x8 once and `register_font( "Sans", sans )` gives it to every map's text objects. The registration belongs to the renderer, not to a map, so it survives every switch. The same handle draws the HUD.

## Lua API reference

- [`load_font`/`register_font`/`draw_text`/`measure_text`](https://popolony2k.github.io/scarab/lua-api/text.html)
- [`renderer_create`](https://popolony2k.github.io/scarab/lua-api/renderer.html)
- [`tilemap_load_map`](https://popolony2k.github.io/scarab/lua-api/tilemap.html), [`camera_set_position`/`camera_move_*`/`zoom_*`](https://popolony2k.github.io/scarab/lua-api/camera.html)
