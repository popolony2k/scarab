# resources/tilemap/objects/

`objects.tmx` is a map made to show Tiled **tile objects** and **text objects**, which sunlight draws since its master plan (before, both were invisible). It comes from [sunlight](https://github.com/popolony2k/sunlight)'s own [`objects` sample](https://github.com/popolony2k/sunlight/tree/feat/master_plan/samples/objects) at commit `9c8c6bf`, shared here because both repos are owned by the same project.

Only one thing changed from sunlight's copy: the tileset path, `../tileset_house.tsx`. sunlight's sample ships its own copy of the house tileset next to the map, and Scarab's byte-identical copy (`tileset_house.tsx` and `images/house.png`) already lives one folder up in [resources/tilemap/](../README.md). A comment at the top of the map says so. Re-sync from sunlight rather than editing this map on its own.

What is in it (60x40 tiles of 32 px, object layers only, no tile layers):
- a `frame` rectangle object at (100, 100, 1000, 800): the viewport's edges at the starting camera of the sample that uses this map;
- six **tile objects**, one per row: at their own size, stretched to a 64 px box, cut by the left viewport edge, animated (tile 3 of the house tileset cycles four frames), with a flip bit (drawn unflipped), and cut by the bottom edge;
- a **text object** label beside each one, in font family `"Sans"`, plus a yellow title. Text objects draw only with a font registered for their family (`register_font`), else `??????`.

Used in [samples/objects](../../../samples/objects/docs/README.md).
