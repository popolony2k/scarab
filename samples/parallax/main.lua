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
 parallax - Tiled's per-layer parallax, and changing it live
 (docs/lua-api/tilemap.html, "Layer parallax"). Scarab's counterpart of
 sunlight's own "parallax" sample, on its map (resources/tilemap/parallax/).

 Kepi, Scarab's mascot, stays at the centre of the view; walking moves the camera, and the
 map wraps around four marker objects so the walk never ends. The layers
 scroll at the speeds the map gives them: terrain and buildings at 1, the
 clouds at 0.5, the sky and the birds at 0.25.

 1-4 pick a layer (sky, birds, clouds, buildings); [ and ] change its
 factor by 0.25 (both axes), 0 sets it to 0 (fixed on the view), N turns it
 the other way, R restores every layer's factor from the map. Zoom out is
 limited so the view never reaches past the map's edge.
]]

local Hud = dofile( "samples/shared/hud.lua" )

local __MAP           <const> = "resources/tilemap/parallax/parallax.tmx"
local __KEPI         <const> = "resources/sprites/kepi_idle.png"   -- Kepi, Scarab's mascot (temporary art)
local __KEPI_FRAMES  <const> = 4
local __KEPI_DELAY   <const> = 100     -- ms per frame; without one the frame is held
local __KEPI_SIZE    <const> = 32
local __KEPI_LAYER   <const> = 4       -- "clouds", the last layer: drawn over everything
local __STEP          <const> = 4       -- map pixels walked per frame while a key is held
local __FACTOR_STEP   <const> = 0.25    -- what [ and ] change a factor by
local __VIEW          <const> = 0       -- the default view
local __LAYER_ROW     <const> = 4       -- HUD rows: the layers from here, then the state and the last message
local __LAYERS        <const> = {       -- the layers 1-4 pick
    { key = KEY_ONE,   id = 3, name = "sky" },
    { key = KEY_TWO,   id = 5, name = "birds" },
    { key = KEY_THREE, id = 4, name = "clouds" },
    { key = KEY_FOUR,  id = 2, name = "buildings" },
}

assert( app_set_name( "Scarab - parallax sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

assert( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) )

-- zoom_reset returns to the PREFERRED zoom (see samples/camera): make it the starting one.
assert( view_set_preferred_zoom( __VIEW, viewport_get_zoom_factor() ) )

-- The wrap rectangle, from the map's marker objects.
local wrap = {
    left   = assert( tilemap_get_object_by_name( "wrap_left" ) ),
    right  = assert( tilemap_get_object_by_name( "wrap_right" ) ),
    top    = select( 2, assert( tilemap_get_object_by_name( "wrap_top" ) ) ),
    bottom = select( 2, assert( tilemap_get_object_by_name( "wrap_bottom" ) ) ),
}

-- The factors the map declares, for R.
local authored = {}

for _, layer in ipairs( __LAYERS ) do
    local fx, fy = assert( tilemap_get_layer_parallax( layer.id ) )

    authored[layer.id] = { fx, fy }
end

local originX, originY = assert( tilemap_get_parallax_origin() )

-- Limit zooming out so the view, centred on Kepi at an edge of the wrap rectangle, stays inside the map
-- (sunlight's sample does the same): half the visible size must fit in the map beyond the rectangle.
local function limit_zoom_out()
    local mapWidth, mapHeight, tileWidth, tileHeight = assert( tilemap_get_map_info() )
    local _, _, viewWidth, viewHeight = viewport_get_dimension()
    local marginX = math.min( wrap.left, mapWidth * tileWidth - wrap.right )
    local marginY = math.min( wrap.top, mapHeight * tileHeight - wrap.bottom )
    local minZoom = math.max( viewWidth / ( 2 * marginX ), viewHeight / ( 2 * marginY ) )
    local _, maxZoom = assert( view_get_zoom_limits( __VIEW ) )

    assert( view_set_zoom_limits( __VIEW, math.ceil( minZoom / ZOOM_STEP ) * ZOOM_STEP, maxZoom ) )
end

limit_zoom_out()

-- Kepi: a screen-space sprite, kept at the view's centre, so no layer's factor moves it.
assert( pool_register_type( "kepi", 1 ) )

local kepi = assert( sprite_acquire( "kepi" ) )

assert( sprite_configure_texture( kepi, 0, __KEPI, __KEPI_FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, __KEPI_DELAY ) )
assert( sprite_set_active_sequence( kepi, 0 ) )
assert( sprite_add_to_layer( kepi, __KEPI_LAYER ) )
assert( sprite_set_visible( kepi, true ) )

local worldX   = ( wrap.left + wrap.right ) // 2     -- where Kepi walks, in map pixels
local worldY   = ( wrap.top + wrap.bottom ) // 2
local selected = 1
local message  = "1-4 pick a layer"

-- Wrap Kepi's map position around the marker rectangle.
local function wrap_position()
    if worldX > wrap.right then worldX = wrap.left + ( worldX - wrap.right )
    elseif worldX < wrap.left then worldX = wrap.right - ( wrap.left - worldX ) end

    if worldY > wrap.bottom then worldY = wrap.top + ( worldY - wrap.bottom )
    elseif worldY < wrap.top then worldY = wrap.bottom - ( wrap.top - worldY ) end
end

-- Centre the camera on Kepi's map position, and keep the sprite at the view's centre at any zoom
-- (a screen-space position is multiplied by the zoom, so it is stored divided by it).
local function follow_kepi()
    local zoom = viewport_get_zoom_factor()
    local _, _, viewWidth, viewHeight = viewport_get_dimension()

    assert( camera_set_position( math.floor( worldX - viewWidth / zoom / 2 ), math.floor( worldY - viewHeight / zoom / 2 ) ) )
    assert( sprite_set_pos( kepi, math.floor( ( viewWidth / 2 - __KEPI_SIZE / 2 ) / zoom ),
                                   math.floor( ( viewHeight / 2 - __KEPI_SIZE / 2 ) / zoom ) ) )
end

follow_kepi()

-- Change the selected layer's factors with fn( fx, fy ) -> fx, fy.
local function change_selected( fn, what )
    local layer  = __LAYERS[selected]
    local fx, fy = assert( tilemap_get_layer_parallax( layer.id ) )

    fx, fy = fn( fx, fy )
    assert( tilemap_set_layer_parallax( layer.id, fx, fy ) )
    message = layer.name .. ": " .. what
end

function on_update( dt )

    -- walking moves the camera
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then worldY = worldY - __STEP end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then worldY = worldY + __STEP end
    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then worldX = worldX - __STEP end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then worldX = worldX + __STEP end

    wrap_position()

    -- zoom, as in samples/camera
    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    if input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or ( shift and input_is_key_down( KEY_EQUAL ) ) then zoom_in() end
    if input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then zoom_out() end
    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) or input_is_key_released( KEY_HOME ) then
        zoom_reset()
    end

    follow_kepi()

    -- live parallax changes
    for index, layer in ipairs( __LAYERS ) do
        if input_is_key_released( layer.key ) then
            selected = index
            message  = layer.name .. " selected"
        end
    end

    if input_is_key_released( KEY_RIGHT_BRACKET ) then
        change_selected( function( fx, fy ) return fx + __FACTOR_STEP, fy + __FACTOR_STEP end, "faster" )
    elseif input_is_key_released( KEY_LEFT_BRACKET ) then
        change_selected( function( fx, fy ) return fx - __FACTOR_STEP, fy - __FACTOR_STEP end, "slower" )
    elseif input_is_key_released( KEY_ZERO ) then
        change_selected( function() return 0, 0 end, "fixed on the view" )
    elseif input_is_key_released( KEY_N ) then
        change_selected( function( fx, fy ) return -fx, -fy end, "the other way" )
    elseif input_is_key_released( KEY_R ) then
        for id, pair in pairs( authored ) do assert( tilemap_set_layer_parallax( id, pair[1], pair[2] ) ) end   -- any order: each layer is set on its own

        message = "every layer back to the map's factors"
    end

    -- HUD
    Hud.line( "parallax sample - layers scroll at their own speed", 1, Hud.TITLE )
    Hud.line( "arrows/WASD walk   PageUp/PageDown zoom   = reset zoom", 2, Hud.TITLE )
    Hud.line( "1-4 pick a layer   [ ] slower/faster   0 fixed   N other way   R map's values", 3, Hud.TITLE )

    for index, layer in ipairs( __LAYERS ) do
        local fx, fy = tilemap_get_layer_parallax( layer.id )

        Hud.line( string.format( "%s %d %s: %g, %g", index == selected and ">" or " ", index, layer.name, fx, fy ),
                  __LAYER_ROW + index - 1, index == selected and Hud.ALERT or Hud.INFO )
    end

    local camX, camY = camera_get_position()

    Hud.line( string.format( "terrain 1, 1   origin %g, %g   camera %d, %d   zoom %.2f", originX, originY, camX, camY,
                             viewport_get_zoom_factor() ), __LAYER_ROW + #__LAYERS, Hud.DIM )
    Hud.line( message, __LAYER_ROW + #__LAYERS + 1, Hud.ALERT )
end
