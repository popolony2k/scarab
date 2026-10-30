--[[
    tilemap_load_map smoke test - run by scripts/load_map_smoke.py (and ci.yml, every platform):

        LOADS=<n> scarab --headless --fast --max-frames 200 scripts/load_map_smoke/project.json

    Pins doc/MASTER_PLAN.md item V10 (sunlight BREAKAGES 40): loading a map over a loaded one.
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

local MAP        = "resources/tilemap/test.tmx"
local BROKEN     = "scripts/load_map_smoke/broken.tmx"
local TEXTURE    = "resources/sprites/sunny_idle_down.png"
local MAP_WIDTH  = 20       -- test.tmx is 20x20 tiles of 32 px
local MAP_HEIGHT = 20
local TILE_SIZE  = 32
local FRAMES     = 4        -- texture frames in sunny_idle_down.png
local LAYER      = 4        -- "clouds" in test.tmx
local LOADS      = tonumber( os.getenv( "LOADS" ) or "1" )

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- the loaded map is still test.tmx: 20x20 tiles of 32 px
local function expect_test_map( when )
    local width, height, tileWidth, tileHeight = tilemap_get_map_info()

    check( width == MAP_WIDTH and height == MAP_HEIGHT and tileWidth == TILE_SIZE and tileHeight == TILE_SIZE,
           when .. ": map info is " .. tostring( width ) .. "x" .. tostring( height ) .. ", tiles " ..
           tostring( tileWidth ) .. "x" .. tostring( tileHeight ) .. " - expected test.tmx" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

check( tilemap_load_map( MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, MAP .. " failed to load" )
expect_test_map( "after the first load" )

check( pool_register_type( "sunny", 1 ) == true, "pool_register_type failed" )

local sunny = sprite_acquire( "sunny" )

check( sprite_configure_texture( sunny, 0, TEXTURE, FRAMES, 0, TEXTURE_ANIMATION_MODE_MANUAL ) == true, "configure failed" )
check( sprite_add_to_layer( sunny, LAYER ) == true, "sprite_add_to_layer failed" )

-- a map that fails to parse: refused, and the loaded map stays loaded
local loaded, err = tilemap_load_map( BROKEN, MAP_ALIGNMENT_TOP_LEFT )

check( loaded == nil and tostring( err ):find( "cannot load map", 1, true ), "loading broken.tmx gave " .. tostring( loaded ) .. " " .. tostring( err ) )
expect_test_map( "after a map that fails to parse" )

-- the same map loaded over itself: replaced, and the sprite still registered
for load = 1, LOADS do
    if tilemap_load_map( MAP, MAP_ALIGNMENT_TOP_LEFT ) ~= true then
        check( false, MAP .. " failed to load over itself (load " .. load .. ")" )
        break
    end
end

expect_test_map( "after " .. LOADS .. " loads over the loaded map" )

local added, addErr = sprite_add_to_layer( sunny, LAYER )

check( added == nil and tostring( addErr ):find( "already on layer", 1, true ),
       "after loading over the map, the sprite is no longer registered: sprite_add_to_layer gave " .. tostring( added ) .. " " .. tostring( addErr ) )

local reported = false

function on_update( dt )
    if reported then
        return
    end

    reported = true

    if #failures == 0 then
        print( "LOAD MAP SMOKE OK loads=" .. LOADS )
        app_quit()
    else
        for _, message in ipairs( failures ) do
            print( "LOAD MAP SMOKE FAIL: " .. message )
        end
    end
end
