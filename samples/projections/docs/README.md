# projections sample

The four Tiled map styles - isometric, staggered, hexagonal and orthogonal - with the functions Scarab already has. Kepi, Scarab's mascot, walks from cell to cell around the obstacles, and a free cursor picks the cell under it. The first three maps are sunlight's own ([resources/tilemap/isometric/](../../../resources/tilemap/isometric/README.md), [staggered/](../../../resources/tilemap/staggered/README.md), [hexagonal/](../../../resources/tilemap/hexagonal/README.md)); the orthogonal one is Scarab's, with the same layout ([orthogonal/](../../../resources/tilemap/orthogonal/README.md)). See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/projections/project.json
```

## Controls

| Keyboard | Controller | Action |
| --- | --- | --- |
| `1` / `2` / `3` / `4` | `L1` / `R1` (previous / next) | Load the isometric, staggered, hexagonal or orthogonal map (the HUD lists them, the loaded one in brackets) |
| Arrow keys | D-pad | Move Kepi one row (up/down) or one column (left/right); obstacles and the map's edge block the way |
| `WASD` | Left stick | Move the cursor, in map pixels |
| `PageUp` / `PageDown`, `+` / `-` | `R2` / `L2` | Zoom in / out |
| `=` / `Home` | `Y` | Reset the zoom |

The controller is the first one connected. The gamepad functions only say whether a button is down, so the sample remembers each button's state from the frame before, to move one cell and switch one map per press.

## What it shows

- **A cell's place** (`tilemap_get_tile`): the tile image's rectangle in map pixels, where the projection puts the cell. The cell itself - a diamond, a pointy-top hexagon on the hexagonal map, or a square on the orthogonal one - is the bottom `tileHeight` pixels of it; the sample outlines Kepi's cell in white with `shape_draw_polygon`, and the HUD shows the rectangle.
- **Picking** (`tilemap_to_tile_matrix`): the cell under the cursor is outlined in yellow, by the cell's real outline. In the isometric map's empty corners, inside its pixel rectangle but outside every diamond, it says "no cell".
- **Moving by row and column:** on an isometric map a row step goes diagonally on screen; on the staggered and hexagonal maps every other row is shifted by half a cell. The obstacles are read from the map's `obstacles` layer (`tilemap_get_tile`, a `gid` above 0).
- **Kepi is world space** (`sprite_set_world_space`): its position is a map position, on the map's `sprites` layer, and the camera centres on it at any zoom.

Tile collisions on these maps use each obstacle shape's bounding box (see the [tile map reference](https://popolony2k.github.io/scarab/lua-api/tilemap.html)), which is why the sample blocks by cell instead.

## Lua API reference

- [Tile map: tiles and map projections](https://popolony2k.github.io/scarab/lua-api/tilemap.html), [Shapes](https://popolony2k.github.io/scarab/lua-api/shape.html), [Sprites: world space](https://popolony2k.github.io/scarab/lua-api/sprite.html)
