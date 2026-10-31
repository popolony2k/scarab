--[[
 Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
]]

--[[
 collision - sprite against sprite and sprite against map tiles
 (docs/lua-api/collision.html), built on the "sprite" sample.

 resources/tilemap/collision.tmx is test.tmx plus a "walls" tile layer
 (id 9): a box of brick tiles whose one tile has a collision rectangle,
 drawn in Tiled's Tile Collision Editor (sunlight's own collision sample
 map). Sunny starts inside the box:
   - collision_add_tile_rule( 4, 9 ) checks her against every wall tile
     she overlaps, in MAP space (the camera and the zoom don't change where
     she stops). The tile handler fires once per wall tile touched, so in a
     corner it fires twice in one frame: the handler undoes the frame's
     movement ONCE - all of it, both axes of a diagonal, or she would work
     her way through a wall. Touching edges don't count, so she can rest
     flush against a wall. The gap on the right of the box lets her out.
   - collision_add_rule( 4, 7 ) pairs her with a fixed second Sunny outside
     the box; collision_set_handler counts the hits, with a short cooldown
     so one overlap counts once, not once per frame (see
     collision_set_handler's own "Immunity gotcha").

 Both sprites are world-space (sprite_set_world_space): their positions are
 map positions, so they line up with the walls whatever the camera, and a
 sprite-against-sprite rule between two world-space sprites compares map
 positions on both sides.

 NOTE: both sprites reuse the same sunny_idle_down.png texture rather
 than a visually distinct "obstacle" asset - sprite_configure_texture
 derives a sprite's displayed HEIGHT from the texture's full native
 height, not from the computed per-frame tile size, so it only produces
 an undistorted sprite for a simple horizontal strip (native height ==
 tile size, true for sunny_idle_down.png) - a genuine 2D icon grid like
 monke_variants.png would come out stretched.
]]

local __PLAYER_LAYER   <const> = 4    -- "clouds" in collision.tmx - a collider bucket here
local __OBSTACLE_LAYER <const> = 7    -- "monke" - likewise
local __WALLS_LAYER    <const> = 9    -- "walls": the brick tiles with a collision rectangle
local __PLAYER_START   <const> = { x = 300, y = 320 }   -- map pixels, inside the box (cols 8-12, rows 9-11)
local __OBSTACLE_POS   <const> = { x = 520, y = 320 }   -- outside the box, past the gap on its right
local __SPEED          <const> = 3    -- map pixels per frame per held direction
local __ZOOM           <const> = 3.8125   -- the zoom it starts at AND the one zoom_reset returns to (preferred_zoom)

local __SUNNY_FRAMES      <const> = 4     -- sunny_idle_down.png: four 32x32 frames side by side
local __SUNNY_FRAME_MILLI <const> = 100   -- per frame (the default -1 holds the frame)
local __HELD_FRAME        <const> = -1    -- sprite_configure_texture's "never step" delay

local Hud = dofile( "samples/shared/hud.lua" )

-- The renderer is created explicitly so the reset zoom matches the start zoom: the default renderer
-- starts at 3.8125 but its preferred zoom (what zoom_reset returns to) is 1.0.
assert( renderer_create{ title = "Scarab - collision sample", zoom = __ZOOM, preferred_zoom = __ZOOM } )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

assert( tilemap_load_map( "resources/tilemap/collision.tmx", MAP_ALIGNMENT_CENTER ) )
assert( pool_register_type( "sunny", 2 ) )

local function add_sunny( pos, layerId, mode, delay )
    local handle = assert( sprite_acquire( "sunny" ) )

    assert( sprite_configure_texture( handle, 0, "resources/sprites/sunny_idle_down.png", __SUNNY_FRAMES, 0, mode, delay ) )
    assert( sprite_set_active_sequence( handle, 0 ) )
    assert( sprite_set_world_space( handle, true ) )
    assert( sprite_set_pos( handle, pos.x, pos.y ) )
    assert( sprite_add_to_layer( handle, layerId ) )

    return handle
end

local player   = add_sunny( __PLAYER_START, __PLAYER_LAYER, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, __SUNNY_FRAME_MILLI )
local obstacle = add_sunny( __OBSTACLE_POS, __OBSTACLE_LAYER, TEXTURE_ANIMATION_MODE_MANUAL, __HELD_FRAME )

assert( collision_add_rule( __PLAYER_LAYER, __OBSTACLE_LAYER ) )
assert( collision_add_tile_rule( __PLAYER_LAYER, __WALLS_LAYER ) )

local __HIT_COOLDOWN_FRAMES <const> = 30  -- ~0.5s at 60fps - avoids counting every frame of one overlap as a separate hit

local frameCount   = 0
local hitCount     = 0
local lastHitFrame = -__HIT_COOLDOWN_FRAMES     -- "long enough ago": the first hit counts

collision_set_handler( function( handleA, handleB )
    if ( frameCount - lastHitFrame ) >= __HIT_COOLDOWN_FRAMES then
        hitCount     = hitCount + 1
        lastHitFrame = frameCount
    end
end )

-- The position before this frame's movement, and whether it was already restored this frame:
-- the tile handler fires once per wall tile touched, the undo must happen once.
local beforeX, beforeY = __PLAYER_START.x, __PLAYER_START.y
local undoneFrame      = -1
local wallStops        = 0

collision_set_tile_handler( function( handle, gid, x, y, width, height )
    if handle == player and undoneFrame ~= frameCount then
        sprite_set_pos( player, beforeX, beforeY )   -- the whole frame's movement, both axes
        undoneFrame = frameCount
        wallStops   = wallStops + 1
    end
end )

function on_update( dt )

    frameCount = frameCount + 1

    local x, y = sprite_get_pos( player )

    beforeX, beforeY = x, y

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then x = x - __SPEED end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then x = x + __SPEED end
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then y = y - __SPEED end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then y = y + __SPEED end

    sprite_set_pos( player, x, y )

    -- The zoom keys every sunlight/Scarab sample shares (see samples/camera): Page Up/Page Down
    -- or + / - zoom, = and Home reset it. + and = are one physical key, told apart by Shift.
    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    if input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or ( shift and input_is_key_down( KEY_EQUAL ) ) then
        zoom_in()
    end

    if input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then
        zoom_out()
    end

    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) or input_is_key_released( KEY_HOME ) then
        zoom_reset()
    end

    Hud.line( "collision sample - walk Sunny into the brick walls,", 1, Hud.TITLE )
    Hud.line( "out of the gap on the right, into the other Sunny", 2, Hud.TITLE )
    Hud.line( string.format( "wall stops: %d   sprite hits: %d   zoom %.2f", wallStops, hitCount, viewport_get_zoom_factor() ), 3, Hud.INFO )
    Hud.line( "arrows/WASD move   PageUp/PageDown or +/- zoom   = or Home reset zoom", 4, Hud.DIM )
end
