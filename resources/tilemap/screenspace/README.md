# resources/tilemap/screenspace/

Scarab's own map for a **screen-space image layer** (`plans/MASTER_PLAN.md` item C9, sunlight I23).

- `screenspace.tmx`: an orthogonal 60x40 map of 32 px tiles with no tile layer, and six image layers. Layer 1, "backdrop", has the Tiled custom property `screen_space` (bool) = `true`, so it is drawn in screen space; layers 2-4 ("house near", "house middle", "house far") are ordinary map-space image layers using `../images/house.png`. Layers 5 ("house stale size", map space) and 6 ("badge stale size", screen space, in the window's bottom-right corner) show the same 160x64 picture but DECLARE a larger size (200x80, 240x96), as a map does when its picture changed after it was saved: sunlight draws an image layer at its picture's real size (I24, `plans/MASTER_PLAN.md` item C10), so each shows one plain house, never a repeated strip.
- `backdrop.png`: a 1260x920 picture (Scarab's default render area) with a gold border, corner squares and a centre ring, so an eye check can see at once whether it is drawn whole. Drawn by `scripts/backdrop_image.py` (deterministic; run it to regenerate). Made for this repo, under the same zlib license as Scarab.

Used in [samples/backdrop](../../../samples/backdrop/docs/README.md).
