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
 tilemap - loading a Tiled .tmx map (docs/lua-api/tilemap.html).

 The minimal map-on-screen sample - tilemap_load_map plus
 tilemap_get_map_info, and a layer's opacity read and written with
 tilemap_get_layer / tilemap_set_layer (Up/Down arrows), no sprites, no camera
 movement (see the "camera"
 sample, which builds on this exact same map for scrolling/zoom
 controls, and "sprite", which adds an animated sprite on top).

 The map itself (resources/tilemap/test.tmx and its tilesets/images) is
 shared with sunlight's own "tilemaprenderer" sample - see
 resources/tilemap/README.md.
]]

assert( app_set_name( "Scarab - tilemap sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local Hud = dofile( "samples/shared/hud.lua" )

-- Not asserted: a failed load is shown on the HUD, with its reason.
local mapLoaded, mapError = tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER )

local mapWidth, mapHeight, tileWidth, tileHeight

if mapLoaded then
    mapWidth, mapHeight, tileWidth, tileHeight = tilemap_get_map_info()
end

-- A layer's opacity is a number from 0 (transparent) to 1 (fully opaque), as
-- Tiled stores it. The Up/Down arrows raise/lower the "clouds" layer's (id 4 in
-- test.tmx - it also holds the grass islands, so the change is easy to see) by
-- __FADE_STEP per press.
local __FADE_LAYER <const> = 4
local __OPAQUE     <const> = 1
local __FADE_STEP  <const> = 0.125  -- exact in binary: 8 steps land on both 0 and 1 with no rounding

function on_update( dt )

    local delta = 0

    if input_is_key_released( KEY_UP ) then delta = __FADE_STEP end
    if input_is_key_released( KEY_DOWN ) then delta = -__FADE_STEP end

    if mapLoaded and delta ~= 0 then
        -- tilemap_set_layer writes all four values: read the layer first, keep the rest
        local visible, opacity, offX, offY = tilemap_get_layer( __FADE_LAYER )
        tilemap_set_layer( __FADE_LAYER, visible, math.max( 0, math.min( opacity + delta, __OPAQUE ) ), offX, offY )
    end

    Hud.line( "tilemap sample - Up/Down change the clouds layer's opacity", 1, Hud.TITLE )

    if mapLoaded then
        local _, opacity = tilemap_get_layer( __FADE_LAYER )

        Hud.line( string.format( "map: %dx%d tiles of %dx%d px (tilemap_get_map_info)", mapWidth, mapHeight, tileWidth, tileHeight ), 2, Hud.DIM )
        Hud.line( string.format( "clouds layer opacity: %g of 1 (tilemap_get_layer)", opacity ), 3, Hud.INFO )
    else
        Hud.line( "tilemap_load_map FAILED: " .. tostring( mapError ), 2, Hud.ALERT )
    end
end
