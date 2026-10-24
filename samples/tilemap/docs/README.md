# tilemap sample

The minimal map-on-screen sample: loads a Tiled `.tmx` map and reports its own dimensions — no sprites, no camera movement. See [samples/README.md](../../README.md) for the full sample list; [camera](../../camera/docs/README.md) builds on this exact same map next.

## Running

From the repo root:

```shell
./build/scarab samples/tilemap/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `↑` / `↓` | Raise/lower the *clouds* layer's opacity by `15` per press, from `0` (transparent) to `255` (opaque) (`tilemap_set_layer`) |

## What it shows

- [main.lua](../main.lua) — `tilemap_load_map("resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER)` loads the map, centered in the viewport since it's smaller than the screen; `tilemap_get_map_info()` reads back its dimensions (in tiles) and tile size (in pixels), drawn on screen to confirm the load actually succeeded rather than just trusting the return value silently.
- **A layer's opacity is an integer 0..255**, `255` = fully opaque. `↑`/`↓` write the next value with `tilemap_set_layer` (steps of 15, which divides 255, so both ends are reached exactly) and the screen shows what `tilemap_get_layer` reads back. `tilemap_set_layer` writes all four of a layer's values, so the sample reads the layer first and passes its visibility and offset back unchanged. The *clouds* layer also holds the grass islands, so they fade with it.
- The map itself ([resources/tilemap/](../../../resources/tilemap/README.md)) is shared, unmodified, from sunlight's own `tilemaprenderer` sample — both repos are owned by the same project, so nothing needed re-authoring.

## Lua API reference

- [`tilemap_load_map`/`tilemap_get_map_info`/`tilemap_get_layer`/`tilemap_set_layer`](https://popolony2k.github.io/scarab/lua-api/tilemap.html)
