# mapless sample

Sprites drawn with **no map loaded at all** (master plan item V8). Before sunlight's master plan, `sprite_add_to_layer` with no map loaded accepted a sprite and updated its collider every frame, but never drew it; now it is drawn, so a title screen, a menu or a game without a Tiled map can use sprites. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/mapless/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Move Sunny |
| `1` / `2` / `3` | Hide or show layer 1 (the shadow), 2 (Sunny), 3 (the glow) |
| `Page Up` / `Page Down`, or `+` / `-` (held, main keyboard or keypad) | Zoom in/out |
| `=` (main keyboard or keypad) | Reset the zoom |
| `Home` | Reset the zoom and Sunny's position |
| `Esc` | Quit |

There is no map to scroll over, so there are no camera keys: a sprite's position is relative to the view (scaled by its zoom).

## What to look for

- All three sprites draw on a black window, with no map. Sunny animates.
- **Draw order:** with no map, a layer id is only a grouping key, and the layers are drawn in ascending id order. Move Sunny: he passes **over** the shadow (layer 1) and **under** the glow (layer 3).
- **Layer mask:** `1`, `2` and `3` hide and show each sprite through the view's layer mask (`view_show_layer`), which hides mapless layers by id the same way it hides a map's. The HUD's second line shows each layer's state.
- Zoom scales every sprite together.

## What it shows

- [main.lua](../main.lua) — `renderer_create{ zoom = 2.0, ... }` and never `tilemap_load_map`; each sprite is `pool_register_type` / `sprite_acquire` / `sprite_configure_texture` / `sprite_add_to_layer` with its own layer id.
- `view_show_layer( 0, id, show )` / `view_is_layer_shown( 0, id )` on the default view.
- Building it found a Scarab bug: every sprite was also drawn a second time, on top of the frame, so the layer mask had no visible effect (master plan item V8a, fixed).

## Assets

`resources/sprites/` (Sunny, `glow.png`, `shadow.png`) and `resources/fonts/caravellius8x8.fnt` - see their READMEs.

## Lua API reference

- [`sprite_add_to_layer` and the sprite functions](https://popolony2k.github.io/scarab/lua-api/sprite.html)
- [`view_show_layer` / `view_is_layer_shown`](https://popolony2k.github.io/scarab/lua-api/views.html)
- [`renderer_create`](https://popolony2k.github.io/scarab/lua-api/renderer.html)
