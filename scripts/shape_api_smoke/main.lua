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
    Shape API smoke test - run by scripts/shape_api_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/shape_api_smoke/project.json

    Pins plans/MASTER_PLAN.md item C2 (sunlight M1, BREAKAGES 38): shape_draw_rectangle / _ellipse /
    _polyline / _polygon / _point. First, before any window: every kind of refusal of every function,
    and that a refused call never creates the renderer. Then, in frames: every function returns true
    with valid arguments - first with NO map loaded, then with a map loaded and the camera scrolled -
    including thick, rotated and fractional shapes, many of them per frame. WHAT a shape looks like
    can't be seen headless (the null renderer draws nothing a script can read): sunlight's own shape
    tests pin the drawing, and samples/surface shows it in a window.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every failure and
    never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __MAP           <const> = "resources/tilemap/test.tmx"
local __MAPLESS_FRAME <const> = 2      -- the frame that draws with no map
local __MAP_FRAME     <const> = 4      -- the frame that draws with a map loaded
local __LAST_FRAME    <const> = 6
local __MANY          <const> = 200    -- shapes drawn in one frame
local __SCROLL        <const> = 64     -- camera position on the loaded map

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

-- Check that a shape call returned true, naming the message it returned otherwise.
local function expect_true( label, value, message )
    check( value == true, label .. ": expected true, got " .. tostring( value ) .. " " .. tostring( message ) )
end

-- The engine asks Lua to claim each stage id an sp_load_stage queues: claim any (this test loads its maps itself),
-- so an unclaimed id can never stop the run.
function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- part 1, before any window: every kind of refusal
local __TRIANGLE <const> = { 0, 0, 40, 0, 20, 30 }

expect_error( "rect x", "argument 'x' must be a number", shape_draw_rectangle( "1", 0, 10, 10, 255, 255, 255, 255 ) )
expect_error( "rect y NaN", "argument 'y' must be a finite number", shape_draw_rectangle( 0, 0 / 0, 10, 10, 255, 255, 255, 255 ) )
expect_error( "rect width", "argument 'width' must be 0 or more", shape_draw_rectangle( 0, 0, -1, 10, 255, 255, 255, 255 ) )
expect_error( "rect height inf", "argument 'height' must be a finite number", shape_draw_rectangle( 0, 0, 10, math.huge, 255, 255, 255, 255 ) )
expect_error( "rect r", "argument 'r'", shape_draw_rectangle( 0, 0, 10, 10, 256, 255, 255, 255 ) )
expect_error( "rect a missing", "argument 'a'", shape_draw_rectangle( 0, 0, 10, 10, 255, 255, 255 ) )
expect_error( "rect a fraction", "argument 'a' must be an integer", shape_draw_rectangle( 0, 0, 10, 10, 255, 255, 255, 0.5 ) )
expect_error( "rect options type", "argument 'options' must be a table", shape_draw_rectangle( 0, 0, 10, 10, 255, 255, 255, 255, 3 ) )
expect_error( "rect unknown option", "unknown option 'thikness'", shape_draw_rectangle( 0, 0, 10, 10, 255, 255, 255, 255, { thikness = 2 } ) )
expect_error( "rect thickness 0", "option 'thickness' must be a number greater than 0", shape_draw_rectangle( 0, 0, 10, 10, 255, 255, 255, 255, { thickness = 0 } ) )
expect_error( "rect rotation", "option 'rotation' must be a finite number", shape_draw_rectangle( 0, 0, 10, 10, 255, 255, 255, 255, { rotation = "90" } ) )
expect_error( "ellipse width", "argument 'width' must be 0 or more", shape_draw_ellipse( 0, 0, -5, 10, 255, 255, 255, 255 ) )
expect_error( "ellipse g", "argument 'g'", shape_draw_ellipse( 0, 0, 10, 10, 0, -1, 0, 255 ) )
expect_error( "ellipse option", "unknown option 'size'", shape_draw_ellipse( 0, 0, 10, 10, 0, 0, 0, 255, { size = 2 } ) )
expect_error( "polyline points type", "argument 'points' must be a table", shape_draw_polyline( 0, 0, "x", 255, 255, 255, 255 ) )
expect_error( "polyline odd", "an even count of numbers, got 3", shape_draw_polyline( 0, 0, { 1, 2, 3 }, 255, 255, 255, 255 ) )
expect_error( "polyline one point", "needs at least 2 points", shape_draw_polyline( 0, 0, { 1, 2 }, 255, 255, 255, 255 ) )
expect_error( "polyline entry", "entry 2 must be a finite number", shape_draw_polyline( 0, 0, { 1, "2", 3, 4 }, 255, 255, 255, 255 ) )
expect_error( "polyline keys", "must be a plain list of numbers", shape_draw_polyline( 0, 0, { 1, 2, 3, 4, n = 1 }, 255, 255, 255, 255 ) )
expect_error( "polyline b", "argument 'b'", shape_draw_polyline( 0, 0, { 1, 2, 3, 4 }, 0, 0, 300, 255 ) )
expect_error( "polygon two points", "needs at least 3 points", shape_draw_polygon( 0, 0, { 1, 2, 3, 4 }, 255, 255, 255, 255 ) )
expect_error( "polygon option", "option 'thickness' must be a number greater than 0", shape_draw_polygon( 0, 0, __TRIANGLE, 255, 255, 255, 255, { thickness = -2 } ) )
expect_error( "point x", "argument 'x' must be a number", shape_draw_point( nil, 0, 255, 255, 255, 255 ) )
expect_error( "point size 0", "argument 'size' must be greater than 0", shape_draw_point( 0, 0, 255, 255, 255, 255, 0 ) )
expect_error( "point size type", "argument 'size' must be a number", shape_draw_point( 0, 0, 255, 255, 255, 255, "big" ) )
expect_error( "point a", "argument 'a'", shape_draw_point( 0, 0, 255, 255, 255 ) )

-- a refused call never reaches the renderer, so none of the above may have created it
check( renderer_get_current() == nil, "a refused shape call created the renderer" )

-- Every shape, with the given label, in one go.
local function draw_every_shape( label, ox, oy )
    expect_true( label .. " rectangle", shape_draw_rectangle( ox, oy, 64, 32, 0, 255, 0, 255 ) )
    expect_true( label .. " thick rotated rectangle", shape_draw_rectangle( ox + 0.5, oy + 0.25, 64.5, 32, 0, 255, 0, 128, { thickness = 3, rotation = 30 } ) )
    expect_true( label .. " zero-size rectangle", shape_draw_rectangle( ox, oy, 0, 0, 0, 255, 0, 255 ) )
    expect_true( label .. " ellipse", shape_draw_ellipse( ox, oy, 96, 48, 255, 128, 0, 255 ) )
    expect_true( label .. " thick rotated ellipse", shape_draw_ellipse( ox, oy, 96, 48, 255, 128, 0, 255, { thickness = 4, rotation = -20 } ) )
    expect_true( label .. " polyline", shape_draw_polyline( ox, oy, { 0, 0, 50, 20, 100, 0 }, 255, 0, 255, 255 ) )
    expect_true( label .. " thick polyline", shape_draw_polyline( ox, oy, { 0, 0, 50, 20, 100, 0 }, 255, 0, 255, 255, { thickness = 2.5 } ) )
    expect_true( label .. " polygon", shape_draw_polygon( ox, oy, __TRIANGLE, 0, 200, 255, 255 ) )
    expect_true( label .. " thick rotated polygon", shape_draw_polygon( ox, oy, __TRIANGLE, 0, 200, 255, 255, { thickness = 3, rotation = 90 } ) )
    expect_true( label .. " point", shape_draw_point( ox, oy, 255, 255, 0, 255 ) )
    expect_true( label .. " big point", shape_draw_point( ox, oy, 255, 255, 0, 255, 6.5 ) )
    expect_true( label .. " options nil", shape_draw_rectangle( ox, oy, 8, 8, 1, 2, 3, 4, nil ) )
    expect_true( label .. " empty options", shape_draw_ellipse( ox, oy, 8, 8, 1, 2, 3, 4, {} ) )

    for i = 1, __MANY do
        check( shape_draw_point( ox + i, oy + i, 255, 255, 255, 255, 1 ) == true, label .. ": a point of many failed" )
    end
end

local frames = 0

-- Called by Scarab every frame: draw every shape with no map (__MAPLESS_FRAME), load a map and scroll the frame
-- before __MAP_FRAME, draw them all again over the map, and at __LAST_FRAME then print OK and quit, or print every failure and keep running, so --max-frames runs out (exit 3).
function on_update( dt )
    frames = frames + 1

    if frames == __MAPLESS_FRAME then
        draw_every_shape( "no map", 10, 10 )
    elseif frames == __MAP_FRAME - 1 then
        check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, "tilemap_load_map failed" )
        check( camera_set_position( __SCROLL, __SCROLL ) == true, "camera_set_position failed" )
    elseif frames == __MAP_FRAME then
        draw_every_shape( "with a map", 100, 100 )
    elseif frames == __LAST_FRAME then
        if #failures == 0 then
            print( "SHAPE API SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "SHAPE API SMOKE FAIL: " .. f ) end
        end
    end
end
