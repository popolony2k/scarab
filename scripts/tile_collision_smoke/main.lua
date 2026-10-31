--[[
    Tile collision smoke test - run by scripts/tile_collision_smoke.py (and ci.yml, every platform):

        ZOOM=<factor> scarab --headless --fast --max-frames 2000 scripts/tile_collision_smoke/project.json

    Pins plans/MASTER_PLAN.md item V11 (sunlight BREAKAGES 4 and 56): sprite-vs-tile collisions and
    tile lookups in MAP space, on collision.tmx next to this script (10x10 tiles of 32 px; collision
    tiles at (row, col) (5,5), (2,2)+(2,3), (8,1) with two shapes, and (9,9) in the far corner).
      - The lookups: tilemap_to_tile_matrix takes a map position; tilemap_get_tile returns the tile's
        map rectangle.
      - A 32x32 sprite swept along x past the (5,5) tile (map pixels 160..191) collides exactly for
        the x where it overlaps it: a view-relative sprite with the camera at 0 for x 129..191, with
        the camera at (64, 64) for view x 65..127; a world-space sprite for map x 129..191 whatever
        the camera, and never two rows away. The tile handed to the handler is its map rectangle
        (160, 160, 32, 32) with gid 1. Up to sunlight I6 only the tile under the sprite's top-left
        corner was tested, shifted by the viewport origin (x 150..180), world-space sprites got the
        camera added, and the tile came back in view space; and Scarab's handler never ran at all
        (it called the Lua function with the wrong argument count).
      - A sprite touching two collision tiles gets one call per tile per frame (2).
      - A tile with two collision shapes is reported (once per frame) - before I7 it hung the frame.
      - The far corner tile collides, and a sprite past the map's far edge collides with nothing.
    The Python driver runs it at zoom 1 and 3.8125: the windows don't depend on the zoom.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local MAP          = "scripts/tile_collision_smoke/collision.tmx"
local TEXTURE      = "resources/sprites/sunny_idle_down.png"
local FRAMES       = 4      -- texture frames in sunny_idle_down.png: a 32x32 sprite
local TILE_LAYER   = 1      -- "ground"
local SPRITE_LAYER = 2      -- "sprites"
local TILE         = 32     -- tile size, map pixels
local ZOOM         = tonumber( os.getenv( "ZOOM" ) or "1" )
local CAMERA       = 64     -- the camera offset of the camera phases, both axes
local SWEEP_FROM   = 0
local SWEEP_TO     = 260
local HOLD_FRAMES  = 10     -- frames a still sprite is held in the call-count phases

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

check( renderer_create{ zoom = ZOOM, preferred_zoom = ZOOM } ~= nil, "renderer_create failed" )

function on_load_stage( stageId ) return true end
sp_wait( 1 )

check( tilemap_load_map( MAP, MAP_ALIGNMENT_TOP_LEFT ) == true, MAP .. " failed to load" )

-- the lookups, in map space
local row, col = tilemap_to_tile_matrix( 0, 0 )
check( row == 0 and col == 0, "to_tile_matrix( 0, 0 ) = " .. tostring( row ) .. "," .. tostring( col ) )
row, col = tilemap_to_tile_matrix( 31, 31 )
check( row == 0 and col == 0, "to_tile_matrix( 31, 31 ) = " .. tostring( row ) .. "," .. tostring( col ) )
row, col = tilemap_to_tile_matrix( 32, 0 )
check( row == 0 and col == 1, "to_tile_matrix( 32, 0 ) = " .. tostring( row ) .. "," .. tostring( col ) )
row, col = tilemap_to_tile_matrix( 319, 319 )
check( row == 9 and col == 9, "to_tile_matrix( 319, 319 ) = " .. tostring( row ) .. "," .. tostring( col ) )
check( tilemap_to_tile_matrix( 320, 0 ) == nil, "to_tile_matrix( 320, 0 ) must be outside the map" )

local gid, tx, ty, tw, th = tilemap_get_tile( 5, 5, TILE_LAYER )
check( gid == 1 and tx == 160 and ty == 160 and tw == TILE and th == TILE,
       string.format( "get_tile( 5, 5 ) = %s %s,%s %sx%s - expected gid 1 at 160,160 32x32", gid, tx, ty, tw, th ) )

check( pool_register_type( "sunny", 1 ) == true, "pool_register_type failed" )

local sunny = sprite_acquire( "sunny" )

check( sprite_configure_texture( sunny, 0, TEXTURE, FRAMES, 0, TEXTURE_ANIMATION_MODE_MANUAL ) == true, "configure failed" )
check( sprite_set_active_sequence( sunny, 0 ) == true, "set_active_sequence failed" )
check( sprite_add_to_layer( sunny, SPRITE_LAYER ) == true, "add_to_layer failed" )
check( collision_add_tile_rule( SPRITE_LAYER, TILE_LAYER ) == true, "collision_add_tile_rule failed" )

local calls   = 0          -- handler calls since the last frame
local lastHit = nil        -- the last tile handed to the handler

check( collision_set_tile_handler( function( handle, tileGid, x, y, w, h )
    calls   = calls + 1
    lastHit = { handle = handle, gid = tileGid, x = x, y = y, w = w, h = h }
end ) == true, "collision_set_tile_handler failed" )

--[[
    The phases. A "sweep" moves the sprite one pixel per frame along x at a fixed y and records the
    first and last x that reported a collision (the sprite is set in on_update; collisions are
    worked out after it, the same frame, and read in the next on_update). A "hold" keeps the sprite
    still and counts the calls per frame.
]]
local phases = {
    { name = "view-relative, camera 0", world = false, camera = 0, y = 160, from = 129, to = 191 },
    { name = "world-space, camera 0", world = true, camera = 0, y = 160, from = 129, to = 191 },
    { name = "view-relative, camera 64", world = false, camera = CAMERA, y = 160 - CAMERA, from = 129 - CAMERA, to = 191 - CAMERA },
    { name = "world-space, camera 64", world = true, camera = CAMERA, y = 160, from = 129, to = 191 },
    { name = "world-space, camera 64, two rows away", world = true, camera = CAMERA, y = 160 - CAMERA, from = nil, to = nil },
    { name = "touching two tiles", world = true, camera = 0, hold = { x = 80, y = 64 }, perFrame = 2 },
    { name = "a tile with two shapes", world = true, camera = 0, hold = { x = 32, y = 256 }, perFrame = 1 },
    { name = "the far corner tile", world = true, camera = 0, hold = { x = 300, y = 300 }, perFrame = 1 },
    { name = "past the far edge", world = true, camera = 0, hold = { x = 330, y = 330 }, perFrame = 0 },
}

local phase    = 0
local current  = nil
local x        = nil
local first    = nil
local last     = nil
local held     = 0
local tileSeen = nil

local function start_phase()
    phase   = phase + 1
    current = phases[phase]

    if not current then return end

    check( sprite_set_world_space( sunny, current.world ) == true, current.name .. ": set_world_space failed" )
    check( camera_set_position( current.camera, current.camera ) == true, current.name .. ": camera_set_position failed" )

    first, last, held, tileSeen = nil, nil, 0, nil
    x = current.hold and current.hold.x or SWEEP_FROM

    check( sprite_set_pos( sunny, x, current.hold and current.hold.y or current.y ) == true, current.name .. ": set_pos failed" )
    calls = 0
end

local function finish_phase()
    if current.hold then
        return
    end

    check( first == current.from and last == current.to,
           string.format( "%s: collides for x %s..%s, expected %s..%s", current.name,
                          tostring( first ), tostring( last ), tostring( current.from ), tostring( current.to ) ) )

    if current.from then
        check( tileSeen and tileSeen.gid == 1 and tileSeen.x == 160 and tileSeen.y == 160 and tileSeen.w == TILE and tileSeen.h == TILE,
               current.name .. ": the tile handed to the handler is not gid 1 at map 160,160 32x32" )
        check( tileSeen and tileSeen.handle == sunny, current.name .. ": the handler got another handle" )
    end
end

local reported = false

function on_update( dt )
    if reported then return end

    if current == nil then
        start_phase()
        return
    end

    if current.hold then
        -- calls made during the previous frame, the sprite held still
        if held > 0 then
            check( calls == current.perFrame, string.format( "%s: %d calls in one frame, expected %d", current.name, calls, current.perFrame ) )
        end

        calls = 0
        held  = held + 1

        if held > HOLD_FRAMES then
            finish_phase()
            start_phase()
        end
    else
        if calls > 0 then
            first    = first or x
            last     = x
            tileSeen = lastHit
        end

        calls = 0
        x     = x + 1

        if x > SWEEP_TO then
            finish_phase()
            start_phase()
        else
            sprite_set_pos( sunny, x, current.y )
        end
    end

    if current == nil then
        reported = true

        if #failures == 0 then
            print( "TILE COLLISION SMOKE OK zoom=" .. ZOOM )
            app_quit()
        else
            for _, message in ipairs( failures ) do
                print( "TILE COLLISION SMOKE FAIL: " .. message )
            end
        end
    end
end
