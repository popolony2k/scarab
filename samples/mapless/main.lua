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

local __ZOOM         <const> = 2.0
local __SPEED        <const> = 2       -- view pixels per frame while an arrow key is held
local __DELAY_MILLI  <const> = 100     -- Sunny's frame delay; without one the frame is held
local __VIEW         <const> = 0       -- the renderer's default view
local __HELD_FRAME   <const> = -1      -- a one-frame sprite: never step
local __SUNNY_FRAMES <const> = 4      -- sunny_idle_down.png: four frames side by side
local __SHADOW_LAYER <const> = 1      -- with no map, a layer id is only a grouping key: lower draws first
local __SUNNY_LAYER  <const> = 2
local __GLOW_LAYER   <const> = 3

local __LAYERS <const> = {
    { key = KEY_ONE,   id = __SHADOW_LAYER, name = "shadow" },
    { key = KEY_TWO,   id = __SUNNY_LAYER,  name = "Sunny" },
    { key = KEY_THREE, id = __GLOW_LAYER,   name = "glow" },
}

local __SHADOW_POS <const> = { x = 120, y = 150 }
local __SUNNY_POS  <const> = { x = 60,  y = 120 }
local __GLOW_POS   <const> = { x = 280, y = 90 }

local renderer, err = renderer_create{
    title          = "Scarab - mapless sample",
    zoom           = __ZOOM,
    preferred_zoom = __ZOOM,
}
assert( renderer, err )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local Hud = dofile( "samples/shared/hud.lua" )

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

add_sprite( "shadow", "resources/sprites/shadow.png", 1, __HELD_FRAME, __SHADOW_POS, __SHADOW_LAYER )
local sunny = add_sprite( "sunny", "resources/sprites/sunny_idle_down.png", __SUNNY_FRAMES, __DELAY_MILLI, __SUNNY_POS, __SUNNY_LAYER )
add_sprite( "glow", "resources/sprites/glow.png", 1, __HELD_FRAME, __GLOW_POS, __GLOW_LAYER )

local sunnyX, sunnyY = __SUNNY_POS.x, __SUNNY_POS.y


function on_update( dt )

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then sunnyX = sunnyX - __SPEED end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then sunnyX = sunnyX + __SPEED end
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then sunnyY = sunnyY - __SPEED end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then sunnyY = sunnyY + __SPEED end

    sprite_set_pos( sunny, sunnyX, sunnyY )

    for _, layer in ipairs( __LAYERS ) do
        if input_is_key_released( layer.key ) then
            view_show_layer( __VIEW, layer.id, not view_is_layer_shown( __VIEW, layer.id ) )
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
        sunnyX, sunnyY = __SUNNY_POS.x, __SUNNY_POS.y
    end

    local shown = {}

    for _, layer in ipairs( __LAYERS ) do
        shown[#shown + 1] = layer.id .. " " .. layer.name .. ( view_is_layer_shown( __VIEW, layer.id ) and " on" or " off" )
    end

    Hud.line_from_bottom( "no map loaded: layers draw in id order, 1 under 2 under 3", 4, Hud.TITLE )
    Hud.line_from_bottom( table.concat( shown, "   " ) .. string.format( "   zoom %.2f", viewport_get_zoom_factor() ), 3, Hud.INFO )
    Hud.line_from_bottom( "arrows/WASD move Sunny   1/2/3 hide/show a layer", 2, Hud.DIM )
    Hud.line_from_bottom( "PageUp/PageDown or +/- zoom   = reset zoom   Home reset", 1, Hud.DIM )
end
