# text sample

HUD-style text layout and **fonts by handle**, beyond `hello-world`'s single static `draw_text` call: each font file is loaded once with `load_font`, several fonts are on screen at the same time, the active font can be switched by handle, and a counter is right-aligned with `measure_text`. See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/text/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `1` / `2` / `3` | Make caravellius8x8, caravellius8x8 bold or Press Start 2P the active font (`set_font( font )`) |
| `0` | Switch to the engine's built-in font (`set_font( nil )`) |
| `Esc` | Quit |

## What it shows

- [main.lua](../main.lua) — **load once**: `load_font` loads three fonts, each once, into handles — [caravellius8x8](../../../resources/fonts/README.md) regular and bold (the project's own *multi-file* AngelCode BMFont atlases, a `.fnt` plus a separate `.png`) and [Press Start 2P](../../../resources/fonts/README.md) (SIL OFL 1.1, a single-file TrueType font). The format is told apart by the file's extension.
- **Several fonts at once**: one line is drawn in each font through `draw_text`'s optional last argument, without changing the active font.
- **The active font**: the title line and the counter use no font argument, so they follow whichever font `set_font` selected — a handle (keys `1`-`3`) or `nil` for the built-in font (key `0`).
- **Accents**: a line in Portuguese and Spanish ("Ação, coração, açúcar - año, niño, mañana - café") in the TrueType font, which loads with Latin accents. A bitmap `.fnt` only has the glyphs its own atlas holds.
- **Errors**: `set_font` with a path that can't exist returns `nil` and the reason, shown on screen, and leaves the active font as it was.
- `measure_text` right-aligns a "Count: N" counter against `screen_get_width()`. Its *x* is recomputed every frame from the measured width, which changes with the digit count and with the active font.
- `screen_get_width()`/`screen_get_height()` draw a border exactly on the fixed `1260x920` render target's edges, confirming they describe that fixed resolution, not the live OS window size.
- The same handles could also serve a map's Tiled text objects (`register_font`), which the `objects` sample (master plan item V5) shows.

## A real engine bug this sample used to avoid — now fixed and demonstrated

`caravellius8x8.fnt`'s multi-file load used to fail: a real, verified engine bug meant multi-file AngelCode BMFont loading (a `.fnt` plus its own separate `.png` atlas) failed under Scarab's mount-based filesystem hook, even though the exact same files loaded correctly through plain raylib on its own. **Fixed in [sunlight v0.17.3](https://github.com/popolony2k/sunlight/releases/tag/v0.17.3)** — see root `CLAUDE.md`'s "Known gotchas" and [resources/fonts/README.md](../../../resources/fonts/README.md) for the full, precisely-isolated story and the fix itself. This sample now demonstrates the real, working multi-file BMFont load as a result, rather than avoiding it.

## Lua API reference

- [`load_font`/`release_font`/`set_font`/`draw_text`/`measure_text`](https://popolony2k.github.io/scarab/lua-api/text.html)
- [`screen_get_width`/`screen_get_height`](https://popolony2k.github.io/scarab/lua-api/app.html)
- [`draw_filled_rectangle`](https://popolony2k.github.io/scarab/lua-api/app.html) (the border)
