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
 tilemap_get_map_info, no sprites, no camera movement (see the "camera"
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

local mapLoaded = tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER )

local mapWidth, mapHeight, tileWidth, tileHeight

if mapLoaded then
    mapWidth, mapHeight, tileWidth, tileHeight = tilemap_get_map_info()
end

function on_update( dt )

    draw_text( "tilemap sample", 20, 20, 24, 255, 255, 255, 255 )

    if mapLoaded then
        draw_text( "tilemap_load_map succeeded - " .. mapWidth .. "x" .. mapHeight .. " tiles, "
            .. tileWidth .. "x" .. tileHeight .. "px each (tilemap_get_map_info)", 20, 54, 20, 200, 200, 200, 255 )
    else
        draw_text( "tilemap_load_map FAILED", 20, 54, 20, 255, 80, 80, 255 )
    end
end
