# collision sample

Two sprites, one keyboard-controlled and one fixed nearby — move into it to trigger `collision_set_handler`. Builds on [sprite](../../sprite/docs/README.md). See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/collision/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Move the player sprite into the fixed one |

## What it shows

- [main.lua](../main.lua) — `collision_add_rule(4, 7)` pairs the player's layer (`4`) against the fixed sprite's layer (`7`); `collision_set_handler` registers the single global callback fired whenever a sprite on one ruled layer overlaps a sprite on the other.
- **The "Immunity gotcha" handled correctly**: `collision_set_handler`'s own docs warn that an unguarded handler re-fires every single frame two sprites stay overlapping, not just once per "touch." This sample tracks a simple frame-count cooldown (`__HIT_COOLDOWN_FRAMES`) so one overlap only ever counts once, no matter how many frames it lasts.
- **A real `LuaSpriteApi` limitation this sample deliberately avoids**: both sprites reuse the same `sunny_idle_down.png` texture rather than a visually distinct "obstacle" asset. `sprite_configure_texture`/`sprite_set_active_sequence` derive a sprite's displayed *height* from the texture's full native height, not from the computed per-frame tile width — correct for a simple horizontal strip (native height already equals the tile size, true here), but a genuine 2D icon grid (like sunlight's own `monke_variants.png`, a 4×4 grid) would come out visibly stretched, since there's no way to override the derived height from Lua. Sticking to one known-good strip asset sidesteps that entirely — see the comment at the top of `main.lua` for the full reasoning.

## Lua API reference

- [`collision_add_rule`/`collision_set_handler`](https://popolony2k.github.io/scarab/lua-api/collision.html)
