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
    Tile-map layer smoke test - run by scripts/tilemap_layer_smoke.py (and ci.yml, every
    platform):

        scarab --headless --fast --max-frames 200 scripts/tilemap_layer_smoke/project.json

    Pins a layer's opacity as tilemap_get_layer / tilemap_set_layer (and their _by_name
    forms) exchange it: a number from 0 (transparent) to 1 (opaque), Tiled's own unit, read
    exactly as authored and written back exactly (plans/MASTER_PLAN.md items V3 and P7, sunlight
    BREAKAGES entries 41 and 58). History: up to sunlight v0.38.0 the 0..255 integer Scarab
    exchanged was not converted symmetrically (an opaque layer read 256, writing 255 hid it);
    V3 made the integer exact, but a read-and-write-back still moved an authored 0.5 to 0.502;
    P7 (sunlight I9) moved to 0..1, so nothing moves.

    resources/tilemap/test.tmx: layer 1 "sky" is opaque, layer 2 "background_clouds" is
    authored at opacity 0.69 and layer 3 "background" at 0.65.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __OPAQUE            <const> = 1
local __SKY               <const> = { id = 1, name = "sky" }
local __BACKGROUND_CLOUDS <const> = { id = 2, name = "background_clouds", authored = 0.69 }
local __BACKGROUND        <const> = { id = 3, name = "background", authored = 0.65 }
local __NO_SUCH_LAYER     <const> = 999

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

local function opacity_of( layerId )
    local _, opacity = tilemap_get_layer( layerId )
    return opacity
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- the runner's working directory is the repo root
check( tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT ), "resources/tilemap/test.tmx failed to load" )

-- 1. the authored values, read back exactly (0.69 reads 0.69, not 176 or 0.68999999761...)
check( opacity_of( __SKY.id ) == __OPAQUE, "an opaque layer reads " .. tostring( opacity_of( __SKY.id ) ) .. ", want 1" )
check( opacity_of( __BACKGROUND_CLOUDS.id ) == __BACKGROUND_CLOUDS.authored,
       "background_clouds reads " .. tostring( opacity_of( __BACKGROUND_CLOUDS.id ) ) .. ", want 0.69" )
check( opacity_of( __BACKGROUND.id ) == __BACKGROUND.authored,
       "background reads " .. tostring( opacity_of( __BACKGROUND.id ) ) .. ", want 0.65" )

for _, layer in ipairs( { __SKY, __BACKGROUND_CLOUDS, __BACKGROUND } ) do
    local visible, opacity, offX, offY = tilemap_get_layer( layer.id )
    local visibleN, opacityN, offXN, offYN = tilemap_get_layer_by_name( layer.name )
    check( visible == visibleN and opacity == opacityN and offX == offXN and offY == offYN,
           layer.name .. ": tilemap_get_layer and tilemap_get_layer_by_name disagree" )
end

-- 2. writing a layer's own values back changes nothing, however many times (the 0..255 integer
-- moved an authored 0.5 to 0.502 on every write-back)
for _, layer in ipairs( { __SKY, __BACKGROUND_CLOUDS, __BACKGROUND } ) do
    local visible, opacity, offX, offY = tilemap_get_layer( layer.id )

    for _ = 1, 3 do
        check( tilemap_set_layer( layer.id, tilemap_get_layer( layer.id ) ), layer.name .. ": write-back failed" )
    end

    local visible2, opacity2, offX2, offY2 = tilemap_get_layer( layer.id )
    check( visible2 == visible and opacity2 == opacity and offX2 == offX and offY2 == offY,
           layer.name .. ": writing its own values back changed them: opacity " .. tostring( opacity ) .. " -> " .. tostring( opacity2 ) )
end

-- 3. exact round trips, by id and by name - 0.5 is the value the integer used to move
for _, value in ipairs( { 0, 0.25, 0.3, 0.5, 0.65, 0.69, __OPAQUE } ) do
    check( tilemap_set_layer( __SKY.id, true, value, 0, 0 ), "tilemap_set_layer( opacity " .. value .. " ) failed" )
    check( opacity_of( __SKY.id ) == value, "opacity " .. value .. " read back as " .. tostring( opacity_of( __SKY.id ) ) )

    check( tilemap_set_layer_by_name( __BACKGROUND.name, true, value, 0, 0 ), "tilemap_set_layer_by_name( opacity " .. value .. " ) failed" )
    local _, byName = tilemap_get_layer_by_name( __BACKGROUND.name )
    check( byName == value, "by name: opacity " .. value .. " read back as " .. tostring( byName ) )
end

-- 4. outside 0..1, or not a number: refused, and the layer keeps its opacity
check( tilemap_set_layer( __SKY.id, true, 0.5, 0, 0 ), "tilemap_set_layer( 0.5 ) failed" )
expect_error( "opacity 1.5", "argument 'opacity' must be between 0 and 1", tilemap_set_layer( __SKY.id, true, 1.5, 0, 0 ) )
expect_error( "opacity -0.1", "argument 'opacity' must be between 0 and 1", tilemap_set_layer( __SKY.id, true, -0.1, 0, 0 ) )
expect_error( "opacity 255", "argument 'opacity' must be between 0 and 1", tilemap_set_layer( __SKY.id, true, 255, 0, 0 ) )
expect_error( "opacity string", "argument 'opacity' must be a number", tilemap_set_layer( __SKY.id, true, "0.5", 0, 0 ) )
check( opacity_of( __SKY.id ) == 0.5, "a refused opacity changed the layer: it reads " .. tostring( opacity_of( __SKY.id ) ) )

-- the other fields round-trip with the opacity
check( tilemap_set_layer( __SKY.id, false, 0.5, 7, -3 ), "tilemap_set_layer( false, 0.5, 7, -3 ) failed" )
local v, o, x, y = tilemap_get_layer( __SKY.id )
check( v == false and o == 0.5 and x == 7 and y == -3, "fields read back as " .. tostring( v ) .. " " .. tostring( o ) .. " " .. tostring( x ) .. " " .. tostring( y ) )

-- an unknown layer: nil, "message" from both a get and a set (item D3)
local unknownGet, unknownGetErr = tilemap_get_layer( __NO_SUCH_LAYER )
local unknownSet, unknownSetErr = tilemap_set_layer( __NO_SUCH_LAYER, true, __OPAQUE, 0, 0 )
check( unknownGet == nil and tostring( unknownGetErr ):find( "no layer", 1, true ), "an unknown layer id must read nil, \"no layer ...\"" )
check( unknownSet == nil and tostring( unknownSetErr ):find( "no layer", 1, true ), "writing an unknown layer id must return nil, \"no layer ...\"" )

local frames = 0
function on_update( dt )
    frames = frames + 1
    if frames == 1 then
        if #failures == 0 then
            print( "TILEMAP LAYER SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "TILEMAP LAYER SMOKE FAIL: " .. f ) end
        end
    end
end
