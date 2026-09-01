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

app_set_name( "Scarab - json sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local data = load_json( "samples/json/data.json" )

function on_update( dt )

    draw_text( "json sample", 20, 20, 24, 255, 255, 255, 255 )

    if data == nil then
        draw_text( "load_json FAILED", 20, 54, 20, 255, 80, 80, 255 )

        return
    end

    draw_text( "title: " .. tostring( data.title ) .. "   debug: " .. tostring( data.debug ), 20, 54, 20, 200, 200, 200, 255 )

    local y = 82

    for index, enemy in ipairs( data.enemies ) do
        draw_text( index .. ". " .. enemy.name .. " (hp " .. enemy.hp .. ")", 20, y, 18, 200, 200, 200, 255 )
        y = y + 24
    end
end
