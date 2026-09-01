# text sample

HUD-style text layout, beyond `hello-world`'s single static `draw_text` call: a border drawn exactly on the fixed render resolution's own edges, a growing counter right-aligned against it via `measure_text`, and two real custom fonts loaded via `set_font` — a single-file TrueType font and a multi-file AngelCode BMFont atlas. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/text/project.json
```

## What it shows

- [main.lua](../main.lua) — `screen_get_width()`/`screen_get_height()` draw a border exactly on the fixed `1260x920` render target's own edges, confirming these describe that fixed design resolution, not the live/resizeable OS window size.
- `measure_text` right-aligns a "Count: N" counter against the right edge — the *x* position is recomputed every frame from the text's own measured width, not hardcoded, since the digit count grows over time (`Count: 9` vs. `Count: 10` are different widths), and now also reflects the active custom font's own real glyph metrics.
- `set_font` is called **three** times: first with a path that can't exist, purely to show its documented graceful-failure behavior (returns `false`, leaves whatever font was already active — the engine's built-in default, at that point — untouched, no error or crash); then with [Press Start 2P](../../../resources/fonts/README.md) (OFL-licensed, single-file TrueType); then with `caravellius8x8.fnt` (an original bitmap font made for Caravellius, a *multi-file* AngelCode BMFont atlas — a separate `.png` referenced from the `.fnt`). The last call wins, so `caravellius8x8` ends up the font actually shown for the rest of the sample. Same call shape regardless of format — `set_font` has no format-specific logic of its own, raylib auto-detects TrueType/OpenType vs. a BMFont atlas purely from the file's own extension. The sample's own HUD shows all three return values on screen.

## A real engine bug this sample used to avoid — now fixed and demonstrated

`caravellius8x8.fnt`'s multi-file load used to fail: a real, verified engine bug meant multi-file AngelCode BMFont loading (a `.fnt` plus its own separate `.png` atlas) failed under Scarab's mount-based filesystem hook, even though the exact same files loaded correctly through plain raylib on its own. **Fixed in [sunlight v0.17.3](https://github.com/popolony2k/sunlight/releases/tag/v0.17.3)** — see root `CLAUDE.md`'s "Known gotchas" and [resources/fonts/README.md](../../../resources/fonts/README.md) for the full, precisely-isolated story and the fix itself. This sample now demonstrates the real, working multi-file BMFont load as a result, rather than avoiding it.

## Lua API reference

- [`set_font`/`measure_text`/`screen_get_width`/`screen_get_height`](https://popolony2k.github.io/scarab/lua-api/text.html)
- [`draw_filled_rectangle`](https://popolony2k.github.io/scarab/lua-api/app.html) (the border)
