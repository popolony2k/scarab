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
    Layer parallax smoke test - run by scripts/layer_parallax_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/layer_parallax_smoke/project.json

    Pins plans/MASTER_PLAN.md item C4 (sunlight L1, BREAKAGES 34 and 49): tilemap_get/set_layer_parallax
    (and _by_name) and tilemap_get_parallax_origin, on sunlight's own parallax.tmx (resources/tilemap/
    parallax/). First, before any window: refusals for their arguments, and that none created the renderer.
    Then: no map loaded; the factors the map declares (terrain 1, sky and birds 0.25, clouds 0.5), by id and
    by name; the origin (-640, -416); exact round trips of 0, negative, fractional and large factors; that a
    parallax change leaves the layer's other fields (visible, opacity, offset, tint) as they were; every
    refusal; the declared values back after the map is loaded again; and frames drawn with changed factors.
    WHAT parallax looks like can't be seen headless: sunlight's own tests pin the drawing, and
    samples/parallax shows it in a window.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every failure and never
    quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __MAP          <const> = "resources/tilemap/parallax/parallax.tmx"
local __TERRAIN      <const> = 1
local __BUILDINGS    <const> = 2
local __SKY          <const> = 3
local __CLOUDS       <const> = 4
local __BIRDS        <const> = 5
local __FAR          <const> = 0.25     -- sky and birds, as the map declares
local __HALF         <const> = 0.5      -- clouds
local __ORIGIN       <const> = { x = -640, y = -416 }
local __FLOAT_HUGE   <const> = 1e39     -- past what a float holds: not finite once stored
local __ROUND_TRIPS  <const> = { { 0, 0 }, { 0.3, -0.7 }, { 1.75, 2 }, { -1, 0.1 }, { 1e30, -1e30 } }
local __DRAW_FRAMES  <const> = 5

local failures = {}

-- Record a failure message when `ok` is false. Every check runs; the failures are printed together at the end.
local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

-- Check that a layer's parallax factors ( fx, fy ) are ( wantX, wantY ).
local function factors_are( label, wantX, wantY, fx, fy )
    check( fx == wantX and fy == wantY,
           string.format( "%s: parallax %s, %s - expected %s, %s", label, tostring( fx ), tostring( fy ), tostring( wantX ), tostring( wantY ) ) )
end

-- The engine asks Lua to claim each stage id an sp_load_stage queues: claim any (this test loads its maps itself),
-- so an unclaimed id can never stop the run.
function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- part 1, before any window: refused for their arguments, so no renderer may exist after
expect_error( "get, id type", "argument 'layerId' must be an integer", tilemap_get_layer_parallax( "1" ) )
expect_error( "get, fractional id", "argument 'layerId' must be an integer", tilemap_get_layer_parallax( 1.5 ) )
expect_error( "get by name, name type", "argument 'layerName' must be a string", tilemap_get_layer_parallax_by_name( 3 ) )
expect_error( "set, fx type", "argument 'fx' must be a number", tilemap_set_layer_parallax( 1, "0.5", 1 ) )
expect_error( "set, fy missing", "argument 'fy' must be a number", tilemap_set_layer_parallax( 1, 0.5 ) )
expect_error( "set, NaN", "argument 'fx' must be a finite number", tilemap_set_layer_parallax( 1, 0 / 0, 1 ) )
expect_error( "set, infinity", "argument 'fy' must be a finite number", tilemap_set_layer_parallax( 1, 1, math.huge ) )
expect_error( "set, past a float", "argument 'fx' must be a finite number", tilemap_set_layer_parallax( 1, __FLOAT_HUGE, 1 ) )
expect_error( "set by name, name type", "argument 'layerName' must be a string", tilemap_set_layer_parallax_by_name( nil, 1, 1 ) )
expect_error( "set by name, fx", "argument 'fx' must be a finite number", tilemap_set_layer_parallax_by_name( "sky", -math.huge, 1 ) )
check( renderer_get_current() == nil, "a parallax call refused for its arguments created the renderer" )

-- part 2, no map loaded
expect_error( "get, no map", "no map is loaded", tilemap_get_layer_parallax( __SKY ) )
expect_error( "set, no map", "no map is loaded", tilemap_set_layer_parallax( __SKY, 1, 1 ) )
expect_error( "set by name, no map", "no map is loaded", tilemap_set_layer_parallax_by_name( "sky", 1, 1 ) )
expect_error( "origin, no map", "no map is loaded", tilemap_get_parallax_origin() )

-- part 3, the map's own values
check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, "tilemap_load_map failed" )
factors_are( "terrain", 1, 1, tilemap_get_layer_parallax( __TERRAIN ) )
factors_are( "buildings", 1, 1, tilemap_get_layer_parallax( __BUILDINGS ) )
factors_are( "sky", __FAR, __FAR, tilemap_get_layer_parallax( __SKY ) )
factors_are( "birds", __FAR, __FAR, tilemap_get_layer_parallax( __BIRDS ) )
factors_are( "clouds", __HALF, __HALF, tilemap_get_layer_parallax( __CLOUDS ) )
factors_are( "clouds by name", __HALF, __HALF, tilemap_get_layer_parallax_by_name( "clouds" ) )
factors_are( "sky by name", __FAR, __FAR, tilemap_get_layer_parallax_by_name( "sky" ) )

local originX, originY = tilemap_get_parallax_origin()
check( originX == __ORIGIN.x and originY == __ORIGIN.y, "the parallax origin is " .. tostring( originX ) .. ", " .. tostring( originY ) )

-- part 4, round trips, by id and by name, and the other fields untouched
local visible, opacity, offsetX, offsetY = tilemap_get_layer( __SKY )
local r, g, b, a = tilemap_get_layer_tint( __SKY )

for _, pair in ipairs( __ROUND_TRIPS ) do
    check( tilemap_set_layer_parallax( __SKY, pair[1], pair[2] ) == true, "set " .. pair[1] .. ", " .. pair[2] .. " failed" )
    factors_are( "round trip " .. pair[1] .. ", " .. pair[2], pair[1], pair[2], tilemap_get_layer_parallax( __SKY ) )
end

check( tilemap_set_layer_parallax_by_name( "clouds", 0.1, 0.9 ) == true, "set by name failed" )
factors_are( "set by name, read by id", 0.1, 0.9, tilemap_get_layer_parallax( __CLOUDS ) )

local visible2, opacity2, offsetX2, offsetY2 = tilemap_get_layer( __SKY )
local r2, g2, b2, a2 = tilemap_get_layer_tint( __SKY )
check( visible2 == visible and opacity2 == opacity and offsetX2 == offsetX and offsetY2 == offsetY,
       "a parallax change touched the layer's other fields: " .. tostring( visible2 ) .. " " .. tostring( opacity2 ) )
check( r2 == r and g2 == g and b2 == b and a2 == a, "a parallax change touched the layer's tint" )

-- and the reverse: changing the layer's other fields keeps its parallax
check( tilemap_set_layer( __SKY, visible, opacity, offsetX, offsetY ) == true, "tilemap_set_layer failed" )
factors_are( "after tilemap_set_layer", 1e30, -1e30, tilemap_get_layer_parallax( __SKY ) )

-- part 5, every refusal with a map loaded
expect_error( "get, unknown id", "no layer 99", tilemap_get_layer_parallax( 99 ) )
expect_error( "set, unknown id", "no layer 99", tilemap_set_layer_parallax( 99, 1, 1 ) )
expect_error( "get, unknown name", "no layer named 'nope'", tilemap_get_layer_parallax_by_name( "nope" ) )
expect_error( "set, unknown name", "no layer named 'nope'", tilemap_set_layer_parallax_by_name( "nope", 1, 1 ) )
expect_error( "set, NaN with a map", "must be a finite number", tilemap_set_layer_parallax( __SKY, 0 / 0, 0 ) )
factors_are( "a refusal changes nothing", 1e30, -1e30, tilemap_get_layer_parallax( __SKY ) )

-- part 6, loading the map again brings back its own values
check( tilemap_set_layer_parallax( __SKY, 2, 2 ) == true, "set before the reload failed" )
check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, "reloading the map failed" )
factors_are( "sky after a reload", __FAR, __FAR, tilemap_get_layer_parallax( __SKY ) )
factors_are( "clouds after a reload", __HALF, __HALF, tilemap_get_layer_parallax( __CLOUDS ) )

-- part 7, frames drawn with changed factors (0, negative, large)
check( tilemap_set_layer_parallax( __SKY, 0, 0 ) == true and tilemap_set_layer_parallax( __CLOUDS, -0.5, 2.5 ) == true,
       "setting the drawing factors failed" )
check( camera_set_position( 64, 32 ) == true, "camera_set_position failed" )

local frames = 0

-- Called by Scarab every frame: after __DRAW_FRAMES frames drawn with changed factors, check the factors are still
-- the ones written, then then print OK and quit, or print every failure and keep running, so --max-frames runs out (exit 3).
function on_update( dt )
    frames = frames + 1

    if frames == __DRAW_FRAMES then
        factors_are( "sky after drawing", 0, 0, tilemap_get_layer_parallax( __SKY ) )
        factors_are( "clouds after drawing", -0.5, 2.5, tilemap_get_layer_parallax( __CLOUDS ) )

        if #failures == 0 then
            print( "LAYER PARALLAX SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "LAYER PARALLAX SMOKE FAIL: " .. f ) end
        end
    end
end
