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
 views - several views of the same world at once (docs/lua-api/views.html):
 the main picture, a MINIMAP of the whole map, and a CLOSE-UP that follows a
 sprite - all drawn every frame from one map and one sprite.

 Demonstrates renderer_create + view_create/view_destroy, view_fit_to_map,
 view_set_zoom, view_set_camera_position, view_set_visible, layer masks
 (view_show_layer - the minimap hides the "clouds" layer, the main view does
 not), view_set_background_color/view_set_clear_background, and
 sprite_set_world_space: a sprite's position is normally relative to the view
 drawing it (measured from the view's corner, ignoring the view's camera), so
 the SAME position lands somewhere different in a view whose camera is not at
 the map origin; with world space on it is a MAP position and every view draws
 it in the right place. Press Space to switch it off: the main view and the
 minimap look the same either way (their camera is at the map origin, so only
 zoom differs and both readings agree), but the close-up - whose camera follows
 the sprite - loses it, because the raw position lands outside its rectangle.
]]

-- sp_* command: see samples/hello-world/docs/README.md for why every entry
-- script needs at least one, even one with nothing to sequence.
sp_wait( 1 )

local __RENDER_WIDTH   = 960
local __RENDER_HEIGHT  = 640
local __MINIMAP        = { x = 700, y = 20,  w = 240, h = 240 }
local __CLOSEUP        = { x = 20,  y = 380, w = 240, h = 240 }
local __CLOSEUP_ZOOM   = 2.0
local __CLOUDS_LAYER   = 4     -- "clouds" in resources/tilemap/test.tmx
local __SPRITE_LAYER   = 5     -- "houses": a layer the minimap keeps
local __SPEED          = 3     -- map pixels per frame

-- FIRST window-needing call (see samples/renderer for why it must be first).
local renderer, err = renderer_create{
    title    = "Scarab - views sample",
    width    = __RENDER_WIDTH,
    height   = __RENDER_HEIGHT,
    draw_fps = true,
    viewport = { x = 10, y = 10, w = __RENDER_WIDTH - 20, h = __RENDER_HEIGHT - 20 },
    zoom     = 1.0,
}

if not renderer then
    error( "cannot create the renderer: " .. err )
end

tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT )

local mapTilesW, mapTilesH, tileW, tileH = tilemap_get_map_info()
local mapWidth  = mapTilesW * tileW
local mapHeight = mapTilesH * tileH

-- Extra views: plain view_create, then configure. Ids are opaque integers (0 = the default view).
local minimap = assert( view_create( renderer, __MINIMAP.x, __MINIMAP.y, __MINIMAP.w, __MINIMAP.h ) )
assert( view_fit_to_map( minimap ) )                        -- whole map, camera at its top-left
view_set_background_color( minimap, 0, 0, 0, 200 )          -- translucent black backdrop
view_show_layer( minimap, "clouds", false )                 -- this view only: the main view still shows the clouds

local closeup = assert( view_create( renderer, __CLOSEUP.x, __CLOSEUP.y, __CLOSEUP.w, __CLOSEUP.h ) )
assert( view_set_zoom( closeup, __CLOSEUP_ZOOM ) )

pool_register_type( "sunny", 1 )

local sunny = sprite_acquire( "sunny" )

sprite_configure_texture( sunny, 0, "resources/sprites/sunny_idle_down.png", 4, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR )
sprite_set_active_sequence( sunny, 0 )
sprite_add_to_layer( sunny, __SPRITE_LAYER )

local worldSpace = true
sprite_set_world_space( sunny, worldSpace )

local spriteWidth, spriteHeight = sprite_get_size( sunny )
local x, y = mapWidth // 2, mapHeight // 2
sprite_set_pos( sunny, x, y )

local function clamp( value, low, high )
    return math.max( low, math.min( high, value ) )
end

function on_update( dt )

    if input_is_key_down( KEY_LEFT )  or input_is_key_down( KEY_A ) then x = x - __SPEED end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then x = x + __SPEED end
    if input_is_key_down( KEY_UP )    or input_is_key_down( KEY_W ) then y = y - __SPEED end
    if input_is_key_down( KEY_DOWN )  or input_is_key_down( KEY_S ) then y = y + __SPEED end

    x = clamp( x, 0, mapWidth - spriteWidth )
    y = clamp( y, 0, mapHeight - spriteHeight )
    sprite_set_pos( sunny, x, y )

    -- The close-up follows the sprite: put the sprite in the middle of the view. The camera is not
    -- re-clamped for us, so keep it inside the map here.
    local viewMapWidth  = __CLOSEUP.w / __CLOSEUP_ZOOM
    local viewMapHeight = __CLOSEUP.h / __CLOSEUP_ZOOM
    view_set_camera_position( closeup,
        math.floor( clamp( x + spriteWidth / 2 - viewMapWidth / 2, 0, mapWidth - viewMapWidth ) ),
        math.floor( clamp( y + spriteHeight / 2 - viewMapHeight / 2, 0, mapHeight - viewMapHeight ) ) )

    if input_is_key_released( KEY_M ) then
        view_set_visible( minimap, not view_get_visible( minimap ) )
    end

    if input_is_key_released( KEY_C ) then
        view_set_visible( closeup, not view_get_visible( closeup ) )
    end

    if input_is_key_released( KEY_L ) then
        view_show_layer( minimap, __CLOUDS_LAYER, not view_is_layer_shown( minimap, __CLOUDS_LAYER ) )
    end

    if input_is_key_released( KEY_SPACE ) then
        worldSpace = not worldSpace
        sprite_set_world_space( sunny, worldSpace )
    end

    draw_text( "views sample - arrows/WASD move the sprite", 20, 20, 20, 255, 255, 255, 255 )
    draw_text( string.format( "map position %d, %d   world space: %s   views: %d",
        x, y, tostring( sprite_get_world_space( sunny ) ), renderer_get_view_count( renderer ) ), 20, 46, 18, 200, 200, 200, 255 )
    draw_text( "M minimap   C close-up   L minimap clouds   Space world space (close-up loses sunny when off)   Esc quit", 20, __RENDER_HEIGHT - 30, 16, 180, 180, 180, 255 )
end
