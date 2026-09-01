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
 camera - scrolling/zooming the camera (docs/lua-api/camera.html), over
 the same map "tilemap" loads (resources/tilemap/ - see it's own
 README.md).

 Demonstrates camera_move_*/zoom_in/zoom_out/camera_reset (incremental,
 keyboard-driven), camera_set_position (an absolute jump, on a key
 press), and reading current state back every frame via
 camera_get_position/viewport_get_dimension/viewport_get_zoom_factor.
]]

app_set_name( "Scarab - camera sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER )

function on_update( dt )

    -- Panning/zoom: each camera_move_*/zoom_in/zoom_out call is one fixed
    -- step (configured on the C++ renderer, not from Lua) - calling it
    -- every frame the key is held gives continuous scrolling/zooming at
    -- the engine's own step rate, the same pattern Caravellius's own
    -- camera.lua uses for it's auto-scroll.
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then
        camera_move_up()
    end

    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then
        camera_move_down()
    end

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then
        camera_move_left()
    end

    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then
        camera_move_right()
    end

    if input_is_key_down( KEY_PAGE_UP ) then
        zoom_in()
    end

    if input_is_key_down( KEY_PAGE_DOWN ) then
        zoom_out()
    end

    if input_is_key_released( KEY_HOME ) then
        camera_reset()
        zoom_reset()
    end

    -- An absolute jump, unlike camera_move_*'s incremental nudges -
    -- camera_set_position does NOT clamp to map boundaries, so this
    -- picks a coordinate safely inside the map (it's own size is read
    -- back via tilemap_get_map_info, not hardcoded).
    if input_is_key_released( KEY_SPACE ) then
        local mapWidth, mapHeight, tileWidth, tileHeight = tilemap_get_map_info()

        camera_set_position( math.floor( mapWidth * tileWidth / 4 ), math.floor( mapHeight * tileHeight / 4 ) )
    end

    local camX, camY = camera_get_position()
    local vpX, vpY, vpW, vpH = viewport_get_dimension()
    local zoom = viewport_get_zoom_factor()

    draw_text( "camera sample - arrows/WASD pan, PageUp/PageDown zoom, Home reset, Space jump", 20, 20, 20, 255, 255, 255, 255 )
    draw_text( "camera_get_position: " .. camX .. ", " .. camY, 20, 50, 18, 200, 200, 200, 255 )
    draw_text( "viewport_get_dimension: " .. vpX .. ", " .. vpY .. " " .. vpW .. "x" .. vpH, 20, 72, 18, 200, 200, 200, 255 )
    draw_text( "viewport_get_zoom_factor: " .. string.format( "%.2f", zoom ), 20, 94, 18, 200, 200, 200, 255 )
end
