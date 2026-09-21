# views sample

Several **views** of the same world at once: the main picture, a **minimap** of the whole map, and a **close-up** that follows a sprite — all drawn every frame from one map and one sprite. Built on [renderer](../../renderer/docs/README.md) (choosing the window/render settings) and [sprite](../../sprite/docs/README.md) (the animated "sunny" character). See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/views/project.json
```

Also runs headless (nothing is drawn, no display needed):

```shell
./build/scarab --headless --fast --max-frames 300 --max-frames-ok samples/views/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` | Move the sprite (map pixels; `sprite_set_pos`) — the close-up follows it |
| `M` | Show/hide the minimap (`view_set_visible`) |
| `C` | Show/hide the close-up |
| `L` | Show/hide the *clouds* layer **in the minimap only** (`view_show_layer` — the main view keeps showing it) |
| `Space` | Switch the sprite's `sprite_set_world_space` off/on (off: the close-up loses the sprite) |

## What it shows

- [main.lua](../main.lua) — `view_create( renderer, x, y, w, h )` adds a view over the same world; ids are opaque integers (`0` is the default view). The minimap is `view_fit_to_map` (the largest zoom at which the whole map fits its rectangle, camera at the map's top-left) with a translucent backdrop (`view_set_background_color`); the close-up is `view_set_zoom( view, 2.0 )` with its camera re-pointed at the sprite every frame (`view_set_camera_position`, kept inside the map by the script — a camera is not re-clamped for you).
- **Layer masks are per view.** The minimap hides the *clouds* layer (`view_show_layer( minimap, "clouds", false )`); the main view and the close-up still show it. A layer given by name needs the map loaded; by id it does not.
- **World space is what makes one sprite right in every view.** A sprite's position is normally *relative to the view drawing it*, so the same `sprite_set_pos` lands somewhere different in a minimap and a close-up. With `sprite_set_world_space( sprite, true )` the position is a **map** position in pixels and every view draws the sprite where it draws the map at that point. Press `Space` to switch it off: the main view and the minimap look the same either way (their camera sits at the map origin, so only the zoom differs and both readings agree), but the **close-up** — whose camera follows the sprite — loses it, because the raw position lands outside its rectangle. To see it land in the *wrong place* instead of vanishing, walk the sprite to about 80 map pixels in from the top-left corner (right and down) and then press `Space`: the close-up's camera has moved away from the origin, but the raw position still falls inside its rectangle, so the sprite shows shifted from where it really is. Right in the corner every view agrees, because every camera is at the origin there.
- Limits worth knowing (from sunlight): each visible extra view is another full pass over the map every frame; collisions and input stay in the default view's space (do not mix world-space and screen-relative sprites in one collision rule); image layers are drawn only in the default view.

## Lua API reference

- [`view_create`/`view_destroy`/`view_fit_to_map`/`view_show_layer`/`view_set_visible`/…](https://popolony2k.github.io/scarab/lua-api/views.html)
- [`renderer_create`/`renderer_get_view_count`](https://popolony2k.github.io/scarab/lua-api/renderer.html)
- [`sprite_set_world_space`/`sprite_get_world_space`](https://popolony2k.github.io/scarab/lua-api/sprite.html)
