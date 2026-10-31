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
 objects - Tiled tile and text objects (master plan item V5), on a map made
 for them (resources/tilemap/objects/objects.tmx, from sunlight's own
 samples/objects - see resources/tilemap/objects/README.md).

 Since sunlight's master plan, tile objects and text objects in a map are
 drawn; before, they were invisible. One row of the map is one tile-object
 case, each labelled by a TEXT object:
   1. at its own size (32 px)          4. animated (cycles 4 frames)
   2. stretched to its 64 px box       5. flip bits ignored (drawn unflipped)
   3. cut by the left viewport edge    6. cut by the bottom viewport edge
 The green rectangle (a rectangle object) is the viewport's frame at the
 starting camera: anything outside it must not be visible.

 Text objects draw only with a font registered for their family - else
 "??????" - so the map's "Sans" labels get the project's own Caravellius
 8x8, loaded ONCE with load_font and registered with register_font. The
 same handle draws this sample's own HUD (draw_text's font argument).
]]

-- The layout sunlight's sample uses (the map's frame object matches it):
-- a 1000 x 800 viewport at (40, 40), zoom 1.0, the camera at (100, 100).
local __VIEWPORT     <const> = { x = 40, y = 40, w = 1000, h = 800 }
local __START_CAMERA <const> = { x = 100, y = 100 }
local __SCROLL_STEP  <const> = 4    -- map pixels per frame while an arrow key is held

local renderer, err = renderer_create{
    title          = "Scarab - objects sample",
    viewport       = __VIEWPORT,
    zoom           = 1.0,
    preferred_zoom = 1.0,
    scroll_step    = { w = __SCROLL_STEP, h = __SCROLL_STEP },
}
assert( renderer, err )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

assert( tilemap_load_map( "resources/tilemap/objects/objects.tmx", MAP_ALIGNMENT_TOP_LEFT ) )

-- One font, one load: the map's "Sans" text objects use the HUD's own font.
local Hud = dofile( "samples/shared/hud.lua" )

assert( register_font( "Sans", Hud.font() ) )

assert( camera_set_position( __START_CAMERA.x, __START_CAMERA.y ) )

function on_update( dt )

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
        camera_set_position( __START_CAMERA.x, __START_CAMERA.y )
    end

    local camX, camY = camera_get_position()

    Hud.line_from_bottom( string.format( "objects sample - camera %d, %d   zoom %.2f", camX, camY, viewport_get_zoom_factor() ), 2, Hud.TITLE )
    Hud.line_from_bottom( "arrows/WASD pan   PageUp/PageDown or +/- zoom   = reset zoom   Home reset", 1, Hud.DIM )
end
