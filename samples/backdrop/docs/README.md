# backdrop sample

A **screen-space image layer**: a full-window picture that stays whole and fixed on the screen while the map under it scrolls and zooms - for a title or cover screen. It is an ordinary Tiled image layer with one custom property, `screen_space = true`. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/backdrop/project.json
```

## Controls

| Key | Action |
| --- | --- |
| Arrow keys | Pan the camera (same directions as [camera](../../camera/docs/README.md)) |
| `WASD` | Walk Kepi |
| `PageUp` / `PageDown`, `+` / `-` | Zoom in / out |
| `=` / `Home` | Reset the zoom |

## What to look for

- **The gold frame stays whole and fixed:** its four edges, its four corner squares and the ring in the middle are all on screen at every zoom and wherever the camera goes. It is not cut to the viewport either: it reaches the window's own edges, outside the viewport's 10-pixel margin.
- **The houses are map space:** they move when you pan and grow or shrink when you zoom.
- **Layer order:** Kepi stands on the backdrop layer (layer 1) and draws over the picture; the houses are later layers and draw over Kepi.
- **A picture is drawn at its real size:** the map declares a larger size than the picture really has for one map-space house (layer 5) and for the small screen-space house in the bottom-right corner (layer 6). Each must show one plain house - no extra slice of roof or wall repeated along its right or bottom edge.

## What it shows

- [main.lua](../main.lua) loads [`resources/tilemap/screenspace/screenspace.tmx`](../../../resources/tilemap/screenspace/README.md) at the default zoom (3.8125). Its layer 1, "backdrop", is an image layer with the Tiled custom property **`screen_space`** (type **bool**, value **true**): sunlight draws it at its offset in window pixels, at its own size, not moved by the camera, the zoom or the layer's parallax, and not cut to the viewport. Its opacity and tint still apply, and like every image layer it is drawn in the default view only.
- Layers 2-4 are ordinary image layers, drawn in map space like the rest of a map (since sunlight's L1; before it, every image layer was drawn in screen space).
- Kepi is a screen-space sprite added to layer 1 with `sprite_add_to_layer`: a sprite on a screen-space layer draws over it, as Caravellius' ship draws over its cover.
- No headless test can see whether the picture is drawn right (the null backend draws nothing): this sample is the eye check, and sunlight's own unit tests pin the drawing.

## Lua API reference

- [Tile map: Tiled custom properties](https://popolony2k.github.io/scarab/lua-api/tilemap.html), [`camera_move_*`/`zoom_*`](https://popolony2k.github.io/scarab/lua-api/camera.html), [Sprites](https://popolony2k.github.io/scarab/lua-api/sprite.html)
