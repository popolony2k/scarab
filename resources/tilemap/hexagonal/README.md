# resources/tilemap/hexagonal/

`hexagonal.tmx` is a **hexagonal** map (`orientation="hexagonal"`, `hexsidelength="20"`, `staggeraxis="y"`, `staggerindex="odd"`): 8x6 hexes of 60x50, rows 35 px apart. It is from [sunlight](https://github.com/popolony2k/sunlight)'s own [`hexagonal` sample](https://github.com/popolony2k/sunlight/tree/feat/master_plan/samples/hexagonal) at commit `ad84cca`, shared here because both repos are owned by the same project. Nothing has been changed; re-sync from sunlight rather than editing it on its own.

- Layers: `ground` (id 1), `obstacles` (id 2: four obstacle tiles, each with a rectangular collision shape over its tile image) and `sprites` (id 3, empty, for sprites).
- Tilesets: tileset_hexagonal.tsx, tileset_hex_obstacle.tsx, with their images in `images/`.

Used in [samples/projections](../../../samples/projections/docs/README.md) and by `scripts/projection_smoke`.
