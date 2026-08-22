# App

*Implemented in* `src/lua/luaappapi.cpp`.

Application-level primitives — concerns that belong to the running app/game itself, not to any one engine subsystem (camera, sound, sprites, ...).

## `app_set_name(name)`

Set the application window's title. Scarab (the engine) has no opinion on what a game calls itself — the window opens with a generic default title (`"Scarab"`, `main.h`'s `APP_NAME`) before any Lua exists to override it, since the window has to exist before `main.lua`'s first line runs. Call this once, early, with the game's real name:

```lua
-- main.lua
app_set_name( "Caravellius" )
```

## `screen_fade(alpha)`

Set a whole-screen fade overlay, drawn on top of every other rendered element (tilemap, sprites, everything) — not a per-texture effect. `alpha` ranges from `0.0` (fully visible, no overlay) to `1.0` (fully black). Intended for stage-start/stage-end and game-over transitions: ramp it over several frames from `on_update` to fade in/out rather than jumping straight to an endpoint.

```lua
-- fade in from black over ~1 second at 60fps
local __FADE_STEP = 1 / 60
local fade_alpha = 1.0

function on_update( dt )
    if fade_alpha > 0 then
        fade_alpha = math.max( 0, fade_alpha - __FADE_STEP )
        screen_fade( fade_alpha )
    end
end
```

## `app_set_fullscreen(fullscreen)` / `app_get_fullscreen()`

Enter/leave fullscreen, or query the current state. The game always renders at it's own fixed internal resolution regardless of the actual window/monitor size — the engine handles scaling and letterboxing (black bars, preserving aspect ratio) to fit whatever the real window size ends up being, whether that's from this toggle or the player manually resizing the window. Nothing else needs to account for it.

```lua
-- toggle fullscreen on a key press (edge-triggered, not held)
if input_is_key_released( KEY_F5 ) then
    app_set_fullscreen( not app_get_fullscreen() )
end
```

## `app_set_draw_fps(draw_fps)` / `app_get_draw_fps()`

Show or hide the on-screen FPS counter (raylib's own, drawn at the top-left corner of the window every frame), or query the current state. Scarab's own default (`main.cpp`) is `false` — a generic engine-level choice, not a game one. Caravellius overrides it to `true`, early in `main.lua`:

```lua
-- main.lua
app_set_draw_fps( true )
```

## `app_set_window_resizeable(resizeable)` / `app_get_window_resizeable()`

Allow or disallow the user resizing the window by dragging its edges/corners, or query the current state. A genuine **live** toggle — safe to call both before the window exists and at any point after, in either direction (unlike raylib's own pre-`InitWindow`-only `FLAG_WINDOW_RESIZABLE` config flag). Scarab's own default (`main.cpp`) is `true`; Caravellius overrides it to `false` (the game renders at a fixed internal resolution with letterboxing, so a resizeable window offers no benefit):

```lua
-- main.lua
app_set_window_resizeable( false )
```

## `app_set_stretch_to_fill(stretchToFill)` / `app_get_stretch_to_fill()`

Choose how the engine's fixed internal render resolution (`DISPLAY_W`x`DISPLAY_H`, `1260x920` today — what `screen_get_width`/`screen_get_height` report, unrelated to the real window size) gets blitted onto the real window/screen whenever the two sizes differ (fullscreen, or a live-resized window). Recomputed every frame, so it applies immediately regardless of when it's called.

- `false` (the engine's own default, both Scarab's and Caravellius's) — **letterbox**: preserves the render target's own aspect ratio via one uniform scale factor, filling whichever axis doesn't fit exactly with black bars.
- `true` — **stretch-to-fill**: fills the entire window/screen with no black bars at all, using independent X/Y scale factors — the image visibly warps (stretches/squishes) whenever the window's own aspect ratio doesn't match the render target's `1260:920`.

```lua
app_set_stretch_to_fill( true )   -- fill the window completely, no letterboxing
```

## `draw_filled_rectangle(x, y, width, height, r, g, b, a)`

Draw a filled, solid-color rectangle in screen space — same coordinate system as `draw_text`/`screen_get_width` (the fixed `1260x920` design resolution, independent of the live window size, fullscreen state, or `app_set_stretch_to_fill`), not a world-space/camera-relative one. Meant for simple HUD elements — a progress/health bar, a meter — that don't warrant a whole sprite/texture asset. A thin pass-through to `ITileMap::DrawFilledRectangle`, itself a thin pass-through to `IEngine::DrawFilledRectangle` — the same underlying call the whole-screen `screen_fade` overlay already uses internally, just exposed here as it's own arbitrary-position/size/color primitive rather than that fixed, full-window, single-alpha-value overlay.

```lua
-- a simple health bar: a dark background plate, then a colored fill
-- proportional to some fraction (0.0-1.0)
draw_filled_rectangle( x, y, width, height, 40, 40, 40, 255 )
draw_filled_rectangle( x, y, math.floor( width * fraction ), height, 220, 40, 40, 255 )
```
