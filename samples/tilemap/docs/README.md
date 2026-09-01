# tilemap sample

The minimal map-on-screen sample: loads a Tiled `.tmx` map and reports its own dimensions — no sprites, no camera movement. See [samples/README.md](../../README.md) for the full sample list; [camera](../../camera/docs/README.md) builds on this exact same map next.

## Running

From the repo root:

```shell
./build/scarab samples/tilemap/project.json
```

## What it shows

- [main.lua](../main.lua) — `tilemap_load_map("resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER)` loads the map, centered in the viewport since it's smaller than the screen; `tilemap_get_map_info()` reads back its dimensions (in tiles) and tile size (in pixels), drawn on screen to confirm the load actually succeeded rather than just trusting the return value silently.
- The map itself ([resources/tilemap/](../../../resources/tilemap/README.md)) is shared, unmodified, from sunlight's own `tilemaprenderer` sample — both repos are owned by the same project, so nothing needed re-authoring.

## Lua API reference

- [`tilemap_load_map`/`tilemap_get_map_info`](https://popolony2k.github.io/scarab/lua-api/tilemap.html)
