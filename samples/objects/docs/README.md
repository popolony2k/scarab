# objects sample

Tiled **tile objects** and **text objects**, drawn from a map. Before sunlight's master plan both were invisible; now a tile placed as an object is drawn (at its own size or stretched into its box, clipped to the viewport, animated), and so is a text object. It uses [resources/tilemap/objects/objects.tmx](../../../resources/tilemap/objects/README.md), taken from sunlight's own `objects` sample. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/objects/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Pan the camera (same direction as [camera](../../camera/docs/README.md)) |
| `Page Up` / `Page Down`, or `+` / `-` (held, main keyboard or keypad) | Zoom in/out |
| `=` (main keyboard or keypad) | Reset the zoom |
| `Home` | Reset the zoom and the camera to its start, (100, 100) |
| `Esc` | Quit |

## What to look for

The green rectangle is the viewport's frame at the starting camera and zoom 1.0: its edges sit exactly on the viewport's own edges, so its top and left lines lie on the clip border. Zoom out and the view covers more of the map, so its right and bottom lines move inside.

- **1. tile object at its own size (32 px)**: the house tile at the size of its tile.
- **2. resized: stretched to its 64 px box**: the object is 64 px, so the tile is stretched to fill it, as Tiled draws it.
- **3. cut by the left viewport edge**: the object starts left of the viewport, so only its right part shows.
- **4. animated: the tile cycles its frames**: tile 3 of the house tileset cycles four frames (small, so zoom in to see it).
- **5. flip bits ignored: drawn unflipped**: the gid has the horizontal-flip bit set; flips aren't drawn, so it looks like case 1.
- **6. cut by the bottom viewport edge**: the object is cut by the bottom edge.
- **The labels** are the map's own text objects, in Caravellius 8x8, and the title in its own yellow. There must be **no `??????`**: that is what a text object draws when no font is registered for its family.

## What it shows

- [main.lua](../main.lua) — `renderer_create{ viewport = { x = 40, y = 40, w = 1000, h = 800 }, zoom = 1.0, ... }`, `tilemap_load_map( ..., MAP_ALIGNMENT_TOP_LEFT )` and `camera_set_position( 100, 100 )`: the layout sunlight's sample uses, which the map's frame object matches.
- **One font, one load**: `load_font` loads Caravellius 8x8 once, `register_font( "Sans", sans )` gives it to the map's `"Sans"` text objects, and the same handle draws the HUD through `draw_text`/`measure_text`'s font argument.
- The HUD sits below the viewport on dark plates and shows the live camera position and zoom.

## Lua API reference

- [`load_font`/`register_font`/`draw_text`/`measure_text`](https://popolony2k.github.io/scarab/lua-api/text.html)
- [`renderer_create`](https://popolony2k.github.io/scarab/lua-api/renderer.html)
- [`tilemap_load_map`](https://popolony2k.github.io/scarab/lua-api/tilemap.html), [`camera_set_position`/`camera_move_*`/`zoom_*`](https://popolony2k.github.io/scarab/lua-api/camera.html)
