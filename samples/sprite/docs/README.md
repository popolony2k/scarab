# sprite sample

Acquiring, configuring, and moving a sprite, on top of the same map [tilemap](../../tilemap/docs/README.md) loads. See [samples/README.md](../../README.md) for the full sample list; [collision](../../collision/docs/README.md) builds on this one next.

## Running

From the repo root:

```shell
./build/scarab samples/sprite/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Move the sprite (`sprite_set_pos`) |

## What it shows

- [main.lua](../main.lua) — `pool_register_type`/`sprite_acquire` (the `SpritePool` CRUD pattern every sprite goes through), `sprite_configure_texture` (a 4-frame, 32×32-per-frame strip, `TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR`), `sprite_set_active_sequence`, `sprite_set_pos`, and `sprite_add_to_layer` — required before a sprite draws at all, and gated by that layer's own `visible` flag in the `.tmx` (layer `4`, "clouds", is visible by default here — see root `CLAUDE.md`'s own gotcha on this).
- **Load-order matters, not "top-level vs. per-frame"**: root `CLAUDE.md`'s own gotcha warns that acquiring/positioning/adding a sprite to a layer from a module's top-level code fails silently if that code runs *before* the map is loaded. This script is safe specifically because `tilemap_load_map` runs synchronously, first, and every sprite call happens strictly after it in the same top-level script — not because top-level sprite setup is safe in general.
- Every frame, `on_update` reads the sprite's current position via `sprite_get_pos`, applies keyboard movement, and writes it back via `sprite_set_pos` — `sprite_get_size` is also read and shown, confirming it reflects the configured texture's real per-frame size (32×32), not the full 128×32 sheet.

## Lua API reference

- [`pool_register_type`/`sprite_acquire`/`sprite_configure_texture`/`sprite_add_to_layer`/`sprite_set_pos`/`sprite_get_pos`/`sprite_get_size`](https://popolony2k.github.io/scarab/lua-api/sprite.html)
