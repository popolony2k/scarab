# resources/tilemap/tint/

`tint.tmx` is Scarab's own map for Tiled **layer tint** (`tintcolor`), made for master-plan item V12 (sunlight I8): every case on one 30x20 map of 32 px tiles, each row labelled with a text object (font family `"Sans"`, so it needs `register_font`). It uses Scarab's house tileset one folder up (`../tileset_house.tsx`, `../images/house.png`).

1. A **tile layer**, untinted and tinted cyan (`#80ffff`).
2. An **image layer**, untinted, tinted blue (`#8080ff`), and at opacity 0.5.
3. **Tile objects** at their own size, turned 45 degrees and stretched to 64 px, on an untinted object layer and on one tinted green (`#80ff80`), plus a rectangle and a text object on the green layer, which are never tinted.
4. A tint with an alpha (`#4080ffff`): the alpha is ignored for drawing, so it looks like row 1; next to it, a layer at opacity 0.5.
5. A **group layer** tinted blue at opacity 0.6, holding an untinted tile layer, one tinted yellow (`#ffff80`), and a rectangle and a text object: the group's tint and opacity multiply into its tile layers; shapes and text take only its opacity.

What each case should look like is in [samples/shapes](../../../samples/shapes/docs/README.md) (key `5`); `scripts/layer_tint_smoke` reads every authored tint back exactly.
