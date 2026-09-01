# resources/tilemap/

Shared Tiled map assets — part of `resources/` at the repo root, the general-purpose shared-resources area shipped with Scarab itself (see the root [README.md](../../README.md)), free for this repo's own samples *and* for any community game built on Scarab to use.

`test.tmx`, its 8 `tileset_*.tsx` tilesets, and their `images/*.png` atlases are the exact same map [sunlight](https://github.com/popolony2k/sunlight)'s own [`tilemaprenderer` sample](https://github.com/popolony2k/sunlight/tree/main/samples/tilemaprenderer) uses — shared here since both repos are owned/authored by the same project. Nothing here has been modified from sunlight's own copy; if sunlight's sample map ever changes, re-sync from there rather than editing this copy independently.

Used in [samples/tilemap](../../samples/tilemap/docs/README.md) (loaded via `tilemap_load_map`), and reused as-is by [samples/camera](../../samples/camera/docs/README.md)/[samples/sprite](../../samples/sprite/docs/README.md)/[samples/collision](../../samples/collision/docs/README.md), the same way sunlight's own samples build on one shared map rather than duplicating it per-sample.
