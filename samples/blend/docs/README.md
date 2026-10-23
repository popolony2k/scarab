# blend sample

Blend modes: how what is drawn next combines with what is already drawn. Runs over the same map [tilemap](../../tilemap/docs/README.md) loads, and is modelled on sunlight's own [`blend` sample](https://github.com/popolony2k/sunlight/tree/feat/master_plan/samples/blend). See [samples/README.md](../../README.md) for the full sample list.

Three things use a mode:
- **Sunny's glow**, a sprite whose own mode the number keys choose (`sprite_set_blend_mode` / `sprite_clear_blend_mode`).
- **A soft shadow** that always multiplies (`sprite_set_blend_mode( shadow, BLEND_MODE_MULTIPLY )`).
- **Four panels** the script draws itself, one per mode, between `draw_begin_blend_mode` and `draw_end_blend_mode`.

## Running

From the repo root:

```shell
./build/scarab samples/blend/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Move Sunny, and its glow with it |
| `1` | Glow with no mode of its own: it draws by whatever mode is in effect (normal) |
| `2` | Glow in `BLEND_MODE_NORMAL` |
| `3` | Glow in `BLEND_MODE_ADD` (the default) |
| `4` | Glow in `BLEND_MODE_MULTIPLY` |
| `5` | Glow in `BLEND_MODE_SUBTRACT` |
| `Esc` | Quit |

## What to look for

- **The glow, drawn under Sunny:** in add, it brightens whatever is under it, towards white at its centre. In multiply, it tints the scene warm. In subtract, it darkens towards blue. In normal (or with no mode), it is a plain warm halo over the scene.
- **The shadow (bottom left):** darkens what is under it, with soft edges. Its PNG's transparent pixels are white on purpose: a naive multiply would paint them white, but sunlight's multiply is exact, so **no white box may show around the shadow**.
- **The panels (right):** the same red, green and blue squares (alpha 200), one panel per mode, each over its own backdrop (the add panel over a dark night, where added colours mix instead of saturating to white):
  - **NORMAL:** the squares cover each other.
  - **ADD:** the colours add: red and green make yellow, and all three make white.
  - **MULTIPLY:** each square tints the sky, and overlaps get darker.
  - **SUBTRACT:** each square takes its own colour out of the sky.
- **The labels and the HUD:** drawn in normal mode after each panel's `draw_end_blend_mode()`, so they look the same in every panel.

## What it shows

- [main.lua](../main.lua) — `renderer_create{ zoom = 1.0, preferred_zoom = 1.0 }` (zoom 1.0 instead of Scarab's default 3.8125, so the map and the 128×128 glow fit next to the panels).
- **Per sprite:** `sprite_set_blend_mode` keeps the shadow in multiply on every frame. The sprite switches to its mode for its own draw only, then back to the mode in effect. `set_glow_mode` calls `sprite_set_blend_mode` or `sprite_clear_blend_mode` on the glow.
- **Scoped:** `draw_panels` draws each panel's squares between `draw_begin_blend_mode( mode )` and `draw_end_blend_mode()`. Lua draws from `on_update`, after the map and the sprites, so a scoped mode never reaches a sprite; that is why sprites have their own.
- **Draw order:** the glow and the shadow are on layer `4` ("clouds") and Sunny on layer `5` ("houses"), so Sunny is drawn over its glow.
- **Assets:** `resources/sprites/glow.png` and `shadow.png`, shared from sunlight's sample (see [resources/sprites/README.md](../../../resources/sprites/README.md)).

## Lua API reference

- [`draw_begin_blend_mode`/`draw_end_blend_mode`/`BLEND_MODE_*`](https://popolony2k.github.io/scarab/lua-api/blend.html)
- [`sprite_set_blend_mode`/`sprite_clear_blend_mode`/`sprite_get_blend_mode`](https://popolony2k.github.io/scarab/lua-api/sprite.html)
