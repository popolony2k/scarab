# resources/tilemap/

Shared Tiled map assets — part of `resources/` at the repo root, the general-purpose shared-resources area shipped with Scarab itself (see the root [README.md](../../README.md)), free for this repo's own samples *and* for any community game built on Scarab to use.

`test.tmx`, its 8 `tileset_*.tsx` tilesets, and their `images/*.png` atlases are the exact same map [sunlight](https://github.com/popolony2k/sunlight)'s own [`tilemaprenderer` sample](https://github.com/popolony2k/sunlight/tree/main/samples/tilemaprenderer) uses — shared here since both repos are owned/authored by the same project. Nothing here has been modified from sunlight's own copy; if sunlight's sample map ever changes, re-sync from there rather than editing this copy independently.

Used in [samples/tilemap](../../samples/tilemap/docs/README.md) (loaded via `tilemap_load_map`), and reused as-is by [samples/camera](../../samples/camera/docs/README.md)/[samples/sprite](../../samples/sprite/docs/README.md)/[samples/collision](../../samples/collision/docs/README.md), the same way sunlight's own samples build on one shared map rather than duplicating it per-sample.

`collision.tmx` is `test.tmx` plus one tile layer, `walls` (id 9): a box of brick tiles with a gap on its right side, drawn with `tileset_wall.tsx` and `images/wall.png` (a 32 px brick tile whose one tile has a collision rectangle over the whole tile, drawn in Tiled's Tile Collision Editor). It is sunlight's own [`collision` sample](https://github.com/popolony2k/sunlight/tree/feat/master_plan/samples/collision) map at commit `4189199`, unmodified, and the only map here with collision tiles. Used in [samples/collision](../../samples/collision/docs/README.md) and by `scripts/tile_collision_smoke` (which has its own small map too).

[tint/](tint/README.md) holds `tint.tmx`, Scarab's own map for Tiled layer tint: tile, image, object and group layers, tint alpha, shapes and text. Used in [samples/shapes](../../samples/shapes/docs/README.md) (key `5`) and by `scripts/layer_tint_smoke`.

[animation/](animation/README.md) holds `animation.tmx`, Scarab's own map for Tiled tile animation by time: one animated tile in several cells, flipped, off screen, on a hidden layer and as tile objects, next to a static tile and an all-0 ms animation. Used in [samples/shapes](../../samples/shapes/docs/README.md) (key `6`) and by `scripts/tile_animation_smoke`.

[shapes/](shapes/README.md) holds `shapes.tmx`, `rotation.tmx` and `thick.tmx`, maps made for shape, rotated and point objects (from sunlight's own `shapes` sample). Used in [samples/shapes](../../samples/shapes/docs/README.md).

[isometric/](isometric/README.md), [staggered/](staggered/README.md) and [hexagonal/](hexagonal/README.md) hold one non-orthogonal map each (from sunlight's own samples): an isometric, a staggered and a hexagonal grid, each with ground, obstacle and sprite layers. Used in [samples/projections](../../samples/projections/docs/README.md) and by `scripts/projection_smoke`.

[orthogonal/](orthogonal/README.md) is Scarab's own orthogonal map with the same layout, drawn with the shared `tileset_cloud_soil.tsx` and `tileset_wall.tsx` here. Used in [samples/projections](../../samples/projections/docs/README.md) (key `4`) and by `scripts/projection_smoke`.

[worlds/](worlds/README.md) holds `grid.world`, a Tiled world file placing four of the maps above side by side (orientations mixed). Used in [samples/worlds](../../samples/worlds/docs/README.md) and by `scripts/world_api_smoke`.

[parallax/](parallax/README.md) holds `parallax.tmx`, a map made for Tiled's per-layer parallax (from sunlight's own `parallax` sample): layers at factors 1, 0.5 and 0.25 and a parallax origin. Used in [samples/parallax](../../samples/parallax/docs/README.md) and by `scripts/layer_parallax_smoke`.

[objects/](objects/README.md) holds `objects.tmx` and `opacity.tmx`, a map made for Tiled tile and text objects (from sunlight's own `objects` sample), which reuses the house tileset here. Used in [samples/objects](../../samples/objects/docs/README.md).
