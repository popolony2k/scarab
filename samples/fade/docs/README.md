# fade sample

View opacity: a cross-fade between two places of one map, and a menu drawn over the map at a chosen opacity. Kepi, Scarab's mascot, walks in map coordinates, so every view draws it where that view shows its place. Built on the [views sample](../../views/docs/README.md). See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/fade/project.json
```

## Controls

| Keyboard | Controller | Action |
| --- | --- | --- |
| Arrow keys | D-pad / left stick | Walk Kepi |
| `F` | `A` | Cross-fade to the other place, and back (one second; pressing it mid-fade turns back from where it is) |
| `M` | `Y` | Open or close the menu |
| `[` / `]` | `L1` / `R1` | The menu's opacity down / up by 0.05 |

The controller is the first one connected.

## What it shows

- **A cross-fade** (`view_set_opacity`): two views cover the same rectangle. The default view shows the map's top-left corner, and the "scene" view, drawn above it, the bottom-right one. The scene view starts at opacity `0`; `F` moves it to `1` (or back) over one second, timed with `app_get_time`, so one place dissolves into the other.
- **A menu over the map:** a smaller square view in the middle, showing the whole map (`view_fit_to_map`), drawn above both (`view_set_draw_order`) at 85%. The scene shows through it; `[` and `]` take it from `0` (only the menu's text is left: text is drawn after every view, so it is not faded with it) to `1` (opaque).
- **What fades:** a view's finished image, not each thing in it - a house still hides what is behind it at any opacity - and its backdrop fades with it. At `0` a view draws nothing, but its sprites keep animating. The HUD shows each view's opacity (`view_get_opacity`).
- **Kepi is world space** (`sprite_set_world_space`): during a fade it fades out in one place and in with the other view.

Fading between two *different* maps uses worlds (several maps at once): the [worlds](../../worlds/docs/README.md) sample does it.

## Lua API reference

- [Views: view opacity](https://popolony2k.github.io/scarab/lua-api/views.html), [Views: draw order and layers](https://popolony2k.github.io/scarab/lua-api/views.html), [Sprites: world space](https://popolony2k.github.io/scarab/lua-api/sprite.html)
