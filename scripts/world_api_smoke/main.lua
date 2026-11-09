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
    World API smoke test - run by scripts/world_api_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/world_api_smoke/project.json

    Pins plans/MASTER_PLAN.md item C7, part 2 (sunlight I19b-f):
    - Before any window: no world exists, every world call is "unknown world N", and none creates the renderer -
      except world_create, which creates the default one (as tilemap_load_map does).
    - Lifetime: world_create / world_load (.tmx and .world) / world_unload, world_get_count, ids never reused, a
      failed load leaves no world, the default world cannot be unloaded.
    - Maps: world_add_map at offsets (negative too), world_remove_map, world_get_map_count / _offset / _bounds;
      map ids start at 0; a .world's maps in the file's order; all or nothing (a missing map); "patterns" refused.
    - Layers and tiles: world_layer_id by name, world_get_tile in the world's pixels, world_to_tile_matrix finding
      the map (an isometric one included); tilemap_get_map_info / tilemap_get_layer(_by_name) with a world.
    - Views: view_set_world / view_get_world; the default view refuses another world; an unloaded world's views go
      back to the default world.
    - Sprites: one world at a time (adding to another world is refused, naming the world), sprite_release detaches
      from any world (a recycled handle can join another), world_unload and tilemap_unload_map detach theirs.
    - Collisions, counted over frames: each world's rules test only its sprites, every hit carries its world id
      (sprite-to-tile and sprite-to-sprite), two worlds never collide, and a sprite over two maps of one world
      hits the obstacles of both (I19e).

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every failure and never
    quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __ORTHO        <const> = "resources/tilemap/orthogonal/orthogonal.tmx"
local __ISO          <const> = "resources/tilemap/isometric/isometric.tmx"
local __GRID         <const> = "resources/tilemap/worlds/grid.world"
local __MISSING      <const> = "scripts/world_api_smoke/missing.world"
local __PATTERNS     <const> = "scripts/world_api_smoke/patterns.world"
local __TEXTURE      <const> = "resources/sprites/kepi_idle.png"
local __GROUND       <const> = 1
local __OBSTACLES    <const> = 2
local __SPRITES      <const> = 3
local __SETTLE       <const> = 3          -- frames before counting, for the positions to take effect
local __HOLD         <const> = 5          -- frames counted
local __OBSTACLE_X   <const> = 96         -- obstacle (2, 3) of the orthogonal map
local __OBSTACLE_Y   <const> = 64

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

-- `got` (a table of results) must equal `want`, value by value
local function same( label, got, want )
    for i = 1, #want do
        if got[i] ~= want[i] then
            local shown = {}

            for j = 1, #want do shown[j] = tostring( got[j] ) end

            check( false, label .. ": " .. table.concat( shown, "," ) .. " - expected " .. table.concat( want, "," ) )
            return
        end
    end
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- before any window
check( world_get_count() == 0, "world_get_count before a renderer must be 0" )
expect_error( "unknown world before", "unknown world 0", world_get_map_count, 0 )
expect_error( "unload before", "unknown world 1", world_unload, 1 )
expect_error( "map info, a world before", "argument 'world': unknown world 5", tilemap_get_map_info, 5 )
check( renderer_get_current() == nil, "a world call refused before any window created the renderer" )

-- world_create creates the default renderer, as tilemap_load_map does
local first = world_create()

check( first ~= nil and first > 0, "world_create failed" )
check( select( 2, renderer_get_current() ) == "lazy_default", "world_create did not create the default renderer" )
check( world_get_count() == 2, "two worlds expected: the default one and the new one" )
expect_error( "an empty world's bounds", "has no map", world_get_bounds, first )
check( world_get_map_count( first ) == 0, "a new world has no map" )
check( tilemap_load_map( __ORTHO, MAP_ALIGNMENT_TOP_LEFT ) == true, "loading the default world's map failed" )

-- maps at offsets
same( "add_map", { world_add_map( first, __ORTHO, -32, 10 ) }, { 0 } )
same( "bounds, negative offset", { world_get_bounds( first ) }, { -32, 10, 256, 192 } )
same( "second map", { world_add_map( first, __ISO, 256, 0 ) }, { 1 } )
same( "offset", { world_get_map_offset( first, 1 ) }, { 256, 0 } )
same( "bounds of two", { world_get_bounds( first ) }, { -32, 0, 736, 224 } )
check( world_get_map_count( first ) == 2, "two maps expected" )
check( world_remove_map( first, 1 ) == true, "world_remove_map failed" )
expect_error( "remove twice", "no map 1 in world " .. first, world_remove_map, first, 1 )
expect_error( "offset of a removed map", "no map 1", world_get_map_offset, first, 1 )
expect_error( "add a missing map", "cannot load map no_such.tmx", world_add_map, first, "no_such.tmx", 0, 0 )
expect_error( "add_map x", "argument 'x' must be an integer", world_add_map, first, __ORTHO, 1.5, 0 )
expect_error( "add_map world", "expected a world handle", world_add_map, "w", __ORTHO, 0, 0 )
check( world_unload( first ) == true, "world_unload failed" )
expect_error( "an unloaded world", "unknown world " .. first, world_get_map_count, first )
expect_error( "the default world", "the default world cannot be unloaded", world_unload, 0 )

-- loading: .tmx, .world, all or nothing, ids never reused
local grid = world_load( __GRID )

check( grid ~= nil and grid > first, "world_load of the .world failed, or reused an id" )
check( world_get_map_count( grid ) == 4, "the grid world must have its file's 4 maps" )
same( "grid map 3", { world_get_map_offset( grid, 3 ) }, { 256, 192 } )
same( "grid bounds", { world_get_bounds( grid ) }, { 0, 0, 704, 416 } )
same( "layer ids", { world_layer_id( grid, "ground" ), world_layer_id( grid, "obstacles" ), world_layer_id( grid, "sprites" ) },
      { __GROUND, __OBSTACLES, __SPRITES } )
expect_error( "unknown layer", "no layer named 'nope' in world " .. grid, world_layer_id, grid, "nope" )
same( "pick map 0", { world_to_tile_matrix( grid, 16, 16 ) }, { 0, 0, 0 } )
same( "pick map 1", { world_to_tile_matrix( grid, 300, 20 ) }, { 1, 0, 1 } )
same( "pick the isometric map", { world_to_tile_matrix( grid, 448, 208 ) }, { 3, 0, 0 } )
expect_error( "pick outside", "is not in a cell of world " .. grid, world_to_tile_matrix, grid, 5000, 5000 )
same( "tile of the isometric map, in world pixels", { select( 2, world_get_tile( grid, 3, 0, 0, __GROUND ) ) }, { 416, 192, 64, 32 } )
same( "tile of map 1", { select( 2, world_get_tile( grid, 1, 2, 3, __OBSTACLES ) ) }, { 352, 64, 32, 32 } )
expect_error( "no tile", "no tile at row 0, col 0 of layer 2 in map 0", world_get_tile, grid, 0, 0, 0, __OBSTACLES )
same( "map info of a world", { tilemap_get_map_info( grid ) }, { 8, 6, 32, 32 } )
check( tilemap_get_layer_by_name( "obstacles", grid ) == true, "tilemap_get_layer_by_name with a world failed" )
check( tilemap_get_layer( __GROUND, grid ) == true, "tilemap_get_layer with a world failed" )
expect_error( "a layer of a world", "no layer 99", tilemap_get_layer, 99, grid )

local count = world_get_count()

expect_error( "missing map in a .world", "cannot load " .. __MISSING, world_load, __MISSING )
expect_error( "patterns", "cannot load " .. __PATTERNS, world_load, __PATTERNS )
expect_error( "missing .tmx", "cannot load nope.tmx", world_load, "nope.tmx" )
expect_error( "not a map", "must name a .tmx map or a .world file", world_load, "x.png" )
check( world_get_count() == count, "a failed world_load left a world behind" )

local single = world_load( __ISO )

check( single ~= nil and world_get_map_count( single ) == 1, "world_load of a .tmx failed" )
same( "a single map is map 0", { world_get_map_offset( single, 0 ) }, { 0, 0 } )

-- views
local renderer = renderer_get_current()
local view     = view_create( renderer, 20, 20, 300, 200 )

check( view_get_world( view ) == 0 and view_get_world( 0 ) == 0, "views start on the default world" )
check( view_set_world( view, single ) == true and view_get_world( view ) == single, "view_set_world failed" )
check( view_set_opacity( view, 0.5 ) == true, "a view showing another world keeps its own settings" )
expect_error( "default view", "the default view always shows the default world", view_set_world, 0, single )
check( view_set_world( 0, 0 ) == true, "the default view may be set to the default world" )
expect_error( "unknown world for a view", "argument 'world': unknown world 999", view_set_world, view, 999 )
check( world_unload( single ) == true, "world_unload of a shown world failed" )
check( view_get_world( view ) == 0, "an unloaded world's view must go back to the default world" )

-- sprites
check( pool_register_type( "kepi", 8 ) == true and pool_register_type( "solo", 1 ) == true, "pool_register_type failed" )

-- A configured, visible world-space sprite at (x, y), from the pool type `kind` ("kepi" unless given).
local function new_sprite( x, y, kind )
    local handle = sprite_acquire( kind or "kepi" )

    check( sprite_configure_texture( handle, 0, __TEXTURE, 4, 0, TEXTURE_ANIMATION_MODE_MANUAL ) == true, "configure failed" )
    check( sprite_set_active_sequence( handle, 0 ) == true and sprite_set_world_space( handle, true ) == true, "sprite setup failed" )
    check( sprite_set_pos( handle, x, y ) == true and sprite_set_visible( handle, true ) == true, "sprite placement failed" )

    return handle
end

local other   = world_load( __ORTHO )
local wander  = new_sprite( 0, 0, "solo" )       -- a one-slot type: releasing it and acquiring again reuses its slot

check( sprite_add_to_layer( wander, __SPRITES, other ) == true, "adding a sprite to another world failed" )
expect_error( "in another world", "the sprite is in world " .. other .. ": take it off its layer there first", sprite_add_to_layer, wander, __SPRITES )
expect_error( "not a layer of that world", "layer 99 is not a layer of the loaded map", sprite_add_to_layer, wander, 99, other )
expect_error( "unknown world for a sprite", "argument 'world': unknown world 999", sprite_add_to_layer, wander, __SPRITES, 999 )
expect_error( "removed from the wrong world", "the sprite is not on layer 3", sprite_remove_from_layer, wander, __SPRITES )
check( sprite_remove_from_layer( wander, __SPRITES, other ) == true, "removing from its world failed" )
check( sprite_add_to_layer( wander, __SPRITES ) == true, "after leaving its world it may join the default one" )
check( sprite_remove_from_layer( wander, __SPRITES ) == true, "removing from the default world failed" )

-- sprite_release detaches: a recycled handle can join another world
check( sprite_add_to_layer( wander, __SPRITES, other ) == true, "re-adding failed" )
check( sprite_release( wander ) == true, "sprite_release failed" )

local recycled = new_sprite( 0, 0, "solo" )

check( recycled ~= nil and recycled ~= wander, "the solo slot must come back with a new handle" )

check( sprite_add_to_layer( recycled, __SPRITES ) == true, "a released sprite stayed in its world: the recycled handle was refused" )
check( sprite_remove_from_layer( recycled, __SPRITES ) == true, "removing the recycled sprite failed" )

-- world_unload detaches its sprites
local doomed = world_load( __ORTHO )

check( sprite_add_to_layer( recycled, __SPRITES, doomed ) == true, "adding to a doomed world failed" )
check( world_unload( doomed ) == true, "world_unload failed" )
check( sprite_add_to_layer( recycled, __SPRITES ) == true, "a sprite of an unloaded world must be free to join another" )
check( sprite_remove_from_layer( recycled, __SPRITES ) == true, "removing after world_unload failed" )

-- collisions: one world `other` (rules for tiles and sprite pairs), the default world (tiles only), and a world of
-- two maps where one sprite overlaps an obstacle of each
local edge = world_create()

check( world_add_map( edge, __ORTHO, 0, 0 ) == 0 and world_add_map( edge, __ORTHO, 96, 32 ) == 1, "the edge world's maps failed" )

local tileOther   = new_sprite( __OBSTACLE_X, __OBSTACLE_Y )      -- on obstacle (2, 3) of `other`
local pairOther   = new_sprite( __OBSTACLE_X, __OBSTACLE_Y )      -- on the ground layer of `other`, over tileOther
local tileDefault = new_sprite( __OBSTACLE_X, __OBSTACLE_Y )      -- on obstacle (2, 3) of the default world
local lonely      = new_sprite( __OBSTACLE_X, __OBSTACLE_Y )      -- the default world's ground: no pair rule there
local overEdge    = new_sprite( 176, 96 )                          -- over obstacle (3, 5) of map 0 and (2, 3) of map 1

check( sprite_add_to_layer( tileOther, __SPRITES, other ) == true and sprite_add_to_layer( pairOther, __GROUND, other ) == true and
       sprite_add_to_layer( tileDefault, __SPRITES ) == true and sprite_add_to_layer( lonely, __GROUND ) == true and
       sprite_add_to_layer( overEdge, __SPRITES, edge ) == true, "adding the collision sprites failed" )
check( collision_add_tile_rule( __SPRITES, __OBSTACLES ) == true and collision_add_tile_rule( __SPRITES, __OBSTACLES, other ) == true and
       collision_add_tile_rule( __SPRITES, __OBSTACLES, edge ) == true, "the tile rules failed" )
check( collision_add_rule( __GROUND, __SPRITES, other ) == true, "the pair rule failed" )
expect_error( "a rule in an unknown world", "argument 'world': unknown world 999", collision_add_rule, __GROUND, __SPRITES, 999 )

local tiles, pairs_ = {}, {}

check( collision_set_tile_handler( function( handle, gid, x, y, w, h, world )
    tiles[#tiles + 1] = { handle = handle, x = x, world = world }
end ) == true, "collision_set_tile_handler failed" )

check( collision_set_handler( function( a, b, world )
    pairs_[#pairs_ + 1] = { a = a, b = b, world = world }
end ) == true, "collision_set_handler failed" )

-- What the counted frames must have seen.
local function check_collisions()
    local byKey = {}

    for _, hit in ipairs( tiles ) do
        local key = tostring( hit.handle ) .. "@" .. tostring( hit.world )

        byKey[key] = ( byKey[key] or 0 ) + 1

        if hit.handle == overEdge then byKey["edge x" .. hit.x] = ( byKey["edge x" .. hit.x] or 0 ) + 1 end
    end

    check( byKey[tostring( tileOther ) .. "@" .. other] == __HOLD, "tile hits in world " .. other .. ": " .. tostring( byKey[tostring( tileOther ) .. "@" .. other] ) )
    check( byKey[tostring( tileDefault ) .. "@0"] == __HOLD, "tile hits in the default world: " .. tostring( byKey[tostring( tileDefault ) .. "@0"] ) )
    check( byKey[tostring( overEdge ) .. "@" .. edge] == 2 * __HOLD, "tile hits over a map edge: " .. tostring( byKey[tostring( overEdge ) .. "@" .. edge] ) )
    check( byKey["edge x160"] == __HOLD and byKey["edge x192"] == __HOLD, "the edge sprite must hit one obstacle of each map" )
    check( #tiles == 4 * __HOLD, "unexpected tile hits: " .. #tiles )

    for _, hit in ipairs( pairs_ ) do
        local ok = hit.world == other and ( ( hit.a == pairOther and hit.b == tileOther ) or ( hit.a == tileOther and hit.b == pairOther ) )

        check( ok, "unexpected sprite hit: " .. tostring( hit.a ) .. ", " .. tostring( hit.b ) .. " in world " .. tostring( hit.world ) )
    end

    check( #pairs_ == __HOLD, "sprite hits in world " .. other .. ": " .. #pairs_ )
end

local frame = 0

function on_update( dt )
    if reported then return end           -- a failed run never quits: --max-frames runs out (exit 3)

    frame = frame + 1

    if frame == __SETTLE then
        tiles, pairs_ = {}, {}
        return
    end

    if frame < __SETTLE + __HOLD then return end

    check_collisions()

    -- tilemap_unload_map detaches the default world's sprites: one may then join another world
    check( tilemap_unload_map() == true, "tilemap_unload_map failed" )
    check( sprite_add_to_layer( tileDefault, __SPRITES, other ) == true, "a sprite of the unloaded default map must be free to join another world" )

    reported = true

    if #failures == 0 then
        print( "WORLD API SMOKE OK" )
        app_quit()
    else
        for _, f in ipairs( failures ) do print( "WORLD API SMOKE FAIL: " .. f ) end
    end
end
