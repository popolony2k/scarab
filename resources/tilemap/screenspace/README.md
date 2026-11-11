# resources/tilemap/screenspace/

Scarab's own map for a **screen-space image layer** (`plans/MASTER_PLAN.md` item C9, sunlight I23).

- `screenspace.tmx`: an orthogonal 60x40 map of 32 px tiles with no tile layer, and four image layers. Layer 1, "backdrop", has the Tiled custom property `screen_space` (bool) = `true`, so it is drawn in screen space; layers 2-4 ("house near", "house middle", "house far") are ordinary map-space image layers using `../images/house.png`.
- `backdrop.png`: a 1260x920 picture (Scarab's default render area) with a gold border, corner squares and a centre ring, so an eye check can see at once whether it is drawn whole. Drawn by `scripts/backdrop_image.py` (deterministic; run it to regenerate). Made for this repo, under the same zlib license as Scarab.

Used in [samples/backdrop](../../../samples/backdrop/docs/README.md).
