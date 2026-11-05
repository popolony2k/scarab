# zorder sample

Which of a layer's sprites draws on top. Four overlapping cards share one layer, and their z-order (`sprite_set_z_order`) decides the stack; a glow on a higher layer stays above them all. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/zorder/project.json
```

## Controls

| Input | Action |
| --- | --- |
| `1` / `2` / `3` / `4` | Bring the red / green / blue / gold card to the front (a z-order above every other card's) |
| `Shift` + `1`-`4` | Send that card to the back (a z-order below every other card's) |
| `0` | Every card back to z-order `0` |

## What it shows

- **Z-order orders one layer:** every frame a layer's sprites are drawn in ascending z-order, so the highest is on top. The HUD lists each card's z-order and the order they are drawn in, back to front.
- **Equal z-orders keep the order they were added in:** the cards are added red, green, blue, gold. Press `0` after any number of changes, and that starting stack comes back.
- **Layers come first:** the glow is on layer 2, the cards on layer 1, so the glow is always above every card, whatever their z-orders.
- **No map needed:** with no map loaded, a layer id is only a grouping key (see [samples/mapless](../../mapless/docs/README.md)).

## Lua API reference

- [Sprites: z-order](https://popolony2k.github.io/scarab/lua-api/sprite.html)
