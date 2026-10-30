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

app_set_name( "Scarab - tilemap sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

-- The HUD's font: Caravellius 8x8, loaded once (the built-in font is hard to read at these sizes).
local hudFont = assert( load_font( "resources/fonts/caravellius8x8.fnt" ) )

local mapLoaded = tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER )

local mapWidth, mapHeight, tileWidth, tileHeight

if mapLoaded then
    mapWidth, mapHeight, tileWidth, tileHeight = tilemap_get_map_info()
end

-- A layer's opacity is a number from 0 (transparent) to 1 (fully opaque), as
-- Tiled stores it. The Up/Down arrows raise/lower the "clouds" layer's (id 4 in
-- test.tmx - it also holds the grass islands, so the change is easy to see) by
-- FADE_STEP per press.
local FADE_LAYER   = 4
local OPAQUE       = 1
local FADE_STEP    = 0.125  -- exact in binary: 8 steps land on both 0 and 1 with no rounding

local PLATE_PADDING = 4     -- pixels around a line of text on its plate
local PLATE_ALPHA   = 170   -- the plate's opacity, 0..255

-- A line of text on a dark translucent plate: the text crosses the white
-- clouds of the map, where light text alone can't be read.
local function draw_text_plate( text, x, y, size, r, g, b )
    draw_filled_rectangle( x - PLATE_PADDING, y - PLATE_PADDING,
        measure_text( text, size, hudFont ) + 2 * PLATE_PADDING, size + 2 * PLATE_PADDING, 0, 0, 0, PLATE_ALPHA )
    draw_text( text, x, y, size, r, g, b, 255, hudFont )
end

function on_update( dt )

    local delta = 0

    if input_is_key_released( KEY_UP ) then delta = FADE_STEP end
    if input_is_key_released( KEY_DOWN ) then delta = -FADE_STEP end

    if mapLoaded and delta ~= 0 then
        -- tilemap_set_layer writes all four values: read the layer first, keep the rest
        local visible, opacity, offX, offY = tilemap_get_layer( FADE_LAYER )
        tilemap_set_layer( FADE_LAYER, visible, math.max( 0, math.min( opacity + delta, OPAQUE ) ), offX, offY )
    end

    draw_text_plate( "tilemap sample - Up/Down change the clouds layer's opacity", 20, 20, 16, 255, 255, 255 )

    if mapLoaded then
        local _, opacity = tilemap_get_layer( FADE_LAYER )

        draw_text_plate( string.format( "map: %dx%d tiles of %dx%d px (tilemap_get_map_info)", mapWidth, mapHeight, tileWidth, tileHeight ),
            20, 48, 16, 220, 220, 220 )
        draw_text_plate( string.format( "clouds layer opacity: %g of 1 (tilemap_get_layer)", opacity ), 20, 76, 16, 255, 220, 120 )
    else
        draw_text_plate( "tilemap_load_map FAILED", 20, 48, 16, 255, 80, 80 )
    end
end
