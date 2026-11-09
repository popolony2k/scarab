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
    View opacity smoke test - run by scripts/view_opacity_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/view_opacity_smoke/project.json

    Pins plans/MASTER_PLAN.md item C7, part 1 (sunlight I19a): view_set_opacity / view_get_opacity.
    - Before any window: asking view 0 is "unknown view 0" and creates no renderer.
    - Every view starts at 1: the default view and a view made by view_create.
    - Exact round trips (0.3 reads 0.3) on the default view and an extra one; 0 and 1 as integers too.
    - Each view keeps its own opacity; a map load keeps it.
    - Every refusal: below 0, above 1, NaN, infinity, a string, a table, missing; an unknown view; a destroyed one.
    - Frames are drawn with the default view at 0.5 and an extra view at 0 (headless draws nothing, but the
      composite path runs; ASan watches it), then everything back at 1.
    What a faded view LOOKS like is checked by eye in samples/fade.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every failure and never
    quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __MAP          <const> = "resources/tilemap/test.tmx"
local __DRAW_FRAMES  <const> = 30          -- frames drawn with views faded
local __VALUES       <const> = { 0, 0.3, 0.25, 0.1, 0.999, 1 }

local failures = {}
local reported = false

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- calls f( ... ) protected: it must not raise, and must return nil + a message containing `needle`
local function expect_error( label, needle, f, ... )
    local ok, value, message = pcall( f, ... )

    if not ok then
        check( false, label .. ": raised a Lua error: " .. tostring( value ) )
        return
    end

    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

-- Every value of __VALUES must read back exactly on `view`.
local function round_trips( label, view )
    for _, value in ipairs( __VALUES ) do
        check( view_set_opacity( view, value ) == true, label .. ": set " .. value .. " failed" )
        check( view_get_opacity( view ) == value, label .. ": " .. value .. " read back as " .. tostring( view_get_opacity( view ) ) )
    end
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- before any window: no view exists, and asking creates no renderer
expect_error( "before a window", "unknown view 0", view_get_opacity, 0 )
expect_error( "before a window, set", "unknown view 0", view_set_opacity, 0, 0.5 )
check( renderer_get_current() == nil, "asking a view's opacity created the renderer" )

check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, "load failed" )

local renderer = renderer_get_current()
local extra    = view_create( renderer, 20, 20, 300, 200 )

check( extra ~= nil, "view_create failed" )
check( view_get_opacity( 0 ) == 1, "the default view does not start at 1" )
check( view_get_opacity( extra ) == 1, "an extra view does not start at 1" )

round_trips( "default view", 0 )
round_trips( "extra view", extra )

-- each view keeps its own
check( view_set_opacity( 0, 0.4 ) == true and view_set_opacity( extra, 0.7 ) == true, "set failed" )
check( view_get_opacity( 0 ) == 0.4 and view_get_opacity( extra ) == 0.7, "views do not keep their own opacity" )

-- a map load keeps it
check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, "reload failed" )
check( view_get_opacity( 0 ) == 0.4 and view_get_opacity( extra ) == 0.7, "a map load changed a view's opacity" )

-- refusals (the value stays as it was)
expect_error( "below 0", "argument 'opacity' must be between 0 and 1", view_set_opacity, 0, -0.01 )
expect_error( "above 1", "argument 'opacity' must be between 0 and 1", view_set_opacity, 0, 1.0001 )
expect_error( "NaN", "argument 'opacity'", view_set_opacity, 0, 0 / 0 )
expect_error( "infinity", "argument 'opacity'", view_set_opacity, 0, math.huge )
expect_error( "string", "argument 'opacity'", view_set_opacity, 0, "0.5" )
expect_error( "table", "argument 'opacity'", view_set_opacity, 0, {} )
expect_error( "missing", "argument 'opacity'", view_set_opacity, 0 )
expect_error( "unknown view", "unknown view 99", view_set_opacity, 99, 0.5 )
expect_error( "unknown view, get", "unknown view 99", view_get_opacity, 99 )
check( view_get_opacity( 0 ) == 0.4, "a refused call changed the opacity" )

-- a destroyed view
local doomed = view_create( renderer, 400, 20, 200, 200 )

check( view_destroy( doomed ) == true, "view_destroy failed" )
expect_error( "destroyed view", "unknown view " .. tostring( doomed ), view_get_opacity, doomed )
expect_error( "destroyed view, set", "unknown view " .. tostring( doomed ), view_set_opacity, doomed, 0.5 )

-- frames drawn faded: the default view at 0.5 over the frame, the extra one at 0
check( view_set_opacity( 0, 0.5 ) == true and view_set_opacity( extra, 0 ) == true, "set for drawing failed" )

local frames = 0

function on_update( dt )
    if reported then return end           -- a failed run never quits: --max-frames runs out (exit 3)

    frames = frames + 1

    if frames < __DRAW_FRAMES then return end

    check( view_get_opacity( 0 ) == 0.5 and view_get_opacity( extra ) == 0, "drawing changed a view's opacity" )
    check( view_set_opacity( 0, 1 ) == true and view_set_opacity( extra, 1 ) == true, "set back to 1 failed" )

    reported = true

    if #failures == 0 then
        print( "VIEW OPACITY SMOKE OK" )
        app_quit()
    else
        for _, f in ipairs( failures ) do print( "VIEW OPACITY SMOKE FAIL: " .. f ) end
    end
end
