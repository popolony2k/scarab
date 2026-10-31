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
    Tile animation smoke test - run by scripts/tile_animation_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 400 scripts/tile_animation_smoke/project.json

    Pins plans/MASTER_PLAN.md item V13 (sunlight I11, BREAKAGES 44 and 60): a Tiled tile animation runs BY
    TIME - the frame shown is the time since tilemap_load_map, modulo the cycle, walked through the
    durations - read with tilemap_get_tile_frame / tilemap_get_object_tile_frame on
    resources/tilemap/animation/animation.tmx (one tile animating over gids 1 / 6 / 11 for
    100 / 50 / 300 ms, a 450 ms cycle; 50 ms is not a whole number of 60 fps frames).

    Every frame for several cycles, every copy of the animated tile - two cells, a flipped cell, a cell
    far to the right, a cell on a hidden layer, a tile object and a flipped tile object - must show
    EXACTLY the frame the virtual clock predicts, with no tolerance: a first frame shown twice, a
    stretched cycle, a frame held one draw too long or an animation that pauses while not drawn all
    fail. Halfway, the animated layer is hidden, masked out of the view and scrolled off screen for a
    while: the frame must still follow the time. A static tile and an all-0 ms animation never change.
    Every refusal is checked. What it LOOKS like is checked by eye with samples/shapes, key 6.

    The map is loaded at frame LOAD_FRAME, a virtual time that is not a whole millisecond, so the
    clock's rounding to milliseconds is exercised too.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every failure
    and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __MAP            <const> = "resources/tilemap/animation/animation.tmx"
local __ANIMATED_LAYER <const> = 1
local __HIDDEN_LAYER   <const> = 2
local __OBJECT_LAYER   <const> = 3
local __LOAD_FRAME     <const> = 7           -- 7/60 s = 116.67 ms: not a whole millisecond
local __SAMPLE_FRAMES  <const> = 300         -- 5 s, 11 cycles
local __HIDE_FROM      <const> = 100         -- frames after the load: the animated layer hidden, masked and off screen ...
local __HIDE_UNTIL     <const> = 180         -- ... until here
local __FAR_CAMERA_X   <const> = 900         -- map pixels: col 2 is far off screen

-- the animation of tile 0 (gid 1): { gid, duration in ms }
local __FRAMES         <const> = { { 1, 100 }, { 6, 50 }, { 11, 300 } }
local __CYCLE          <const> = 450
local __STATIC_GID     <const> = 16
local __ZERO_CYCLE_GID <const> = 4            -- all durations 0: shows its first frame (gid 4)

-- every cell that holds the animated tile: { row, col, layer, label }
local __ANIMATED_CELLS <const> = {
    { 2, 2,  __ANIMATED_LAYER, "cell 2,2" },
    { 2, 8,  __ANIMATED_LAYER, "cell 2,8 (same tile)" },
    { 2, 14, __ANIMATED_LAYER, "cell 2,14 (flipped)" },
    { 2, 36, __ANIMATED_LAYER, "cell 2,36 (far right)" },
    { 5, 2,  __HIDDEN_LAYER,   "cell 5,2 (hidden layer)" },
}
local __ANIMATED_OBJECTS <const> = { "anim_object", "anim_object_flipped" }

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

-- the virtual clock's milliseconds, as sunlight's VirtualClock rounds them (llround)
local function now_ms()
    return math.floor( app_get_time() * 1000 + 0.5 )
end

-- the gid the animation shows `elapsed` ms after the map loaded
local function expected_gid( elapsed )
    local inCycle = elapsed % __CYCLE

    for _, frame in ipairs( __FRAMES ) do
        if inCycle < frame[2] then return frame[1] end
        inCycle = inCycle - frame[2]
    end
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

local frames  = 0
local loadMs  = nil
local seen    = {}     -- gid -> number of sampled frames that showed it
local hidden  = false

local function check_frame( sample )
    local elapsed = now_ms() - loadMs
    local want    = expected_gid( elapsed )
    local where   = string.format( "sample %d (%d ms after the load)", sample, elapsed )

    for _, cell in ipairs( __ANIMATED_CELLS ) do
        local gid, err = tilemap_get_tile_frame( cell[1], cell[2], cell[3] )
        check( gid == want, string.format( "%s, %s: gid %s, expected %d %s", where, cell[4], tostring( gid ), want, err or "" ) )
    end

    for _, name in ipairs( __ANIMATED_OBJECTS ) do
        local gid, err = tilemap_get_object_tile_frame( name )
        check( gid == want, string.format( "%s, object %s: gid %s, expected %d %s", where, name, tostring( gid ), want, err or "" ) )
    end

    -- asking only reads: asking again gives the same answer
    check( tilemap_get_tile_frame( 2, 2, __ANIMATED_LAYER ) == want, where .. ": a second query differed" )
    check( tilemap_get_tile_frame( 2, 20, __ANIMATED_LAYER ) == __STATIC_GID, where .. ": the static tile changed" )
    check( tilemap_get_tile_frame( 2, 26, __ANIMATED_LAYER ) == __ZERO_CYCLE_GID, where .. ": the 0 ms cycle changed" )

    seen[want] = ( seen[want] or 0 ) + 1
end

local function set_hidden( hide )
    local visible, opacity, offsetX, offsetY = tilemap_get_layer( __ANIMATED_LAYER )

    check( tilemap_set_layer( __ANIMATED_LAYER, not hide, opacity, offsetX, offsetY ) == true, "tilemap_set_layer failed" )
    check( view_show_layer( 0, __ANIMATED_LAYER, not hide ) == true, "view_show_layer failed" )
    check( camera_set_position( hide and __FAR_CAMERA_X or 0, 0 ) == true, "camera_set_position failed" )
    hidden = hide
end

local function check_refusals()
    expect_error( "object layer", "is not a tile layer", tilemap_get_tile_frame( 0, 0, __OBJECT_LAYER ) )
    expect_error( "unknown layer", "no layer 99", tilemap_get_tile_frame( 0, 0, 99 ) )
    expect_error( "row -1", "is off the map", tilemap_get_tile_frame( -1, 2, __ANIMATED_LAYER ) )
    expect_error( "col 40", "is off the map", tilemap_get_tile_frame( 2, 40, __ANIMATED_LAYER ) )
    expect_error( "row 12", "is off the map", tilemap_get_tile_frame( 12, 2, __ANIMATED_LAYER ) )
    expect_error( "empty cell", "the cell is empty", tilemap_get_tile_frame( 0, 0, __ANIMATED_LAYER ) )
    expect_error( "row as a string", "argument 'row' must be an integer", tilemap_get_tile_frame( "2", 2, __ANIMATED_LAYER ) )
    expect_error( "fractional col", "argument 'col' must be an integer", tilemap_get_tile_frame( 2, 2.5, __ANIMATED_LAYER ) )
    expect_error( "missing layer", "argument 'layerId' must be an integer", tilemap_get_tile_frame( 2, 2 ) )
    expect_error( "unknown object", "no object named 'nope'", tilemap_get_object_tile_frame( "nope" ) )
    expect_error( "label object", "is not a tile object", tilemap_get_object_tile_frame( "not_a_tile" ) )
    expect_error( "object name as a number", "argument 'name' must be a string", tilemap_get_object_tile_frame( 1 ) )
end

function on_update( dt )
    frames = frames + 1

    if frames < __LOAD_FRAME then
        return
    end

    if frames == __LOAD_FRAME then
        expect_error( "no map", "no map is loaded", tilemap_get_tile_frame( 2, 2, __ANIMATED_LAYER ) )
        expect_error( "no map, object", "no map is loaded", tilemap_get_object_tile_frame( "anim_object" ) )

        check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, __MAP .. " failed to load" )
        loadMs = now_ms()
        check( loadMs % 1 == 0 and app_get_time() * 1000 ~= loadMs, "the load must not fall on a whole millisecond: " .. app_get_time() )
        check_refusals()
    end

    local sample = frames - __LOAD_FRAME

    if sample == __HIDE_FROM then set_hidden( true ) end
    if sample == __HIDE_UNTIL then set_hidden( false ) end

    check_frame( sample )

    if sample == __SAMPLE_FRAMES then
        -- not vacuous: every frame of the animation was seen, the 50 ms one included
        for _, frame in ipairs( __FRAMES ) do
            check( ( seen[frame[1]] or 0 ) > 0, "gid " .. frame[1] .. " was never seen" )
        end
        check( not hidden, "the layer is still hidden" )

        if #failures == 0 then
            print( "TILE ANIMATION SMOKE OK" )
            app_quit()
        else
            for i, f in ipairs( failures ) do
                if i > 40 then print( "TILE ANIMATION SMOKE FAIL: ... " .. ( #failures - 40 ) .. " more" ) break end
                print( "TILE ANIMATION SMOKE FAIL: " .. f )
            end
        end
    end
end
