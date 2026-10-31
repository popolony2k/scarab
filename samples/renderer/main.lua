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

local Hud = dofile( "samples/shared/hud.lua" )

local __MIN_ZOOM     <const> = 0.5      -- the view's zoom limits, inclusive factors
local __MAX_ZOOM     <const> = 4.0
local __INVALID_ZOOM <const> = 3.8      -- not a whole multiple of ZOOM_STEP (0.0625)

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
assert( view_set_zoom_limits( view, __MIN_ZOOM, __MAX_ZOOM ) )

-- Now the window exists, so window-needing calls are fine.
assert( tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER ) )

local mapWidth, mapHeight, tileWidth, tileHeight = tilemap_get_map_info()

-- Rejected calls come back as nil + a message - shown on screen, to see the exact text.
local lastMessage = "press Space to try an invalid zoom (" .. __INVALID_ZOOM .. ")"

function on_update( dt )

    -- Held keys give continuous zoom/pan: each call is one fixed step. The zoom keys are
    -- the ones every sunlight/Scarab sample shares (see samples/camera): Page Up/Page Down
    -- and + / - zoom, on the main keyboard and the keypad; + and = are one physical key
    -- (+ is Shift and =), told apart by whether Shift is held.
    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    if input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or
       ( shift and input_is_key_down( KEY_EQUAL ) ) then
        view_zoom_in( view )
    end

    if input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then
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

    -- = resets the zoom only (to the preferred zoom, 1.0 here - the zoom this sample
    -- starts at); Home resets the zoom and the camera.
    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) then
        view_zoom_reset( view )
    end

    if input_is_key_released( KEY_HOME ) then
        view_zoom_reset( view )
        view_reset_camera( view )
    end

    if input_is_key_released( KEY_SPACE ) then
        -- Not a whole multiple of ZOOM_STEP: rejected, never rounded.
        local ok, message = view_set_zoom( view, __INVALID_ZOOM )
        lastMessage = ok and "zoom set" or message
    end

    -- Everything below is READ BACK from the renderer, not remembered from creation.
    local config       = renderer_get_config( renderer )
    local minZoom, maxZoom = view_get_zoom_limits( view )
    local camX, camY   = view_get_camera_position( view )
    local x, y, w, h   = view_get_dimension( view )
    local stepW, stepH = view_get_scroll_step( view )

    Hud.line( string.format( "renderer_get_config: %dx%d  fps %d", config.width, config.height, config.fps ), 1, Hud.TITLE )
    Hud.line( string.format( "title '%s'", config.title ), 2, Hud.TITLE )
    Hud.line( string.format( "view %d   zoom %.4f", view, view_get_zoom( view ) ), 3, Hud.INFO )
    Hud.line( string.format( "zoom limits %.4f .. %.4f", minZoom, maxZoom ), 4, Hud.INFO )
    Hud.line( string.format( "ZOOM_STEP %.4f   viewport %d,%d %dx%d", ZOOM_STEP, x, y, w, h ), 5, Hud.INFO )
    Hud.line( string.format( "camera %d,%d   scroll step %d,%d", camX, camY, stepW, stepH ), 6, Hud.INFO )
    Hud.line( "map " .. tostring( mapWidth ) .. "x" .. tostring( mapHeight ) .. " tiles   backend " .. tostring( renderer_get_backend( renderer ) ), 7, Hud.INFO )
    Hud.lines( lastMessage, 8, Hud.ALERT )
    Hud.line_from_bottom( "PageUp/PageDown or +/- zoom  = reset zoom  Home reset", 2, Hud.DIM )
    Hud.line_from_bottom( "arrows pan   Space invalid zoom   Esc quit", 1, Hud.DIM )
end
