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
 zorder - which of a layer's sprites draws on top (docs/lua-api/sprite.html, "Z-order").

 Four overlapping cards on ONE layer. Every frame a layer's sprites are drawn
 in ascending z-order (sprite_set_z_order), so the highest is on top. Keys
 1-4 bring a card to the front (a z-order above every other card's);
 Shift+1-4 send it to the back (below every other's). 0 sets every card back
 to the same z-order, 0: cards with equal z-orders draw in the order they
 were added, so the starting stack (red under green under blue under gold)
 comes back, whatever happened before.

 The glow is on a HIGHER layer, so it stays above every card whatever their
 z-orders: z-order only orders the sprites of one layer, layers still draw in
 their own order. No map is loaded: with none, a layer id is just a grouping
 key (see samples/mapless).
]]

local __ZOOM         <const> = 1.0     -- the cards are 120 px: draw them at their own size
local __CARD_LAYER   <const> = 1
local __GLOW_LAYER   <const> = 2       -- a higher layer: always above the cards
local __HELD_FRAME   <const> = -1      -- one-frame sprites: never step
local __CARD_STEP    <const> = { x = 50, y = 40 }   -- how far each card sits from the previous one
local __FIRST_CARD   <const> = { x = 300, y = 220 }
local __GLOW_POS     <const> = { x = 330, y = 230 }
local __EQUAL_Z      <const> = 0

local __CARDS <const> = {
    { name = "red",   path = "resources/sprites/card_red.png",   key = KEY_ONE },
    { name = "green", path = "resources/sprites/card_green.png", key = KEY_TWO },
    { name = "blue",  path = "resources/sprites/card_blue.png",  key = KEY_THREE },
    { name = "gold",  path = "resources/sprites/card_gold.png",  key = KEY_FOUR },
}

local renderer, err = renderer_create{
    title          = "Scarab - z-order sample",
    zoom           = __ZOOM,
    preferred_zoom = __ZOOM,
}
assert( renderer, err )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local Hud = dofile( "samples/shared/hud.lua" )

-- One single-frame sprite, added to a layer with no map loaded.
local function add_sprite( tag, path, x, y, layerId )
    assert( pool_register_type( tag, 1 ) )

    local handle = assert( sprite_acquire( tag ) )

    assert( sprite_configure_texture( handle, 0, path, 1, 0, TEXTURE_ANIMATION_MODE_MANUAL, __HELD_FRAME ) )
    assert( sprite_set_active_sequence( handle, 0 ) )
    assert( sprite_set_pos( handle, x, y ) )
    assert( sprite_add_to_layer( handle, layerId ) )

    return handle
end

-- The cards are added red, green, blue, gold: with equal z-orders, that is their stack, back to front.
local cards = {}

for index, card in ipairs( __CARDS ) do
    cards[index] = {
        name   = card.name,
        key    = card.key,
        handle = add_sprite( "card_" .. card.name, card.path, __FIRST_CARD.x + ( index - 1 ) * __CARD_STEP.x,
                             __FIRST_CARD.y + ( index - 1 ) * __CARD_STEP.y, __CARD_LAYER ),
    }
end

add_sprite( "glow", "resources/sprites/glow.png", __GLOW_POS.x, __GLOW_POS.y, __GLOW_LAYER )

local message = "the cards start with equal z-orders: drawn in the order they were added"

-- The highest and lowest z-order any card has now.
local function z_range()
    local highest, lowest = nil, nil

    for _, card in ipairs( cards ) do
        local z = assert( sprite_get_z_order( card.handle ) )

        highest = ( highest == nil or z > highest ) and z or highest
        lowest  = ( lowest == nil or z < lowest ) and z or lowest
    end

    return highest, lowest
end

-- The cards back to front, as sunlight draws them: by z-order, equal ones in the order they were added.
local function draw_order()
    local order = {}

    for index, card in ipairs( cards ) do
        order[index] = { name = card.name, index = index, z = assert( sprite_get_z_order( card.handle ) ) }
    end

    table.sort( order, function( a, b )
        if a.z ~= b.z then return a.z < b.z end

        return a.index < b.index
    end )

    local names = {}

    for _, entry in ipairs( order ) do names[#names + 1] = entry.name end

    return table.concat( names, " < " )
end

function on_update( dt )

    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    for _, card in ipairs( cards ) do
        if input_is_key_released( card.key ) then
            local highest, lowest = z_range()

            if shift then
                assert( sprite_set_z_order( card.handle, lowest - 1 ) )
                message = card.name .. " sent to the back"
            else
                assert( sprite_set_z_order( card.handle, highest + 1 ) )
                message = card.name .. " brought to the front"
            end
        end
    end

    if input_is_key_released( KEY_ZERO ) then
        for _, card in ipairs( cards ) do assert( sprite_set_z_order( card.handle, __EQUAL_Z ) ) end

        message = "every card at z-order 0: the order they were added in again"
    end

    -- HUD
    Hud.line( "z-order sample - which of a layer's sprites draws on top", 1, Hud.TITLE )
    Hud.line( "1-4 bring a card to the front   Shift+1-4 send it to the back   0 all equal", 2, Hud.TITLE )

    local values = {}

    for index, card in ipairs( cards ) do
        values[#values + 1] = index .. " " .. card.name .. " z=" .. sprite_get_z_order( card.handle )
    end

    Hud.line( table.concat( values, "   " ), 4, Hud.INFO )
    Hud.line( "drawn back to front: " .. draw_order(), 5, Hud.INFO )
    Hud.line( message, 6, Hud.ALERT )
    Hud.line( "the glow is on a higher layer: always above every card", 7, Hud.DIM )
end
