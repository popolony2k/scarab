# resources/tilemap/parallax/

A map made to show Tiled's **per-layer parallax**, from [sunlight](https://github.com/popolony2k/sunlight)'s own [`parallax` sample](https://github.com/popolony2k/sunlight/tree/feat/master_plan/samples/parallax) at commit `ad84cca`, shared here because both repos are owned by the same project. Nothing has been changed; re-sync from sunlight rather than editing it on its own.

- `parallax.tmx`: 40x26 tiles of 32 px, with a parallax origin of (-640, -416). Its layers, back to front, with their `parallaxx`/`parallaxy`:
  - `terrain` (id 1) and `buildings` (id 2) at 1, a normal layer;
  - `sky` (id 3, opacity 0.85) and `birds` (id 5) at 0.25;
  - `clouds` (id 4, opacity 0.95) at 0.5.

  The object layer `markers` holds four points, `wrap_left`, `wrap_right`, `wrap_top` and `wrap_bottom`, which mark the rectangle a walk wraps around.
- Nine external tilesets (`tileset_*.tsx`) and their atlases in `images/`.

Used in [samples/parallax](../../../samples/parallax/docs/README.md) and by `scripts/layer_parallax_smoke`.
