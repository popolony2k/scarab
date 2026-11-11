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
 backdrop - a screen-space image layer (docs/lua-api/tilemap.html, "Tiled custom properties"):
 a full-window picture that stays whole and fixed on the screen while the map under it scrolls
 and zooms - a title or cover screen, the way Caravellius draws its cover.

 resources/tilemap/screenspace/screenspace.tmx has four image layers. Layer 1, "backdrop", carries
 the Tiled custom property screen_space = true (bool): it is drawn at its offset in window pixels,
 at its own size, whatever the camera or the zoom. The three houses (layers 2-4) are ordinary image
 layers, drawn in map space: they move and scale with the camera.

 Kepi stands on layer 1, like Caravellius' ship on its cover: sprites on a screen-space layer draw
 over it, and the houses on the later layers draw over Kepi.
]]

local Hud = dofile( "samples/shared/hud.lua" )

local __MAP           <const> = "resources/tilemap/screenspace/screenspace.tmx"
local __KEPI          <const> = "resources/sprites/kepi_idle.png"   -- Kepi, Scarab's mascot (temporary art)
local __KEPI_FRAMES   <const> = 4
local __KEPI_DELAY    <const> = 100     -- ms per frame; without one the frame is held
local __BACKDROP      <const> = 1       -- the screen-space image layer, Kepi's layer too
local __STEP          <const> = 2       -- pixels Kepi walks per frame while a key is held
local __START_X       <const> = 150     -- where Kepi starts, relative to the view (scaled by the zoom)
local __START_Y       <const> = 100
local __VIEW          <const> = 0       -- the default view

assert( app_set_name( "Scarab - backdrop sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

assert( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) )

-- zoom_reset returns to the PREFERRED zoom (see samples/camera): make it the starting one.
assert( view_set_preferred_zoom( __VIEW, viewport_get_zoom_factor() ) )

-- Kepi, a screen-space sprite on the backdrop layer.
assert( pool_register_type( "kepi", 1 ) )

local kepi = assert( sprite_acquire( "kepi" ) )

assert( sprite_configure_texture( kepi, 0, __KEPI, __KEPI_FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, __KEPI_DELAY ) )
assert( sprite_set_active_sequence( kepi, 0 ) )
assert( sprite_add_to_layer( kepi, __BACKDROP ) )
assert( sprite_set_visible( kepi, true ) )
assert( sprite_set_pos( kepi, __START_X, __START_Y ) )

-- Move Kepi by (dx, dy) pixels.
local function walk( dx, dy )
    local x, y = assert( sprite_get_pos( kepi ) )

    assert( sprite_set_pos( kepi, x + dx, y + dy ) )
end

-- Called by Scarab every frame (dt is a fixed step, not measured time): pan the camera (arrows), walk Kepi
-- (WASD), zoom (PageUp/PageDown, + and -, = or Home resets), and draw the HUD.
function on_update( dt )

    -- the camera: the houses move, the backdrop does not
    if input_is_key_down( KEY_UP ) then camera_move_up() end          -- the same directions as samples/camera
    if input_is_key_down( KEY_DOWN ) then camera_move_down() end
    if input_is_key_down( KEY_LEFT ) then camera_move_left() end
    if input_is_key_down( KEY_RIGHT ) then camera_move_right() end

    -- Kepi
    if input_is_key_down( KEY_W ) then walk( 0, -__STEP ) end
    if input_is_key_down( KEY_S ) then walk( 0, __STEP ) end
    if input_is_key_down( KEY_A ) then walk( -__STEP, 0 ) end
    if input_is_key_down( KEY_D ) then walk( __STEP, 0 ) end

    -- zoom, as in samples/camera: the houses scale, the backdrop does not
    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    if input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or ( shift and input_is_key_down( KEY_EQUAL ) ) then zoom_in() end
    if input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then zoom_out() end
    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) or input_is_key_released( KEY_HOME ) then
        zoom_reset()
    end

    local camX, camY = camera_get_position()

    Hud.line( "backdrop sample - a screen-space image layer (screen_space = true)", 1, Hud.TITLE )
    Hud.line( "arrows pan   WASD walk Kepi   PageUp/PageDown zoom   = reset zoom", 2, Hud.TITLE )
    Hud.line( "the gold frame stays whole and fixed; the houses move and scale with the map", 3, Hud.INFO )
    Hud.line( string.format( "camera %d, %d   zoom %.2f", camX, camY, viewport_get_zoom_factor() ), 4, Hud.DIM )
end
