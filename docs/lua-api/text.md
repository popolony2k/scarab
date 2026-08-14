# Text

*Implemented in* `src/lua/luatextapi.cpp`.

Font loading and screen-space text rendering — a thin wrapper over `ITileMap::SetFont`/`DrawText`/`MeasureText`/`GetWindowWidth`/`GetWindowHeight`. All HUD logic (what to draw, when, positioning, score/lives formatting) lives in game-side Lua on top of these; this API knows nothing about what the text being drawn actually means. Kept as its own page/file (not folded into [app.md](app.md)) since text/font is its own subsystem, not a generic app-level concern.

## `set_font(path) -> success`

Load (or replace) the font used by `draw_text`/`measure_text`, from a font file on disk. The format is auto-detected purely by the file's own extension — TrueType/OpenType (`.ttf`/`.otf`) and an AngelCode BMFont atlas (`.fnt`) are both supported without this API (or the caller) needing to know which — `draw_text`'s own code has no format-specific logic at all.

Until this is called for the first time (or if every call so far has failed), `draw_text`/`measure_text` use the engine's own built-in default font, so text can be drawn with zero setup. A bad/missing path returns `false` and leaves whatever font was previously active untouched — it does not error or crash.

```lua
-- main.lua, early in bootstrap
set_font( BASE_PATH .. "fonts/msx-screen0.ttf" )
```

## `draw_text(text, x, y, size, r, g, b, a)`

Draw a line of text on screen, in screen space — no camera/viewport transform of its own, same as the FPS counter — using whichever font is currently active (see `set_font`). `r`/`g`/`b`/`a` are each `0`-`255`.

```lua
draw_text( "Lives: 5", 20, 20, 24, 255, 255, 255, 255 )
```

Meant to be called every frame from a normal `on_update`-style hook (e.g. via `Enemies.register_update`, the same per-frame dispatch every enemy/player module in Caravellius already uses) — nothing here is "issue once and it persists"; a HUD element only stays on screen for as long as something keeps calling `draw_text` for it each frame.

## `measure_text(text, size) -> width`

Measure how wide a line of text would render, in pixels, at a given font size — using the same font `draw_text` itself would use. Meant for HUD layout (e.g. right-aligning a score/lives readout against `screen_get_width()`) without hardcoding assumed widths, since digit/character count can change over a playthrough.

```lua
local text  = "Lives: " .. tostring( Lives.count )
local width = measure_text( text, 24 )
local x     = screen_get_width() - width - 20  -- flush to the right edge, with a 20px margin
```

## `screen_get_width() -> width` / `screen_get_height() -> height`

The engine's own fixed design/render resolution, in pixels (`DISPLAY_W`/`DISPLAY_H` in `main.h` — `1260x920` today) — the coordinate space `draw_text` and every other screen-space draw call operate in. Constant for the app's whole lifetime, set once at construction.

**Deliberately not the same as the live OS window size** — the engine always letterbox-scales this fixed resolution to fit whatever the real window size ends up being (whether from `app_set_fullscreen` or the player manually resizing the window, see [app.md](app.md)). HUD layout should always measure against these, not against anything that varies with the actual window.
