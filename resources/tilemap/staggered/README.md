# resources/tilemap/staggered/

`staggered.tmx` is a **staggered** map (`orientation="staggered"`, `staggeraxis="y"`, `staggerindex="odd"`): 8x6 diamond tiles of 64x48, every odd row shifted right by half a tile. It is from [sunlight](https://github.com/popolony2k/sunlight)'s own [`staggered` sample](https://github.com/popolony2k/sunlight/tree/feat/master_plan/samples/staggered) at commit `ad84cca`, shared here because both repos are owned by the same project. Nothing has been changed; re-sync from sunlight rather than editing it on its own.

- Layers: `ground` (id 1), `obstacles` (id 2: four obstacle tiles, each with a rectangular collision shape over its tile image) and `sprites` (id 3, empty, for sprites).
- Tilesets: tileset_staggered.tsx, tileset_obstacle.tsx, with their images in `images/`.

Used in [samples/projections](../../../samples/projections/docs/README.md) and by `scripts/projection_smoke`.
