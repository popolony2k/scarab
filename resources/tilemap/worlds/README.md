# resources/tilemap/worlds/

`grid.world` is a Tiled **world** file: four maps placed side by side, 2x2 - three copies of the [orthogonal](../orthogonal/README.md) map and the [isometric](../isometric/README.md) map in the bottom-right corner, orientations mixed. It is Scarab's own, made for C7 (worlds). The maps are referenced one folder up (`../orthogonal/orthogonal.tmx`, `../isometric/isometric.tmx`), so it adds no map of its own. Every map has the same `ground` (id 1), `obstacles` (id 2) and `sprites` (id 3) layers, so the world's layer ids are those.

Only the `maps` list is read (Scarab, through sunlight, refuses a file that places maps by `patterns`).

Used in [samples/worlds](../../../samples/worlds/docs/README.md) (key `2`) and by `scripts/world_api_smoke`.
