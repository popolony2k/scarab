# worlds sample

Several maps on screen at once, each in a **world** of its own: split screen, a cross-fade between two different maps, a world of four maps from a Tiled `.world` file, and a world unloaded and loaded again. Scarab's counterpart of sunlight's own `worlds` sample, on the shared maps. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/worlds/project.json
```

## Controls

| Keyboard | Controller | Action |
| --- | --- | --- |
| `WASD` | Left stick | Walk the left Kepi (the cloud map) |
| Arrow keys | D-pad | Walk the right Kepi (the world the right half shows) |
| `1` / `2` | `Y` (switches) | Show the isometric world or the grid world on the right |
| `F` | `A` | Cross-fade the left half to the isometric world, and back |
| `U` | `B` | Unload the isometric world, or load it again |
| `T` | `X` | Tint the grid world's ground layer, or take the tint off |

The controller is the first one connected.

## What it shows

- **Split screen** (`view_create`, `view_set_world`): the left half is the default view, which always shows the **default world** - the cloud map loaded with `tilemap_load_map`. The right half is an extra view showing another world, made with `world_load` from a map (`resources/tilemap/isometric/`) or from a Tiled `.world` file ([resources/tilemap/worlds/grid.world](../../../resources/tilemap/worlds/README.md)).
- **Isolated worlds:** each world has its own Kepi (`sprite_add_to_layer( handle, layer, world )`, with the layer id from `world_layer_id`). The two Kepis never meet and never collide.
- **Several maps in one world:** the grid world holds four maps at offsets, orientations mixed. Its Kepi walks across the maps' edges; a tile collision rule of that world only (`collision_add_tile_rule( ..., grid )`) stops him at the obstacles of every map. The tile handler receives the world as its last argument, and the HUD counts the hits.
- **A layer of a whole world** (`tilemap_set_layer_tint_by_name( "ground", r, g, b, a, grid )`): one call tints the "ground" layer of all four maps of the grid world - a layer setter given a world reaches every map that has the layer, so the maps stay alike across their edges.
- **A cross-fade between two maps** (`view_set_opacity`): an overlay view over the left half shows the isometric world, with an opaque backdrop (`view_set_background_color`), and `F` fades it from 0 to 1 in one second.
- **Unloading a world** (`world_unload`): the views that showed it go back to the default world, and its Kepi is detached - not drawn, not colliding, still yours. `U` again loads it (`world_load`) and puts Kepi back.
- **Whole worlds:** every view shows its whole world (`view_fit_to_map`, again whenever it changes world); `world_get_bounds` keeps each Kepi inside its world.

## Lua API reference

- [Worlds](https://popolony2k.github.io/scarab/lua-api/worlds.html), [Views: the world a view shows, view opacity](https://popolony2k.github.io/scarab/lua-api/views.html), [Collision: rules per world](https://popolony2k.github.io/scarab/lua-api/collision.html)
