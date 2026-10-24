# resources/tilemap/shapes/

Three maps made to show Tiled **shape objects**, **rotated objects** and **point objects**, from [sunlight](https://github.com/popolony2k/sunlight)'s own [`shapes` sample](https://github.com/popolony2k/sunlight/tree/feat/master_plan/samples/shapes) at commit `9c8c6bf`, shared here because both repos are owned by the same project. Re-sync from sunlight rather than editing them on their own.

- `shapes.tmx`: the four shapes (rectangle, ellipse, polyline, polygon) and how the viewport clips them, with a frame object at the viewport's edges.
- `rotation.tmx`: rectangles, polylines, polygons, an ellipse, tile objects and text objects with a Tiled rotation, each turned clockwise about its stored point (the small square marked "pivot"). Its embedded tileset uses Sunny's frames.
- `thick.tmx`: outlines with a `line_width` property (whole and fractional widths) and point objects with a `point_size`; both scale with the zoom.

Only one thing changed from sunlight's copies: `rotation.tmx`'s embedded tileset image is `../../sprites/sunny_idle_down.png`, Scarab's byte-identical copy in [resources/sprites/](../../sprites/README.md), where sunlight's sample keeps its own copy next to the map. That `..` path needs sunlight `f651acc` (I4) or later: before, a `..` in an embedded tileset's image path made `tilemap_load_map` refuse the map. sunlight's fourth map, `stress.tmx` (a benchmark), is not copied. Every text object uses font family `"Sans"`, which needs `register_font`.

Used in [samples/shapes](../../../samples/shapes/docs/README.md).
