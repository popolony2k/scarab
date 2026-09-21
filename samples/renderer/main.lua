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
 renderer - choosing your own window/render settings with renderer_create
 (docs/lua-api/renderer.html), then driving the renderer's default view with
 the view_* functions (docs/lua-api/views.html), over the same map "tilemap"
 loads (resources/tilemap/ - see it's own README.md).

 Demonstrates renderer_create (a strictly validated options table),
 renderer_get_config (the CURRENT effective settings, read back and drawn
 live), renderer_get_default_view, and view_set_zoom_limits/view_zoom_in/
 view_zoom_out/view_set_zoom/view_move_camera_*/view_set_camera_position.

 The ordering rule that matters: renderer_create has to be the FIRST call
 that needs a window. Anything earlier (app_set_name, set_font,
 tilemap_load_map, ...) would create the default renderer first, and
 renderer_create would then fail - naming the call that did it.
]]

-- sp_* command: see samples/hello-world/docs/README.md for why every entry
-- script needs at least one, even one with nothing to sequence.
sp_wait( 1 )

-- FIRST window-needing call. Anything you leave out keeps Scarab's default.
local renderer, err = renderer_create{
    title      = "Scarab - renderer sample",
    width      = 960,
    height     = 640,
    fps        = 60,
    resizeable = true,
    draw_fps   = true,
    viewport   = { x = 10, y = 10, w = 940, h = 620 },   -- must fit the render area
    zoom       = 1.0,                                    -- a FACTOR, a multiple of ZOOM_STEP
}

-- renderer_create never raises a Lua error: it returns nil and the reason.
if not renderer then
    error( "cannot create the renderer: " .. err )
end

local view = renderer_get_default_view( renderer )

-- Zoom limits are factors too, and inclusive.
view_set_zoom_limits( view, 0.5, 4.0 )

-- Now the window exists, so window-needing calls are fine.
tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER )

local mapWidth, mapHeight, tileWidth, tileHeight = tilemap_get_map_info()

-- Rejected calls come back as nil + a message - shown on screen, to see the exact text.
local lastMessage = "press Space to try an invalid zoom (3.8)"

function on_update( dt )

    -- Held keys give continuous zoom/pan: each call is one fixed step.
    if input_is_key_down( KEY_Z ) then
        view_zoom_in( view )
    end

    if input_is_key_down( KEY_X ) then
        view_zoom_out( view )
    end

    if input_is_key_down( KEY_LEFT ) then
        view_move_camera_left( view )
    end

    if input_is_key_down( KEY_RIGHT ) then
        view_move_camera_right( view )
    end

    if input_is_key_down( KEY_UP ) then
        view_move_camera_up( view )
    end

    if input_is_key_down( KEY_DOWN ) then
        view_move_camera_down( view )
    end

    if input_is_key_released( KEY_R ) then
        view_zoom_reset( view )
        view_reset_camera( view )
    end

    if input_is_key_released( KEY_SPACE ) then
        -- 3.8 is not a whole multiple of ZOOM_STEP (0.0625): rejected, never rounded.
        local ok, message = view_set_zoom( view, 3.8 )
        lastMessage = ok and "zoom set" or message
    end

    -- Everything below is READ BACK from the renderer, not remembered from creation.
    local config       = renderer_get_config( renderer )
    local minZoom, maxZoom = view_get_zoom_limits( view )
    local camX, camY   = view_get_camera_position( view )
    local x, y, w, h   = view_get_dimension( view )
    local stepW, stepH = view_get_scroll_step( view )

    draw_text( string.format( "renderer_get_config: %dx%d  fps %d  title '%s'", config.width, config.height, config.fps, config.title ), 20, 20, 18, 255, 255, 255, 255 )
    draw_text( string.format( "view %d   zoom %.4f   limits %.4f .. %.4f   ZOOM_STEP %.4f", view, view_get_zoom( view ), minZoom, maxZoom, ZOOM_STEP ), 20, 44, 18, 255, 255, 255, 255 )
    draw_text( string.format( "viewport %d,%d %dx%d   camera %d,%d   scroll step %d,%d", x, y, w, h, camX, camY, stepW, stepH ), 20, 68, 18, 255, 255, 255, 255 )
    draw_text( "map " .. tostring( mapWidth ) .. "x" .. tostring( mapHeight ) .. " tiles   backend " .. tostring( renderer_get_backend( renderer ) ), 20, 92, 18, 200, 200, 200, 255 )
    draw_text( lastMessage, 20, 116, 18, 255, 200, 120, 255 )
    draw_text( "Z / X zoom   arrows pan   R reset   Space invalid zoom   Esc quit", 20, config.height - 32, 16, 180, 180, 180, 255 )
end
