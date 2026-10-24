--[[
    Tile-map layer smoke test - run by scripts/tilemap_layer_smoke.py (and ci.yml, every
    platform):

        scarab --headless --fast --max-frames 200 scripts/tilemap_layer_smoke/project.json

    Pins a layer's opacity as tilemap_get_layer / tilemap_set_layer (and their _by_name
    forms) exchange it (doc/MASTER_PLAN.md item V3, sunlight BREAKAGES entry 41): an integer
    0..255, 255 = opaque, converted to and from libtmx's own 0.0..1.0. Up to sunlight v0.38.0
    the two directions were not inverses: an opaque layer read back as 256, writing 255 made a
    layer invisible, and writing a layer's own values back stored -1.0.

    resources/tilemap/test.tmx: layer 1 "sky" is opaque, layer 2 "background_clouds" is
    authored at opacity 0.69 and layer 3 "background" at 0.65.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local OPAQUE             = 255
local SKY                = { id = 1, name = "sky" }
local BACKGROUND_CLOUDS  = { id = 2, name = "background_clouds", authored = 0.69 }
local BACKGROUND         = { id = 3, name = "background", authored = 0.65 }
local NO_SUCH_LAYER      = 999

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- what a 0..1 opacity reads back as: sunlight rounds opacity x 255 to the nearest integer
local function expected_opacity( fraction )
    return math.floor( fraction * OPAQUE + 0.5 )
end

local function opacity_of( layerId )
    local _, opacity = tilemap_get_layer( layerId )
    return opacity
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- the runner's working directory is the repo root
check( tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT ), "resources/tilemap/test.tmx failed to load" )

-- 1. the authored values, read back
check( opacity_of( SKY.id ) == OPAQUE, "an opaque layer reads " .. tostring( opacity_of( SKY.id ) ) .. ", want " .. OPAQUE )
check( opacity_of( BACKGROUND_CLOUDS.id ) == expected_opacity( BACKGROUND_CLOUDS.authored ),
       "background_clouds (0.69) reads " .. tostring( opacity_of( BACKGROUND_CLOUDS.id ) ) .. ", want " .. expected_opacity( BACKGROUND_CLOUDS.authored ) )
check( opacity_of( BACKGROUND.id ) == expected_opacity( BACKGROUND.authored ),
       "background (0.65) reads " .. tostring( opacity_of( BACKGROUND.id ) ) .. ", want " .. expected_opacity( BACKGROUND.authored ) )

for _, layer in ipairs( { SKY, BACKGROUND_CLOUDS, BACKGROUND } ) do
    local visible, opacity, offX, offY = tilemap_get_layer( layer.id )
    local visibleN, opacityN, offXN, offYN = tilemap_get_layer_by_name( layer.name )
    check( visible == visibleN and opacity == opacityN and offX == offXN and offY == offYN,
           layer.name .. ": tilemap_get_layer and tilemap_get_layer_by_name disagree" )
end

-- 2. writing a layer's own values back changes nothing (it used to store -1.0 for an opaque layer)
for _, layer in ipairs( { SKY, BACKGROUND_CLOUDS, BACKGROUND } ) do
    local visible, opacity, offX, offY = tilemap_get_layer( layer.id )
    check( tilemap_set_layer( layer.id, visible, opacity, offX, offY ), layer.name .. ": write-back failed" )
    local visible2, opacity2, offX2, offY2 = tilemap_get_layer( layer.id )
    check( visible2 == visible and opacity2 == opacity and offX2 == offX and offY2 == offY,
           layer.name .. ": writing its own values back changed them: opacity " .. tostring( opacity ) .. " -> " .. tostring( opacity2 ) )
end

-- 3. exact round trips, by id and by name
for _, value in ipairs( { 0, 1, 128, 254, OPAQUE } ) do
    check( tilemap_set_layer( SKY.id, true, value, 0, 0 ), "tilemap_set_layer( opacity " .. value .. " ) failed" )
    check( opacity_of( SKY.id ) == value, "opacity " .. value .. " read back as " .. tostring( opacity_of( SKY.id ) ) )

    check( tilemap_set_layer_by_name( BACKGROUND.name, true, value, 0, 0 ), "tilemap_set_layer_by_name( opacity " .. value .. " ) failed" )
    local _, byName = tilemap_get_layer_by_name( BACKGROUND.name )
    check( byName == value, "by name: opacity " .. value .. " read back as " .. tostring( byName ) )
end

-- writing 255 keeps the layer visible and reads back 255. (Old sunlight stored opacity 0.0 for
-- 255, invisible on screen, but its read formula turned 0.0 back into 255, so that bug can't be
-- seen from Lua at all: this pins only the read-back. The other checks catch the old pin.)
check( tilemap_set_layer( SKY.id, true, OPAQUE, 0, 0 ), "tilemap_set_layer( 255 ) failed" )
local skyVisible, skyOpacity = tilemap_get_layer( SKY.id )
check( skyVisible == true and skyOpacity == OPAQUE, "after writing 255: visible " .. tostring( skyVisible ) .. ", opacity " .. tostring( skyOpacity ) )

-- 4. out-of-range values are clamped to 0..255
check( tilemap_set_layer( SKY.id, true, 300, 0, 0 ), "tilemap_set_layer( 300 ) failed" )
check( opacity_of( SKY.id ) == OPAQUE, "opacity 300 read back as " .. tostring( opacity_of( SKY.id ) ) .. ", want it clamped to " .. OPAQUE )
check( tilemap_set_layer( SKY.id, true, -5, 0, 0 ), "tilemap_set_layer( -5 ) failed" )
check( opacity_of( SKY.id ) == 0, "opacity -5 read back as " .. tostring( opacity_of( SKY.id ) ) .. ", want it clamped to 0" )

-- the other fields round-trip with the opacity
check( tilemap_set_layer( SKY.id, false, 128, 7, -3 ), "tilemap_set_layer( false, 128, 7, -3 ) failed" )
local v, o, x, y = tilemap_get_layer( SKY.id )
check( v == false and o == 128 and x == 7 and y == -3, "fields read back as " .. tostring( v ) .. " " .. tostring( o ) .. " " .. tostring( x ) .. " " .. tostring( y ) )

-- an unknown layer: nil, "message" from both a get and a set (item D3)
local unknownGet, unknownGetErr = tilemap_get_layer( NO_SUCH_LAYER )
local unknownSet, unknownSetErr = tilemap_set_layer( NO_SUCH_LAYER, true, OPAQUE, 0, 0 )
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
