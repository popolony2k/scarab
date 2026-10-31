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

local MAP          = "resources/tilemap/test.tmx"
local TEXTURE      = "resources/sprites/sunny_idle_down.png"
local FRAMES       = 4      -- texture frames in sunny_idle_down.png
local LAYER_A      = 2      -- "background_clouds" in test.tmx
local LAYER_B      = 3      -- "background"
local POS          = 50     -- both sprites at ( POS, POS ): they overlap
local PHASE_FRAMES = 60
local NO_SEQUENCE  = -1     -- sprite_get_active_sequence of a sprite with no active sequence

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

local function setup( handle, layerId )
    check( sprite_configure_texture( handle, 0, TEXTURE, FRAMES, 0, TEXTURE_ANIMATION_MODE_MANUAL ) == true, "configure failed" )
    check( sprite_set_active_sequence( handle, 0 ) == true, "set_active_sequence failed" )
    check( sprite_set_pos( handle, POS, POS ) == true, "set_pos failed" )
    check( sprite_add_to_layer( handle, layerId ) == true, "add_to_layer " .. layerId .. " failed" )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

check( tilemap_load_map( MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, MAP .. " failed to load" )
check( pool_register_type( "sunny", 2 ) == true, "pool_register_type failed" )

local a, b = sprite_acquire( "sunny" ), sprite_acquire( "sunny" )

setup( a, LAYER_A )
setup( b, LAYER_B )
check( collision_add_rule( LAYER_A, LAYER_B ) == true, "collision_add_rule failed" )

local hits   = 0
local frames = 0

check( collision_set_handler( function() hits = hits + 1 end ) == true, "collision_set_handler failed" )

local function expect_hits( phase, expected )
    check( hits == expected, string.format( "phase %s: %d collision calls in %d frames, expected %d", phase, hits, PHASE_FRAMES, expected ) )
    hits = 0
end

function on_update( dt )
    frames = frames + 1

    if frames == PHASE_FRAMES then
        expect_hits( "1 (map loaded)", PHASE_FRAMES )
        check( tilemap_unload_map() == true, "tilemap_unload_map failed" )

        -- every sprite on a layer was unloaded: no active sequence, and not usable until configured again
        check( sprite_get_active_sequence( a ) == NO_SEQUENCE, "after unload, active sequence is " .. tostring( sprite_get_active_sequence( a ) ) )

        local ok, err = sprite_set_active_sequence( a, 0 )

        check( ok == nil and tostring( err ):find( "is not configured", 1, true ), "after unload, set_active_sequence gave " .. tostring( ok ) .. " " .. tostring( err ) )
    elseif frames == 2 * PHASE_FRAMES then
        expect_hits( "2 (map unloaded)", 0 )
        check( tilemap_load_map( MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, MAP .. " failed to load again" )
        setup( a, LAYER_A )
        setup( b, LAYER_B )
    elseif frames == 3 * PHASE_FRAMES then
        expect_hits( "3 (map loaded again, sprites added again)", PHASE_FRAMES )

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
