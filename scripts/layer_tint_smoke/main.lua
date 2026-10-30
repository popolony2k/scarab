--[[
    Layer tint smoke test - run by scripts/layer_tint_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/layer_tint_smoke/project.json

    Pins doc/MASTER_PLAN.md item V12 (sunlight BREAKAGES 45 and 57, I8): tilemap_get_layer_tint /
    tilemap_set_layer_tint (and their _by_name forms) on resources/tilemap/tint/tint.tmx - every
    authored Tiled "tintcolor" read back exactly, alpha included (opaque white for a layer with
    none), for tile, image, object and group layers; set then get; writing back the values read
    changes nothing; tilemap_set_layer keeps a layer's tint; and every error. Drawing (what a tint
    LOOKS like) is checked by eye with samples/shapes, key 5.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local MAP = "resources/tilemap/tint/tint.tmx"

-- layer id -> the authored tint { r, g, b, a } (tint.tmx's own tintcolor attributes)
local AUTHORED = {
    [1]  = { 255, 255, 255, 255 },   -- tile layer, no tint
    [2]  = { 128, 255, 255, 255 },   -- tile layer #80ffff
    [3]  = { 255, 255, 255, 255 },   -- image layer, no tint
    [4]  = { 128, 128, 255, 255 },   -- image layer #8080ff
    [7]  = { 128, 255, 128, 255 },   -- object layer #80ff80
    [8]  = { 128, 255, 255, 64 },    -- tile layer #4080ffff: alpha kept
    [10] = { 128, 128, 255, 255 },   -- group layer #8080ff
    [12] = { 255, 255, 128, 255 },   -- tile layer inside the group #ffff80
}

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

local function tint_is( label, expected, r, g, b, a )
    check( r == expected[1] and g == expected[2] and b == expected[3] and a == expected[4],
           string.format( "%s: tint %s,%s,%s,%s - expected %d,%d,%d,%d", label, tostring( r ), tostring( g ), tostring( b ),
                          tostring( a ), expected[1], expected[2], expected[3], expected[4] ) )
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

expect_error( "no map", "no map is loaded", tilemap_get_layer_tint( 1 ) )
check( tilemap_load_map( MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, MAP .. " failed to load" )

-- every authored tint, by id
for layerId, expected in pairs( AUTHORED ) do
    tint_is( "layer " .. layerId, expected, tilemap_get_layer_tint( layerId ) )
end

-- by name
tint_is( "by name", AUTHORED[2], tilemap_get_layer_tint_by_name( "tiles tinted cyan" ) )
tint_is( "group by name", AUTHORED[10], tilemap_get_layer_tint_by_name( "group tinted blue, opacity 0.6" ) )

-- set then get, by id and by name
check( tilemap_set_layer_tint( 1, 10, 20, 30, 40 ) == true, "set_layer_tint failed" )
tint_is( "set then get", { 10, 20, 30, 40 }, tilemap_get_layer_tint( 1 ) )
check( tilemap_set_layer_tint_by_name( "tiles untinted", 200, 100, 50, 255 ) == true, "set_layer_tint_by_name failed" )
tint_is( "set by name then get", { 200, 100, 50, 255 }, tilemap_get_layer_tint( 1 ) )
check( tilemap_set_layer_tint( 10, 1, 2, 3, 4 ) == true, "set_layer_tint on a group failed" )
tint_is( "group set then get", { 1, 2, 3, 4 }, tilemap_get_layer_tint( 10 ) )

-- writing back the values read changes nothing (alpha included)
check( tilemap_set_layer_tint( 8, tilemap_get_layer_tint( 8 ) ) == true, "write-back failed" )
tint_is( "write-back", AUTHORED[8], tilemap_get_layer_tint( 8 ) )

-- tilemap_set_layer (visible/opacity/offset) keeps the tint
local visible, opacity, offsetX, offsetY = tilemap_get_layer( 2 )
check( tilemap_set_layer( 2, visible, opacity, offsetX, offsetY ) == true, "set_layer failed" )
tint_is( "after set_layer", AUTHORED[2], tilemap_get_layer_tint( 2 ) )

-- and the tint keeps visibility/opacity/offset
check( tilemap_set_layer_tint( 2, 255, 0, 0, 255 ) == true, "set_layer_tint failed" )
local v2, o2, x2, y2 = tilemap_get_layer( 2 )
check( v2 == visible and o2 == opacity and x2 == offsetX and y2 == offsetY, "set_layer_tint changed the layer's other values" )

-- errors
expect_error( "unknown layer", "no layer 99", tilemap_get_layer_tint( 99 ) )
expect_error( "unknown name", "no layer named 'nope'", tilemap_get_layer_tint_by_name( "nope" ) )
expect_error( "set unknown", "no layer 99", tilemap_set_layer_tint( 99, 0, 0, 0, 0 ) )
expect_error( "channel 256", "argument 'g' must be between 0 and 255", tilemap_set_layer_tint( 1, 0, 256, 0, 0 ) )
expect_error( "channel negative", "argument 'a' must be between 0 and 255", tilemap_set_layer_tint( 1, 0, 0, 0, -1 ) )
expect_error( "channel fraction", "argument 'r' must be an integer", tilemap_set_layer_tint( 1, 0.5, 0, 0, 0 ) )
expect_error( "channel missing", "argument 'a'", tilemap_set_layer_tint( 1, 0, 0, 0 ) )
expect_error( "name type", "argument 'layerName' must be a string", tilemap_set_layer_tint_by_name( 5, 0, 0, 0, 0 ) )

local reported = false

function on_update( dt )
    if reported then return end
    reported = true

    if #failures == 0 then
        print( "LAYER TINT SMOKE OK" )
        app_quit()
    else
        for _, message in ipairs( failures ) do
            print( "LAYER TINT SMOKE FAIL: " .. message )
        end
    end
end
