# text sample

HUD-style text layout, beyond `hello-world`'s single static `draw_text` call: a border drawn exactly on the fixed render resolution's own edges, a growing counter right-aligned against it via `measure_text`, and a real custom font loaded via `set_font`. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/text/project.json
```

## What it shows

- [main.lua](../main.lua) — `screen_get_width()`/`screen_get_height()` draw a border exactly on the fixed `1260x920` render target's own edges, confirming these describe that fixed design resolution, not the live/resizeable OS window size.
- `measure_text` right-aligns a "Count: N" counter against the right edge — the *x* position is recomputed every frame from the text's own measured width, not hardcoded, since the digit count grows over time (`Count: 9` vs. `Count: 10` are different widths), and now also reflects the active custom font's own real glyph metrics.
- `set_font` is called **twice**: first with a path that can't exist, purely to show its documented graceful-failure behavior (returns `false`, leaves whatever font was already active — the engine's built-in default, at that point — untouched, no error or crash); then with [Press Start 2P](../../../resources/fonts/README.md) (OFL-licensed), to show an actual successful custom-font swap. The sample's own HUD shows both return values on screen.

## A real engine bug this sample avoids, on purpose

`resources/fonts/` also holds an original bitmap font made for Caravellius (`caravellius8x8.fnt`) — this sample deliberately does **not** use it. A real, verified engine bug means multi-file AngelCode BMFont loading (a `.fnt` plus its own separate `.png` atlas) currently fails under Scarab's mount-based filesystem hook, even though the exact same files load correctly through plain raylib on its own — see root `CLAUDE.md`'s "Known gotchas" and [resources/fonts/README.md](../../../resources/fonts/README.md) for the full, precisely-isolated story. Press Start 2P is a single-file TrueType font, unaffected by that bug, which is exactly why it's the one used here.

## Lua API reference

- [`set_font`/`measure_text`/`screen_get_width`/`screen_get_height`](https://popolony2k.github.io/scarab/lua-api/text.html)
- [`draw_filled_rectangle`](https://popolony2k.github.io/scarab/lua-api/app.html) (the border)
