# resources/tilemap/orthogonal/

`orthogonal.tmx` is an **orthogonal** map (`orientation="orthogonal"`): 8x6 square tiles of 32x32. It is Scarab's own map, made for C5 so that [samples/projections](../../../samples/projections/docs/README.md) shows all four Tiled map styles with the same layout as the [isometric](../isometric/README.md), [staggered](../staggered/README.md) and [hexagonal](../hexagonal/README.md) maps.

- Layers: `ground` (id 1, the grass patch of `../tileset_cloud_soil.tsx`), `obstacles` (id 2: four brick tiles of `../tileset_wall.tsx`, on the same cells as the isometric map's obstacles; the brick tile's collision rectangle covers the whole tile) and `sprites` (id 3, empty, for sprites).
- Tilesets: it uses the shared tilesets one folder up (`../tileset_cloud_soil.tsx`, `../tileset_wall.tsx`) and their images in `../images/`, so it adds no new art.

Used in [samples/projections](../../../samples/projections/docs/README.md) (key `4`) and by `scripts/projection_smoke`.
