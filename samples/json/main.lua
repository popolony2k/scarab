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
 json - load_json (docs/lua-api/json.html), the engine's only
 config-file bridge.

 Loads data.json (a small object containing a boolean, a string, and a
 nested array of objects) and walks the resulting Lua table - JSON
 objects become string-keyed tables, JSON arrays become 1-based,
 ipairs-friendly tables, exactly as load_json's own doc describes.
]]

local Hud = dofile( "samples/shared/hud.lua" )

-- The HUD row of the first enemy (rows 1 and 2 are the title and the top-level values).
local __FIRST_ENEMY_ROW <const> = 3

assert( app_set_name( "Scarab - json sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

-- load_json returns nil, "message" on failure: kept, so the HUD can show it.
local data, loadError = load_json( "samples/json/data.json" )

function on_update( dt )

    Hud.line( "json sample", 1, Hud.TITLE )

    if data == nil then
        Hud.line( "load_json FAILED: " .. tostring( loadError ), 2, Hud.ALERT )

        return
    end

    Hud.line( "title: " .. tostring( data.title ) .. "   debug: " .. tostring( data.debug ), 2, Hud.INFO )

    for index, enemy in ipairs( data.enemies ) do
        Hud.line( index .. ". " .. enemy.name .. " (hp " .. enemy.hp .. ")", __FIRST_ENEMY_ROW + index - 1, Hud.DIM )
    end
end
