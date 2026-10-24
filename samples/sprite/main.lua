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
 sprite - acquiring/configuring/moving a sprite (docs/lua-api/sprite.html),
 on top of the same map "tilemap" loads (resources/tilemap/).

 Demonstrates pool_register_type/sprite_acquire (the SpritePool CRUD
 pattern), sprite_configure_texture (a 4-frame auto-animating strip),
 sprite_add_to_layer (required before a sprite draws at all - see the
 layer-visibility gotcha in root CLAUDE.md), and keyboard-driven
 sprite_set_pos/sprite_get_pos/sprite_get_size.

 IMPORTANT - see root CLAUDE.md's own gotcha: acquiring/positioning/
 adding a sprite to a layer from top-level (load-time) code fails
 silently if that code runs BEFORE the map is loaded. This script is
 safe specifically because tilemap_load_map is called synchronously,
 first, and every sprite call below runs strictly after it in the same
 top-level script - not because top-level sprite setup is safe in
 general.
]]

app_set_name( "Scarab - sprite sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER )

-- "sunny" is sunlight's own sample character (resources/sprites/, shared
-- from sunlight/samples/sprite/ - see resources/sprites/README.md), a
-- 128x32 strip of four 32x32 frames.
pool_register_type( "sunny", 1 )

local handle = sprite_acquire( "sunny" )

-- The last argument is the frame delay in ms: WITHOUT it (the default, -1) the frame is held and sunny never animates.
sprite_configure_texture( handle, 0, "resources/sprites/sunny_idle_down.png", 4, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 100 )
sprite_set_active_sequence( handle, 0 )
-- World space: sunny's position is a MAP position in pixels, so the camera
-- can follow it across the whole map (by default a sprite's position is
-- relative to the view, and it would stay put on screen while the map moves).
sprite_set_world_space( handle, true )
sprite_set_pos( handle, 100, 100 )

-- Layer 4 ("clouds" in test.tmx) is visible by default (no visible="0")
-- - a sprite added to a layer authored invisible would never draw at
-- all, regardless of sprite_set_visible (see root CLAUDE.md).
sprite_add_to_layer( handle, 4 )

local __SPEED = 3  -- pixels/frame, fine for a short demo - see CLAUDE.md's dt gotcha for why real games shouldn't scale movement by dt's fake fixed value either

-- Dark text: the map behind the text is mostly white clouds and light sky,
-- so white/light-grey text can't be read over it.
local TITLE_COLOR = { r = 20, g = 20, b = 20, a = 255 }
local INFO_COLOR  = { r = 50, g = 50, b = 70, a = 255 }

local mapWidth, mapHeight, tileWidth, tileHeight = tilemap_get_map_info()
local mapPixelWidth  = mapWidth * tileWidth
local mapPixelHeight = mapHeight * tileHeight

local function clamp( value, low, high )
    return math.max( low, math.min( value, high ) )
end

-- The camera follows sunny: it keeps sunny centred in the view, clamped so it
-- never shows anything past the map's edges (camera_set_position itself does
-- not clamp). The camera position is the map point shown at the view's
-- top-left; the view shows viewport size / zoom map pixels.
local function follow( x, y, width, height )
    local _, _, viewportWidth, viewportHeight = viewport_get_dimension()
    local zoom          = viewport_get_zoom_factor()
    local visibleWidth  = viewportWidth / zoom
    local visibleHeight = viewportHeight / zoom
    local cameraX = clamp( math.floor( x + width / 2 - visibleWidth / 2 ), 0, math.max( 0, math.floor( mapPixelWidth - visibleWidth ) ) )
    local cameraY = clamp( math.floor( y + height / 2 - visibleHeight / 2 ), 0, math.max( 0, math.floor( mapPixelHeight - visibleHeight ) ) )

    camera_set_position( cameraX, cameraY )

    return cameraX, cameraY
end

function on_update( dt )

    local x, y = sprite_get_pos( handle )

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

    local width, height = sprite_get_size( handle )

    -- sunny stays on the map
    x = clamp( x, 0, mapPixelWidth - width )
    y = clamp( y, 0, mapPixelHeight - height )

    sprite_set_pos( handle, x, y )

    local cameraX, cameraY = follow( x, y, width, height )

    draw_text( "sprite sample - arrows/WASD to move, the camera follows", 20, 20, 20, TITLE_COLOR.r, TITLE_COLOR.g, TITLE_COLOR.b, TITLE_COLOR.a )
    draw_text( "sprite_get_pos: " .. x .. ", " .. y .. "   sprite_get_size: " .. width .. "x" .. height,
        20, 50, 18, INFO_COLOR.r, INFO_COLOR.g, INFO_COLOR.b, INFO_COLOR.a )
    draw_text( "camera_get_position: " .. cameraX .. ", " .. cameraY,
        20, 72, 18, INFO_COLOR.r, INFO_COLOR.g, INFO_COLOR.b, INFO_COLOR.a )
end
