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
 collision - colliding two sprites (docs/lua-api/collision.html), built
 on the "sprite" sample.

 Two "sunny" sprites (resources/sprites/, see that folder's own README):
 one keyboard-controlled, one fixed in place nearby. collision_add_rule
 pairs their two layers; collision_set_handler fires whenever they
 overlap, with a short cooldown so a multi-frame overlap counts once,
 not once per frame (see collision_set_handler's own "Immunity gotcha").

 NOTE: both sprites reuse the same sunny_idle_down.png texture rather
 than a visually distinct "obstacle" asset (e.g. sunlight's own sample
 uses monke_variants.png, a 4x4 icon grid) - sprite_configure_texture
 derives a sprite's displayed HEIGHT from the texture's full native
 height, not from the computed per-frame tile size, so it only produces
 an undistorted sprite for a simple horizontal strip (native height ==
 tile size, true for sunny_idle_down.png) - a genuine 2D icon grid like
 monke_variants.png would come out stretched. Sticking to one
 known-good asset avoids that entirely.
]]

app_set_name( "Scarab - collision sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER )

pool_register_type( "sunny", 2 )

local player = sprite_acquire( "sunny" )

sprite_configure_texture( player, 0, "resources/sprites/sunny_idle_down.png", 4, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR )
sprite_set_active_sequence( player, 0 )
sprite_set_pos( player, 100, 90 )
sprite_add_to_layer( player, 4 )   -- "clouds" in test.tmx - just a collider bucket here, unrelated to it's tile content

local obstacle = sprite_acquire( "sunny" )

sprite_configure_texture( obstacle, 0, "resources/sprites/sunny_idle_down.png", 4, 0, TEXTURE_ANIMATION_MODE_MANUAL )
sprite_set_active_sequence( obstacle, 0 )
sprite_set_pos( obstacle, 172, 94 )
sprite_add_to_layer( obstacle, 7 )  -- "monke" in test.tmx - likewise, just a second collider bucket

collision_add_rule( 4, 7 )

local hitCount           = 0
local frameCount         = 0
local lastHitFrame       = -1000
local __HIT_COOLDOWN_FRAMES = 30  -- ~0.5s at 60fps - avoids counting every frame of one overlap as a separate hit

collision_set_handler( function( handleA, handleB )
    if ( frameCount - lastHitFrame ) >= __HIT_COOLDOWN_FRAMES then
        hitCount     = hitCount + 1
        lastHitFrame = frameCount
    end
end )

local __SPEED = 3

function on_update( dt )

    frameCount = frameCount + 1

    local x, y = sprite_get_pos( player )

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then
        x = x - __SPEED
    end

    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then
        x = x + __SPEED
    end

    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then
        y = y - __SPEED
    end

    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then
        y = y + __SPEED
    end

    sprite_set_pos( player, x, y )

    draw_text( "collision sample - move the sprite into the fixed one", 20, 20, 20, 255, 255, 255, 255 )
    draw_text( "hits: " .. hitCount, 20, 50, 20, 200, 200, 200, 255 )
end
