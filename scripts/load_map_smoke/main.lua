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
    tilemap_load_map smoke test - run by scripts/load_map_smoke.py (and ci.yml, every platform):

        LOADS=<n> scarab --headless --fast --max-frames 200 scripts/load_map_smoke/project.json

    Pins plans/MASTER_PLAN.md item V10 (sunlight BREAKAGES 40): loading a map over a loaded one.
      - A map that fails to parse (broken.tmx, next to this script) returns nil + the reason and
        leaves the loaded map loaded (up to sunlight v0.38.0 the old map was lost - leaked - and
        no map was left).
      - Sprites stay registered across a load over a loaded map: a sprite added to a layer is
        still on it afterwards (sprite_add_to_layer again reports it already there).
      - Then the map is loaded over itself LOADS more times (default 1). The Python driver runs
        this twice, with few and with many loads, and compares the two runs' peak memory: up to
        sunlight v0.38.0 every load over a loaded map leaked the whole previous map.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __MAP        <const> = "resources/tilemap/test.tmx"
local __BROKEN     <const> = "scripts/load_map_smoke/broken.tmx"
local __TEXTURE    <const> = "resources/sprites/sunny_idle_down.png"
local __MAP_WIDTH  <const> = 20       -- test.tmx is 20x20 tiles of 32 px
local __MAP_HEIGHT <const> = 20
local __TILE_SIZE  <const> = 32
local __FRAMES     <const> = 4        -- texture frames in sunny_idle_down.png
local __LAYER      <const> = 4        -- "clouds" in test.tmx
local __LOADS      <const> = tonumber( os.getenv( "LOADS" ) or "1" )

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- the loaded map is still test.tmx: 20x20 tiles of 32 px
local function expect_test_map( when )
    local width, height, tileWidth, tileHeight = tilemap_get_map_info()

    check( width == __MAP_WIDTH and height == __MAP_HEIGHT and tileWidth == __TILE_SIZE and tileHeight == __TILE_SIZE,
           when .. ": map info is " .. tostring( width ) .. "x" .. tostring( height ) .. ", tiles " ..
           tostring( tileWidth ) .. "x" .. tostring( tileHeight ) .. " - expected test.tmx" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, __MAP .. " failed to load" )
expect_test_map( "after the first load" )

check( pool_register_type( "sunny", 1 ) == true, "pool_register_type failed" )

local sunny = sprite_acquire( "sunny" )

check( sprite_configure_texture( sunny, 0, __TEXTURE, __FRAMES, 0, TEXTURE_ANIMATION_MODE_MANUAL ) == true, "configure failed" )
check( sprite_add_to_layer( sunny, __LAYER ) == true, "sprite_add_to_layer failed" )

-- a map that fails to parse: refused, and the loaded map stays loaded
local loaded, err = tilemap_load_map( __BROKEN, MAP_ALIGNMENT_TOP_LEFT )

check( loaded == nil and tostring( err ):find( "cannot load map", 1, true ), "loading broken.tmx gave " .. tostring( loaded ) .. " " .. tostring( err ) )
expect_test_map( "after a map that fails to parse" )

-- the same map loaded over itself: replaced, and the sprite still registered
for load = 1, __LOADS do
    if tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) ~= true then
        check( false, __MAP .. " failed to load over itself (load " .. load .. ")" )
        break
    end
end

expect_test_map( "after " .. __LOADS .. " loads over the loaded map" )

local added, addErr = sprite_add_to_layer( sunny, __LAYER )

check( added == nil and tostring( addErr ):find( "already on layer", 1, true ),
       "after loading over the map, the sprite is no longer registered: sprite_add_to_layer gave " .. tostring( added ) .. " " .. tostring( addErr ) )

local reported = false

function on_update( dt )
    if reported then
        return
    end

    reported = true

    if #failures == 0 then
        print( "LOAD MAP SMOKE OK loads=" .. __LOADS )
        app_quit()
    else
        for _, message in ipairs( failures ) do
            print( "LOAD MAP SMOKE FAIL: " .. message )
        end
    end
end
