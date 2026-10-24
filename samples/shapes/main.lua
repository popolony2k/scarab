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
 shapes - Tiled shape objects (rectangle, ellipse, polyline, polygon),
 rotated objects and point objects (master plan item V6), on three maps made
 for them (resources/tilemap/shapes/, from sunlight's own samples/shapes - see
 resources/tilemap/shapes/README.md). Keys 1, 2 and 3 switch maps:

   1. shapes.tmx   - the four shapes, and how the viewport clips them;
   2. rotation.tmx - shapes, tile objects and text objects with a Tiled
                     rotation, each drawn turned clockwise about its stored
                     point (the small square marked "pivot"). Before sunlight's
                     master plan the rotation was ignored;
   3. thick.tmx    - outlines with a line_width property and point objects
                     with a point_size; both scale with the zoom.

 Switching loads another map over the one already loaded (tilemap_load_map
 again), which sunlight no longer leaks (master plan item V10). The maps'
 "Sans" text objects get the project's own Caravellius 8x8, loaded ONCE and
 registered; the same handle draws this sample's HUD. The registration is
 the renderer's, not the map's, so it survives every switch.
]]

-- The layout sunlight's sample uses: a 1000 x 800 viewport at (40, 40),
-- zoom 1.0, the camera at the map's origin.
local VIEWPORT     = { x = 40, y = 40, w = 1000, h = 800 }
local START_CAMERA = { x = 0, y = 0 }
local SCROLL_STEP  = 4    -- map pixels per frame while an arrow key is held

local MAP_DIR = "resources/tilemap/shapes/"
local MAPS = {
    { key = KEY_ONE,   file = "shapes.tmx",   title = "1 shapes: the four shapes, clipped by the viewport" },
    { key = KEY_TWO,   file = "rotation.tmx", title = "2 rotation: each object turned about its pivot" },
    { key = KEY_THREE, file = "thick.tmx",    title = "3 thick: line widths and points, scaled by zoom" },
}

local renderer, err = renderer_create{
    title          = "Scarab - shapes sample",
    viewport       = VIEWPORT,
    zoom           = 1.0,
    preferred_zoom = 1.0,
    scroll_step    = { w = SCROLL_STEP, h = SCROLL_STEP },
}
assert( renderer, err )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

-- One font, one load: every map's "Sans" text objects and the HUD below.
local sans = assert( load_font( "resources/fonts/caravellius8x8.fnt" ) )
assert( register_font( "Sans", sans ) )

local current

local function show_map( entry )
    assert( tilemap_load_map( MAP_DIR .. entry.file, MAP_ALIGNMENT_TOP_LEFT ) )
    zoom_reset()
    camera_set_position( START_CAMERA.x, START_CAMERA.y )
    current = entry
end

show_map( MAPS[1] )

local HUD_X         = 40
local HUD_Y         = 846
local HUD_LINE      = 24    -- pixels between HUD lines
local HUD_SIZE      = 16
local PLATE_PADDING = 4     -- pixels around a line of text on its plate
local PLATE_ALPHA   = 170   -- the plate's opacity, 0..255

-- A line of HUD text in "Sans" on a dark translucent plate.
local function draw_hud_line( text, x, y, r, g, b )
    draw_filled_rectangle( x - PLATE_PADDING, y - PLATE_PADDING,
        measure_text( text, HUD_SIZE, sans ) + 2 * PLATE_PADDING, HUD_SIZE + 2 * PLATE_PADDING, 0, 0, 0, PLATE_ALPHA )
    draw_text( text, x, y, HUD_SIZE, r, g, b, 255, sans )
end

function on_update( dt )

    for _, entry in ipairs( MAPS ) do
        if input_is_key_released( entry.key ) and entry ~= current then show_map( entry ) end
    end

    -- Same mapping as samples/camera: the arrow moves the map's content that way.
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then camera_move_up() end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then camera_move_down() end
    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then camera_move_left() end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then camera_move_right() end

    -- The zoom keys every sunlight/Scarab sample shares (see samples/camera):
    -- Page Up/Page Down or + / - zoom, = resets the zoom, Home resets the zoom
    -- and the camera. + and = are one physical key, told apart by Shift.
    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    if input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or
       ( shift and input_is_key_down( KEY_EQUAL ) ) then
        zoom_in()
    end

    if input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then
        zoom_out()
    end

    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) then
        zoom_reset()
    end

    if input_is_key_released( KEY_HOME ) then
        zoom_reset()
        camera_set_position( START_CAMERA.x, START_CAMERA.y )
    end

    local camX, camY = camera_get_position()

    draw_hud_line( current.title, HUD_X, HUD_Y, 255, 255, 255 )
    draw_hud_line( string.format( "camera %d, %d   zoom %.2f   keys 1, 2, 3 switch maps", camX, camY, viewport_get_zoom_factor() ),
        HUD_X, HUD_Y + HUD_LINE, 200, 200, 200 )
    draw_hud_line( "arrows/WASD pan   PageUp/PageDown or +/- zoom   = reset zoom   Home reset", HUD_X, HUD_Y + 2 * HUD_LINE, 200, 200, 200 )
end
