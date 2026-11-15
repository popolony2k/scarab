# parallax sample

Tiled's per-layer parallax, read from the map and changed live. Kepi, Scarab's mascot, stays at the centre of the view; walking moves the camera, and the layers scroll at their own speeds. Scarab's counterpart of sunlight's own `parallax` sample, on its map. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/parallax/project.json
```

## Controls

| Input | Action |
| --- | --- |
| Arrow keys / `WASD` | Walk (the camera follows Kepi; the map wraps around) |
| `PageUp` / `PageDown`, `+` / `-` | Zoom in / out |
| `=` / `Home` | Reset the zoom |
| `1` / `2` / `3` / `4` | Pick a layer: sky, birds, clouds, buildings |
| `[` / `]` | The picked layer slower / faster (its factor −/+ 0.25, both axes) |
| `0` | The picked layer fixed on the view (factor 0) |
| `N` | The picked layer moving the other way (factor negated) |
| `R` | Every layer back to the map's own factors |

## What it shows

- **The map's own factors** ([resources/tilemap/parallax/](../../../resources/tilemap/parallax/README.md)): terrain and buildings at 1, the clouds at 0.5, the sky and the birds at 0.25. Walk and the ground passes at full speed, the clouds slower, the sky and birds slowest. The HUD shows each layer's factor (`tilemap_get_layer_parallax`) and the map's parallax origin (`tilemap_get_parallax_origin`).
- **Changing them live** (`tilemap_set_layer_parallax`): the next frame draws with the new factor. `0` pins a layer to the view, a factor above 1 makes it a foreground that passes faster than the ground, and a negative one moves it the other way. `R` puts the map's values back.
- **The wrap:** four marker objects (`tilemap_get_object_by_name`) mark a rectangle; crossing one edge carries Kepi to the opposite one. Zooming out is limited (`view_set_zoom_limits`) so the view never reaches past the map's edge.
- **Kepi is screen space:** Scarab's mascot ([resources/sprites/kepi_idle.png](../../../resources/sprites/README.md)) stays at the view's centre, and no layer's factor moves it.

## Lua API reference

- [Tile map: layer parallax](https://popolony2k.github.io/scarab/lua-api/tilemap.html), [Views: zoom limits](https://popolony2k.github.io/scarab/lua-api/views.html)
