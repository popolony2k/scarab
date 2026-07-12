# Sprites

*Implemented in* `src/lua/luaspriteapi.cpp`, backed by `Engine::SpritePool` — a fixed-capacity pool of sprite slots, not a dynamically-growing list. Every sprite in the game (enemies, bullets, the player ship) is acquired from this same pool.

## Handles

`sprite_acquire` returns an opaque integer **handle**, not an object. `0` always means "invalid" (`INVALID_SPRITE_HANDLE`) — check for it after `sprite_acquire` in case the pool for that type is exhausted. A handle from a released slot doesn't silently alias a new sprite that reuses the same slot — every operation on a stale handle simply fails (returns `false`/`nil`) instead of touching the wrong sprite, since each slot's internal generation counter is bumped on release.

## The full lifecycle

```lua
-- 1. Register a pool for this sprite type, once, with a fixed capacity
pool_register_type("enemy_satellite", 50)

-- 2. Acquire a handle
local handle = sprite_acquire("enemy_satellite")
if handle == 0 then
  return  -- pool exhausted
end

-- 3. Configure at least one texture sequence (sequence 0 is the convention
--    for "main" appearance; sequence 1 is commonly "explosion", but any
--    sequence id is valid - it's just an index you choose)
sprite_configure_texture(handle, 0, BASE_PATH .. "sprites/satellite-ship.png",
                          3, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 150)
sprite_set_active_sequence(handle, 0)

-- 4. Position it and make it visible on a Tiled layer
sprite_set_pos(handle, 100, 0)
sprite_add_to_layer(handle, LAYER_ENEMIES_SHIPS)

-- ... later, on death/despawn ...

-- 5. Remove from the layer BEFORE releasing - the pool has no idea which
--    layer(s) a handle was added to, so skipping this step leaves a
--    dangling collider that keeps firing collision callbacks against a
--    handle that no longer resolves to anything.
sprite_remove_from_layer(handle, LAYER_ENEMIES_SHIPS)
sprite_release(handle)
```

## `pool_register_type(typeTag, capacity) -> success`

Reserve `capacity` slots for sprites acquired with this `typeTag` string. Must be called once before the first `sprite_acquire` for that tag.

## `sprite_acquire(typeTag) -> handle`

Get a free handle from the pool registered for `typeTag`. Returns `0` if the pool is exhausted (every slot currently in use).

## `sprite_release(handle) -> success`

Return a handle to its pool, making the slot available for a future `sprite_acquire`. **Call `sprite_remove_from_layer` first** if the sprite was ever added to a layer (see the gotcha above).

## `sprite_configure_texture(handle, sequenceId, path, framesByTexture, activeTileIndex, animationMode, delayMilli?) -> success`

Load a texture file and configure it as animation `sequenceId` on `handle`. `path` points at a texture sheet containing `framesByTexture` equal-width frames laid out horizontally; `activeTileIndex` is which frame to start on. `delayMilli` (optional, defaults to `-1`) controls automatic switching to a *different* texture later added to this same sequence — pass `-1` to disable that switching entirely, which is what every sprite in Caravellius does today (each sequence holds exactly one texture). It's unrelated to the per-frame tile animation within that one texture, which `animationMode` controls.

```lua
-- a 5-frame strip, starting on frame 2, auto-animating
sprite_configure_texture(handle, 0, path, 5, 2, TEXTURE_ANIMATION_MODE_ANIMATE_CENTER, 100)
```

Animation mode constants:

| Constant | Behavior |
|---|---|
| `TEXTURE_ANIMATION_MODE_MANUAL` | No automatic frame advance — you control the active tile yourself |
| `TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR` | Cycles through frames automatically, wrapping around |
| `TEXTURE_ANIMATION_MODE_AUTOMATIC_RIGHT_LEFT` | Cycles automatically, bouncing back and forth |
| `TEXTURE_ANIMATION_MODE_ANIMATE_LEFT` / `_RIGHT` / `_CENTER` | Runtime-switchable directional animation — the player ship uses these to bank left/right/center in response to input (see `sprite_set_animation_mode` below) |

## `sprite_set_active_sequence(handle, sequenceId) -> success`

Switch which configured sequence is currently displayed/collided against (also syncs the sprite's bounding box to that sequence's size — important since a "main" and an "explosion" sequence are often different sizes).

```lua
sprite_set_active_sequence(handle, 1)  -- switch to the explosion sequence
```

## `sprite_get_active_sequence(handle) -> sequenceId`

## `sprite_set_animation_mode(handle, sequenceId, mode) -> success`

Change a sequence's animation mode *after* it's already configured — for sprites that switch modes at runtime (e.g. the player ship banking left/right based on input). Enemies that only ever set their mode once typically just pass the final mode to `sprite_configure_texture` and never call this.

```lua
sprite_set_animation_mode(handle, 0, TEXTURE_ANIMATION_MODE_ANIMATE_LEFT)
```

## Position and size

```lua
sprite_get_pos(handle) -> x, y
sprite_set_pos(handle, x, y)
sprite_get_size(handle) -> width, height
```

Position is **top-left anchored**, not center-anchored — `x, y` is the sprite's top-left corner, matching both the render clip rect and the AABB collision math. `pos - height/2` means "above the sprite's own top edge," not "above center."

## Visibility

```lua
sprite_set_visible(handle, visible)
sprite_get_visible(handle) -> visible
```

## Layers

```lua
sprite_add_to_layer(handle, layerId) -> success
sprite_remove_from_layer(handle, layerId) -> success
```

Adding a sprite to a layer is also what registers it for collision detection on that layer (see [collision.md](collision.md)) — a sprite never added to any layer never collides with anything, regardless of `collision_add_rule`.
