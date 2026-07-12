# Collision

*Implemented in* `src/lua/luacollisionapi.cpp` and `src/lua/luacollisionlistener.cpp`.

Collision detection in this engine is entirely **layer-based**: you declare which pairs of Tiled layers should be checked against each other (`collision_add_rule`), and register one global handler function that's called whenever any two colliding sprites belong to a ruled-together pair. There's no per-sprite "on hit" registration — dispatch to the right game logic based on the two handles is entirely up to your handler.

## `collision_add_rule(layerA, layerB) -> success`

Declare that sprites on `layerA` should be checked against sprites on `layerB` every frame. Order doesn't matter for the check itself, but your handler always receives the two handles in a fixed order matching how the *engine* discovered the pair, not necessarily the order you declared the rule in.

```lua
collision_add_rule(LAYER_PLAYER_SHIP, LAYER_ENEMIES_SHIPS)
collision_add_rule(LAYER_PLAYER_SHIP_BULLETS, LAYER_ENEMIES_SHIPS)
```

Declaring `collision_add_rule(x, x)` with both arguments the same layer causes every collider on that layer to "collide with itself" every frame (trivially, at distance 0) — always use two distinct layers.

## `collision_add_tile_rule(layerId, tileLayerId) -> success`

Declare that sprites on `layerId` should be checked against the static tiles of `tileLayerId` (e.g. terrain collision, as opposed to sprite-vs-sprite).

## `collision_set_handler(fn)`

Register the single global function called whenever two sprites on a ruled pair of layers collide: `fn(handleA, handleB)`.

```lua
collision_set_handler(function(handleA, handleB)
  -- dispatch based on which layer(s) these handles actually belong to -
  -- the engine doesn't tell you that here, your own game bookkeeping does
  if Enemies.owners[handleA] then
    Enemies.owners[handleA].on_hit(handleA)
  end
end)
```

Only one handler can be registered at a time — a second `collision_set_handler` call replaces the first, it doesn't add a second listener.

**Immunity gotcha**: if your handler doesn't guard against re-triggering on a sprite that's already reacting to a previous hit (already exploding, already blinking invincible, etc.), two sprites overlapping for several consecutive frames re-fires the handler every single frame of that overlap — not just once per "touch." Track and check hit/invincibility state yourself inside the handler; the engine has no concept of "already handled this collision."

## `collision_set_tile_handler(fn)`

Register the handler for sprite-vs-tile collisions (from `collision_add_tile_rule`): `fn(handle, gid, x, y, width, height)` — `gid` is the tile's Tiled global id, `x`/`y`/`width`/`height` its world-space position and size.

```lua
collision_set_tile_handler(function(handle, gid, x, y, width, height)
  print(("hit tile gid %d at %d,%d"):format(gid, x, y))
end)
```
