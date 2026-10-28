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
 mapless - sprites drawn with NO map loaded (master plan item V8). Before
 sunlight's master plan, sprite_add_to_layer with no map loaded accepted a
 sprite (and updated its collider) but never drew it; now it is drawn, so a
 title screen, a menu or any game without a Tiled map can use sprites.

 With no map, a layer id is only a grouping key: the layers are drawn in
 ascending id order, so the shadow (layer 1) is under Sunny (layer 2), and
 the glow (layer 3) is over him. Keys 1, 2 and 3 hide or show each layer
 through the view's layer mask (view_show_layer), which hides mapless layers
 the same way it hides a map's.

 There is no map to scroll over, so the camera keys do nothing here: a
 sprite's position is relative to the view (scaled by its zoom), and the
 arrows move Sunny instead.
]]

local ZOOM        = 2.0
local SPEED       = 2       -- view pixels per frame while an arrow key is held
local DELAY_MILLI = 100     -- Sunny's frame delay; without one the frame is held
local VIEW        = 0       -- the renderer's default view

local LAYERS = {
    { key = KEY_ONE,   id = 1, name = "shadow" },
    { key = KEY_TWO,   id = 2, name = "Sunny" },
    { key = KEY_THREE, id = 3, name = "glow" },
}

local SHADOW_POS = { x = 120, y = 150 }
local SUNNY_POS  = { x = 60,  y = 120 }
local GLOW_POS   = { x = 280, y = 90 }

local renderer, err = renderer_create{
    title          = "Scarab - mapless sample",
    zoom           = ZOOM,
    preferred_zoom = ZOOM,
}
assert( renderer, err )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local sans = assert( load_font( "resources/fonts/caravellius8x8.fnt" ) )

-- One sprite per layer, added to its layer with no map loaded at all.
local function add_sprite( tag, path, frames, delay, pos, layerId )
    assert( pool_register_type( tag, 1 ) )

    local handle = assert( sprite_acquire( tag ) )

    assert( sprite_configure_texture( handle, 0, path, frames, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, delay ) )
    assert( sprite_set_active_sequence( handle, 0 ) )
    assert( sprite_set_pos( handle, pos.x, pos.y ) )
    assert( sprite_add_to_layer( handle, layerId ) )

    return handle
end

add_sprite( "shadow", "resources/sprites/shadow.png", 1, -1, SHADOW_POS, 1 )
local sunny = add_sprite( "sunny", "resources/sprites/sunny_idle_down.png", 4, DELAY_MILLI, SUNNY_POS, 2 )
add_sprite( "glow", "resources/sprites/glow.png", 1, -1, GLOW_POS, 3 )

local sunnyX, sunnyY = SUNNY_POS.x, SUNNY_POS.y

local HUD_X         = 20
local HUD_Y         = 812
local HUD_LINE      = 24    -- pixels between HUD lines
local HUD_SIZE      = 16
local PLATE_PADDING = 4     -- pixels around a line of text on its plate
local PLATE_ALPHA   = 170   -- the plate's opacity, 0..255

-- A line of HUD text in Caravellius 8x8 on a dark translucent plate.
local function draw_hud_line( text, x, y, r, g, b )
    draw_filled_rectangle( x - PLATE_PADDING, y - PLATE_PADDING,
        measure_text( text, HUD_SIZE, sans ) + 2 * PLATE_PADDING, HUD_SIZE + 2 * PLATE_PADDING, 0, 0, 0, PLATE_ALPHA )
    draw_text( text, x, y, HUD_SIZE, r, g, b, 255, sans )
end

function on_update( dt )

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then sunnyX = sunnyX - SPEED end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then sunnyX = sunnyX + SPEED end
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then sunnyY = sunnyY - SPEED end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then sunnyY = sunnyY + SPEED end

    sprite_set_pos( sunny, sunnyX, sunnyY )

    for _, layer in ipairs( LAYERS ) do
        if input_is_key_released( layer.key ) then
            view_show_layer( VIEW, layer.id, not view_is_layer_shown( VIEW, layer.id ) )
        end
    end

    -- The zoom keys every sunlight/Scarab sample shares (see samples/camera):
    -- Page Up/Page Down or + / - zoom, = resets the zoom, Home resets the zoom
    -- and Sunny's position. + and = are one physical key, told apart by Shift.
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
        sunnyX, sunnyY = SUNNY_POS.x, SUNNY_POS.y
    end

    local shown = {}

    for _, layer in ipairs( LAYERS ) do
        shown[#shown + 1] = layer.id .. " " .. layer.name .. ( view_is_layer_shown( VIEW, layer.id ) and " on" or " off" )
    end

    draw_hud_line( "no map loaded: layers draw in id order, 1 under 2 under 3", HUD_X, HUD_Y, 255, 255, 255 )
    draw_hud_line( table.concat( shown, "   " ) .. string.format( "   zoom %.2f", viewport_get_zoom_factor() ),
        HUD_X, HUD_Y + HUD_LINE, 200, 200, 200 )
    draw_hud_line( "arrows/WASD move Sunny   1/2/3 hide/show a layer", HUD_X, HUD_Y + 2 * HUD_LINE, 200, 200, 200 )
    draw_hud_line( "PageUp/PageDown or +/- zoom   = reset zoom   Home reset", HUD_X, HUD_Y + 3 * HUD_LINE, 200, 200, 200 )
end
