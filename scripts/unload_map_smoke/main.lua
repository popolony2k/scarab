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
    tilemap_unload_map smoke test - run by scripts/unload_map_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 400 scripts/unload_map_smoke/project.json

    Pins plans/MASTER_PLAN.md item V9 (sunlight BREAKAGES 47 and 55): tilemap_unload_map unloads
    every sprite on a layer (no active sequence afterwards, sprite_set_active_sequence refuses it
    until it is configured again) and takes their colliders out, so an unloaded sprite never
    collides and a sprite added again after a new map loads collides exactly once per frame.
    Two overlapping sprites on layers 2 and 3 of resources/tilemap/test.tmx, a rule between those
    layers, and the collision handler's calls counted over PHASE_FRAMES frames per phase:
      1. map loaded, both sprites added          -> exactly one call per frame;
      2. after tilemap_unload_map                -> none (sunlight up to I5 still reported one per
                                                    frame; the published v0.1.25 segfaulted);
      3. map loaded again, sprites set up again  -> exactly one call per frame (up to I5: four,
                                                    since each collider was registered twice).

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __MAP          <const> = "resources/tilemap/test.tmx"
local __TEXTURE      <const> = "resources/sprites/sunny_idle_down.png"
local __FRAMES       <const> = 4      -- texture frames in sunny_idle_down.png
local __LAYER_A      <const> = 2      -- "background_clouds" in test.tmx
local __LAYER_B      <const> = 3      -- "background"
local __POS          <const> = 50     -- both sprites at ( __POS, __POS ): they overlap
local __PHASE_FRAMES <const> = 60
local __NO_SEQUENCE  <const> = -1     -- sprite_get_active_sequence of a sprite with no active sequence

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

local function setup( handle, layerId )
    check( sprite_configure_texture( handle, 0, __TEXTURE, __FRAMES, 0, TEXTURE_ANIMATION_MODE_MANUAL ) == true, "configure failed" )
    check( sprite_set_active_sequence( handle, 0 ) == true, "set_active_sequence failed" )
    check( sprite_set_pos( handle, __POS, __POS ) == true, "set_pos failed" )
    check( sprite_add_to_layer( handle, layerId ) == true, "add_to_layer " .. layerId .. " failed" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, __MAP .. " failed to load" )
check( pool_register_type( "sunny", 2 ) == true, "pool_register_type failed" )

local a, b = sprite_acquire( "sunny" ), sprite_acquire( "sunny" )

setup( a, __LAYER_A )
setup( b, __LAYER_B )
check( collision_add_rule( __LAYER_A, __LAYER_B ) == true, "collision_add_rule failed" )

local hits   = 0
local frames = 0

check( collision_set_handler( function() hits = hits + 1 end ) == true, "collision_set_handler failed" )

local function expect_hits( phase, expected )
    check( hits == expected, string.format( "phase %s: %d collision calls in %d frames, expected %d", phase, hits, __PHASE_FRAMES, expected ) )
    hits = 0
end

function on_update( dt )
    frames = frames + 1

    if frames == __PHASE_FRAMES then
        expect_hits( "1 (map loaded)", __PHASE_FRAMES )
        check( tilemap_unload_map() == true, "tilemap_unload_map failed" )

        -- every sprite on a layer was unloaded: no active sequence, and not usable until configured again
        check( sprite_get_active_sequence( a ) == __NO_SEQUENCE, "after unload, active sequence is " .. tostring( sprite_get_active_sequence( a ) ) )

        local ok, err = sprite_set_active_sequence( a, 0 )

        check( ok == nil and tostring( err ):find( "is not configured", 1, true ), "after unload, set_active_sequence gave " .. tostring( ok ) .. " " .. tostring( err ) )
    elseif frames == 2 * __PHASE_FRAMES then
        expect_hits( "2 (map unloaded)", 0 )
        check( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, __MAP .. " failed to load again" )
        setup( a, __LAYER_A )
        setup( b, __LAYER_B )
    elseif frames == 3 * __PHASE_FRAMES then
        expect_hits( "3 (map loaded again, sprites added again)", __PHASE_FRAMES )

        if #failures == 0 then
            print( "UNLOAD MAP SMOKE OK" )
            app_quit()
        else
            for _, message in ipairs( failures ) do
                print( "UNLOAD MAP SMOKE FAIL: " .. message )
            end
        end
    end
end
