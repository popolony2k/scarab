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

local PLAYER_LAYER   = 4    -- "clouds" in collision.tmx - a collider bucket here
local OBSTACLE_LAYER = 7    -- "monke" - likewise
local WALLS_LAYER    = 9    -- "walls": the brick tiles with a collision rectangle
local PLAYER_START   = { x = 300, y = 320 }   -- map pixels, inside the box (cols 8-12, rows 9-11)
local OBSTACLE_POS   = { x = 520, y = 320 }   -- outside the box, past the gap on its right
local SPEED          = 3    -- map pixels per frame per held direction
local ZOOM           = 3.8125   -- the zoom it starts at AND the one zoom_reset returns to (preferred_zoom)

-- The renderer is created explicitly so the reset zoom matches the start zoom: the default renderer
-- starts at 3.8125 but its preferred zoom (what zoom_reset returns to) is 1.0.
assert( renderer_create{ title = "Scarab - collision sample", zoom = ZOOM, preferred_zoom = ZOOM } )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

assert( tilemap_load_map( "resources/tilemap/collision.tmx", MAP_ALIGNMENT_CENTER ) )
assert( pool_register_type( "sunny", 2 ) )

local function add_sunny( pos, layerId, mode, delay )
    local handle = assert( sprite_acquire( "sunny" ) )

    assert( sprite_configure_texture( handle, 0, "resources/sprites/sunny_idle_down.png", 4, 0, mode, delay ) )
    assert( sprite_set_active_sequence( handle, 0 ) )
    assert( sprite_set_world_space( handle, true ) )
    assert( sprite_set_pos( handle, pos.x, pos.y ) )
    assert( sprite_add_to_layer( handle, layerId ) )

    return handle
end

local player   = add_sunny( PLAYER_START, PLAYER_LAYER, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 100 )   -- 100 ms per frame (the default -1 holds the frame)
local obstacle = add_sunny( OBSTACLE_POS, OBSTACLE_LAYER, TEXTURE_ANIMATION_MODE_MANUAL, -1 )

assert( collision_add_rule( PLAYER_LAYER, OBSTACLE_LAYER ) )
assert( collision_add_tile_rule( PLAYER_LAYER, WALLS_LAYER ) )

local frameCount            = 0
local hitCount              = 0
local lastHitFrame          = -1000
local __HIT_COOLDOWN_FRAMES = 30  -- ~0.5s at 60fps - avoids counting every frame of one overlap as a separate hit

collision_set_handler( function( handleA, handleB )
    if ( frameCount - lastHitFrame ) >= __HIT_COOLDOWN_FRAMES then
        hitCount     = hitCount + 1
        lastHitFrame = frameCount
    end
end )

-- The position before this frame's movement, and whether it was already restored this frame:
-- the tile handler fires once per wall tile touched, the undo must happen once.
local beforeX, beforeY = PLAYER_START.x, PLAYER_START.y
local undoneFrame      = -1
local wallStops        = 0

collision_set_tile_handler( function( handle, gid, x, y, width, height )
    if handle == player and undoneFrame ~= frameCount then
        sprite_set_pos( player, beforeX, beforeY )   -- the whole frame's movement, both axes
        undoneFrame = frameCount
        wallStops   = wallStops + 1
    end
end )

local TEXT_SIZE     = 20
local PLATE_PADDING = 4     -- pixels around a line of text on its plate
local PLATE_ALPHA   = 170   -- the plate's opacity, 0..255

-- A line of text on a dark translucent plate - the map behind it is mostly light sky and clouds.
local function draw_plate_text( text, x, y, r, g, b )
    draw_filled_rectangle( x - PLATE_PADDING, y - PLATE_PADDING, measure_text( text, TEXT_SIZE ) + 2 * PLATE_PADDING,
                           TEXT_SIZE + 2 * PLATE_PADDING, 0, 0, 0, PLATE_ALPHA )
    draw_text( text, x, y, TEXT_SIZE, r, g, b, 255 )
end

function on_update( dt )

    frameCount = frameCount + 1

    local x, y = sprite_get_pos( player )

    beforeX, beforeY = x, y

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then x = x - SPEED end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then x = x + SPEED end
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then y = y - SPEED end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then y = y + SPEED end

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

    draw_plate_text( "collision sample - walk Sunny into the brick walls, out of the gap on the right, into the other Sunny", 20, 20, 255, 255, 255 )
    draw_plate_text( string.format( "wall stops: %d   sprite hits: %d   zoom %.2f", wallStops, hitCount, viewport_get_zoom_factor() ),
                     20, 50, 220, 220, 220 )
    draw_plate_text( "arrows/WASD move   PageUp/PageDown or +/- zoom   = or Home reset zoom", 20, 80, 220, 220, 220 )
end
