# Tile map

*Implemented in* `src/engine/luatilemapapi.cpp`, wrapping sunlight's `ITileMap` (Tiled `.tmx` maps via libtmx).

## `tilemap_load_map(path, alignment) -> success`

Load a `.tmx` map file, replacing any currently loaded map. `alignment` (optional, defaults to `MAP_ALIGNMENT_CENTER`) controls how the map is positioned relative to the viewport when it's smaller than the screen.

```lua
tilemap_load_map(BASE_PATH .. "tilemap/corsair/corsair.tmx", MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM)
```

Alignment constants: `MAP_ALIGNMENT_CENTER`, `MAP_ALIGNMENT_TOP_LEFT`, `MAP_ALIGNMENT_TOP_RIGHT`, `MAP_ALIGNMENT_BOTTOM_LEFT`, `MAP_ALIGNMENT_BOTTOM_RIGHT`, `MAP_ALIGNMENT_CENTER_WIDTH_TOP`, `MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM`, `MAP_ALIGNMENT_CENTER_HEIGHT_LEFT`, `MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT`.

## `tilemap_unload_map() -> success`

Unload the current map.

## `tilemap_get_map_info() -> mapWidth, mapHeight, tileWidth, tileHeight`

Dimensions of the currently loaded map, in tiles (`mapWidth`/`mapHeight`) and pixels-per-tile (`tileWidth`/`tileHeight`). Returns `nil` if no map is loaded.

```lua
local mapW, mapH, tileW, tileH = tilemap_get_map_info()
```

## Layers

Every sprite lives on a numbered (or named) Tiled layer — layer ids are whatever you assigned them to in the Tiled editor, not something the engine invents (Caravellius keeps its own names for them in `resources/scripts/layerids.lua`).

### `tilemap_get_layer(layerId) -> visible, opacity, offsetX, offsetY`
### `tilemap_get_layer_by_name(layerName) -> visible, opacity, offsetX, offsetY`

```lua
local visible, opacity, offX, offY = tilemap_get_layer(2)
```

### `tilemap_set_layer(layerId, visible, opacity, offsetX, offsetY) -> success`
### `tilemap_set_layer_by_name(layerName, visible, opacity, offsetX, offsetY) -> success`

```lua
-- fade a layer out
tilemap_set_layer(2, true, 128, 0, 0)
```

## `tilemap_get_tile(row, col, layerId) -> gid, x, y, width, height`

Look up a single tile by its row/column position on a given layer. `gid` is the tile's Tiled global id (`0` conventionally means "no tile here" in Tiled, same as the raw `.tmx` format). Returns `nil` if the layer or tile position doesn't exist.

```lua
local gid, x, y, w, h = tilemap_get_tile(0, 0, 1)
```

## `tilemap_to_tile_matrix(worldX, worldY) -> row, col`

Convert a world/screen pixel coordinate to a tile row/column — the inverse of the position math you'd otherwise do by hand against `tilemap_get_map_info`'s tile size. Returns `nil` if the coordinate is outside the map.

```lua
local row, col = tilemap_to_tile_matrix(100, 250)
```
