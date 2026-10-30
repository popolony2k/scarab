# collision sample

Sunny against map tiles and against another sprite. She starts inside a box of brick walls (map tiles with a Tiled collision rectangle) that stop her from every side; the gap on the right lets her out to a second Sunny, whose hits are counted. Builds on [sprite](../../sprite/docs/README.md). See [samples/README.md](../../README.md) for the full sample list.

## Running

From the repo root:

```shell
./build/scarab samples/collision/project.json
```

## Controls

| Key | Action |
| --- | --- |
| `Arrow keys` / `WASD` (held) | Move Sunny (two keys at once move diagonally) |
| `Page Up` / `Page Down`, or `+` / `-` (held, main keyboard or keypad) | Zoom in/out |
| `=` (main keyboard or keypad) or `Home` | Reset the zoom to the one it starts at (3.8125) |
| `Esc` | Quit |

## What to look for

- **The walls stop her from every side**: left, right, above, below, and in the corners, also moving diagonally (she stops at the wall rather than sliding through it). She can rest flush against a wall. The HUD counts each stop.
- **Zoom doesn't change where she stops**: tile collisions are worked out in map space.
- **The gap** on the right of the box (one tile) lets her out; walk her into the second Sunny and the hit count goes up once per touch.

## What it shows

- [main.lua](../main.lua) — the map is [`resources/tilemap/collision.tmx`](../../../resources/tilemap/README.md): `test.tmx` plus a `walls` tile layer (id 9) drawn with `tileset_wall.tsx`, whose one tile has a collision rectangle drawn in Tiled's Tile Collision Editor. A tile collides only if its tile has a collision shape.
- **Sprite against tiles:** `collision_add_tile_rule( 4, 9 )` checks Sunny (layer 4) against every wall tile she overlaps, every frame. `collision_set_tile_handler` gets `( handle, gid, x, y, width, height )` with the tile's **map** rectangle, and fires **once per tile touched**: two calls in a corner. So the handler undoes the frame's movement **once**, and all of it - a diagonal is two steps in one frame, and undoing only one would let her work her way through a wall (a lesson from sunlight's own `collision` sample). The handler runs after `on_update` in the same frame, so the undone position is the one drawn.
- **Sprite against sprite:** `collision_add_rule( 4, 7 )` pairs her with the second Sunny (layer 7); `collision_set_handler` counts the hits with a frame-count cooldown (`__HIT_COOLDOWN_FRAMES`), because a handler re-fires every frame two sprites stay overlapping (its "Immunity gotcha").
- **The zoom it starts at is the one it resets to:** `renderer_create{ zoom = 3.8125, preferred_zoom = 3.8125 }` - the default renderer would start at 3.8125 but reset to its preferred zoom, 1.0.
- **World space:** both sprites use `sprite_set_world_space( handle, true )`, so their positions are map positions that line up with the walls whatever the camera (the map is centre-aligned) or the zoom.
- **A `LuaSpriteApi` limitation this sample avoids**: both sprites reuse `sunny_idle_down.png` rather than a distinct "obstacle" asset. `sprite_configure_texture` derives a sprite's displayed *height* from the texture's full native height, so a genuine 2D icon grid (like sunlight's `monke_variants.png`) would come out stretched - see the comment at the top of `main.lua`.

## Lua API reference

- [`collision_add_rule` / `collision_add_tile_rule` / `collision_set_handler` / `collision_set_tile_handler`](https://popolony2k.github.io/scarab/lua-api/collision.html)
- [`sprite_set_world_space`](https://popolony2k.github.io/scarab/lua-api/sprite.html)
