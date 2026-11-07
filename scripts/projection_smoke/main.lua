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
    Projection smoke test - run by scripts/projection_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 400 scripts/projection_smoke/project.json

    Pins plans/MASTER_PLAN.md item C5: Scarab's existing functions on sunlight's isometric, staggered and
    hexagonal maps (resources/tilemap/<name>/). For each map: the map info; exact cell rectangles
    (tilemap_get_tile, projection-aware); the four obstacle cells; points outside the map refused by
    tilemap_to_tile_matrix; the camera's scroll limits; view_fit_to_map's zoom; and a world-space sprite
    colliding with an obstacle (tile collisions use each shape's bounding box, so on the isometric map one
    sprite can touch several adjacent obstacles: the expected count is worked out here from the obstacles'
    collision rectangles) and not off the map.

    Picking (sunlight I17, pinned in P13): every cell's centre, and a point just inside the far tip of
    every cell's diamond or hexagon, picks that cell; a point inside the map's pixel rectangle but outside every
    cell (an isometric map's empty corner) picks none. Before I17, isometric and staggered centres picked a
    neighbour (0 of 48) and hexagon tips picked the next cell.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every failure and never
    quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __TEXTURE      <const> = "resources/sprites/kepi_idle.png"
local __FRAMES       <const> = 4
local __GROUND       <const> = 1
local __OBSTACLES    <const> = 2
local __SPRITES      <const> = 3
local __ROWS         <const> = 6
local __COLS         <const> = 8
local __CELLS        <const> = __ROWS * __COLS
local __SCROLL_TRIES <const> = 400         -- camera_move_* calls: far more than any of these maps needs
local __SPRITE_SIZE  <const> = 32
local __HOLD_FRAMES  <const> = 3           -- frames a sprite is held still while collisions are counted

-- What each map must give (measured at the default renderer: viewport 1240x900, zoom 3.8125). `shape` is the
-- obstacle tile's collision rectangle (its tileset's objectgroup, at the tile image's top-left); `empty` a point
-- inside the map's pixel rectangle but outside every cell.
local __MAPS <const> = {
    { name = "isometric", info = { 8, 6, 64, 32 },
      cells = { { 0, 0, 160, 0, 64, 32 }, { 1, 1, 160, 32, 64, 32 }, { 5, 7, 224, 192, 64, 32 } },
      obstacle = { 2, 3, 192, 80, 64, 48 }, shape = { 64, 32 }, scroll = { 122, 0 }, fit = 2.375, empty = { 5, 5 } },
    { name = "staggered", info = { 8, 6, 64, 48 },
      cells = { { 0, 0, 0, 0, 64, 48 }, { 1, 0, 32, 24, 64, 48 }, { 5, 7, 480, 120, 64, 48 } },
      obstacle = { 2, 3, 192, 48, 64, 48 }, shape = { 64, 48 }, scroll = { 218, 0 }, fit = 2.375 },
    { name = "hexagonal", info = { 8, 6, 60, 50 },
      cells = { { 0, 0, 0, 0, 60, 50 }, { 1, 0, 30, 35, 60, 50 }, { 5, 7, 450, 175, 60, 50 } },
      obstacle = { 2, 3, 180, 70, 60, 50 }, shape = { 60, 50 }, scroll = { 184, 0 }, fit = 2.5625 },
}

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

local function same( label, got, want )
    for i = 1, #want do
        if got[i] ~= want[i] then
            check( false, string.format( "%s: %s - expected %s", label, table.concat( got, "," ), table.concat( want, "," ) ) )
            return
        end
    end
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- The static checks of one map (it is loaded by the caller).
local function check_map( m )
    same( m.name .. " map info", { tilemap_get_map_info() }, m.info )

    for _, cell in ipairs( m.cells ) do
        local gid, x, y, w, h = tilemap_get_tile( cell[1], cell[2], __GROUND )

        check( gid ~= nil and gid > 0, m.name .. " cell " .. cell[1] .. "," .. cell[2] .. " has no ground tile" )
        same( m.name .. " cell " .. cell[1] .. "," .. cell[2] .. " rectangle", { x, y, w, h }, { cell[3], cell[4], cell[5], cell[6] } )
    end

    local obstacles = 0

    for r = 0, __ROWS - 1 do
        for c = 0, __COLS - 1 do
            local gid = tilemap_get_tile( r, c, __OBSTACLES )

            if gid and gid > 0 then obstacles = obstacles + 1 end
        end
    end

    check( obstacles == 4, m.name .. ": " .. obstacles .. " obstacle cells, expected 4" )

    local o = m.obstacle
    local gid, x, y, w, h = tilemap_get_tile( o[1], o[2], __OBSTACLES )

    check( gid ~= nil and gid > 0, m.name .. ": no obstacle at " .. o[1] .. "," .. o[2] )
    same( m.name .. " obstacle rectangle", { x, y, w, h }, { o[3], o[4], o[5], o[6] } )

    -- picking: points outside the map, and every cell's centre (see the header for isometric/staggered)
    check( tilemap_to_tile_matrix( -10, -10 ) == nil, m.name .. ": a point left of the map was picked" )
    check( tilemap_to_tile_matrix( 100000, 100000 ) == nil, m.name .. ": a point far past the map was picked" )

    local tileHeight = m.info[4]
    local centres    = 0
    local tips       = 0

    for r = 0, __ROWS - 1 do
        for c = 0, __COLS - 1 do
            local _, cx, cy, cw, ch = tilemap_get_tile( r, c, __GROUND )
            local midX   = cx + cw // 2
            local bottom = cy + ch                    -- the cell's footprint ends at the image's bottom
            local pr, pc = tilemap_to_tile_matrix( midX, bottom - tileHeight // 2 )

            if pr == r and pc == c then centres = centres + 1 end

            pr, pc = tilemap_to_tile_matrix( midX, bottom - 2 )   -- just inside the bottom tip

            if pr == r and pc == c then tips = tips + 1 end
        end
    end

    check( centres == __CELLS, string.format( "%s: %d of %d cell centres pick themselves", m.name, centres, __CELLS ) )
    check( tips == __CELLS, string.format( "%s: %d of %d cells' bottom tips pick themselves", m.name, tips, __CELLS ) )

    if m.empty then
        check( tilemap_to_tile_matrix( m.empty[1], m.empty[2] ) == nil, m.name .. ": a point outside every cell was picked" )
    end

    -- the camera's scroll limits, and view_fit_to_map
    for _ = 1, __SCROLL_TRIES do camera_move_right(); camera_move_down() end
    same( m.name .. " camera at its near limit", { camera_get_position() }, { 0, 0 } )

    for _ = 1, __SCROLL_TRIES do camera_move_left(); camera_move_up() end
    same( m.name .. " camera at its far limit", { camera_get_position() }, m.scroll )

    local zoom = viewport_get_zoom_factor()

    check( view_fit_to_map( 0 ) == true, m.name .. ": view_fit_to_map failed" )
    check( viewport_get_zoom_factor() == m.fit, m.name .. ": fit zoom " .. viewport_get_zoom_factor() .. ", expected " .. m.fit )
    check( view_set_zoom( 0, zoom ) == true, m.name .. ": restoring the zoom failed" )
end

-- a world-space sprite and a tile collision rule, shared by every map
check( pool_register_type( "kepi", 1 ) == true, "pool_register_type failed" )

local kepi  = sprite_acquire( "kepi" )
local calls = 0

check( collision_set_tile_handler( function( handle, gid, x, y, w, h ) calls = calls + 1 end ) == true, "collision_set_tile_handler failed" )

local ruleAdded = false     -- the tile rule is added once: rules survive loading another map, so adding it
                            -- for every map would count each hit once per copy

-- Load a map and put the sprite on its sprites layer (unloading a map unloads its sprites, loading another does not).
local function setup_map( m )
    check( tilemap_load_map( "resources/tilemap/" .. m.name .. "/" .. m.name .. ".tmx", MAP_ALIGNMENT_TOP_LEFT ) == true, m.name .. ": load failed" )
    check( camera_set_position( 0, 0 ) == true, m.name .. ": camera_set_position failed" )   -- TOP_LEFT keeps the previous camera
    check_map( m )
    check( sprite_configure_texture( kepi, 0, __TEXTURE, __FRAMES, 0, TEXTURE_ANIMATION_MODE_MANUAL ) == true, m.name .. ": configure failed" )
    check( sprite_set_active_sequence( kepi, 0 ) == true and sprite_set_world_space( kepi, true ) == true, m.name .. ": sprite setup failed" )
    sprite_remove_from_layer( kepi, __SPRITES )   -- may not be on it yet
    check( sprite_add_to_layer( kepi, __SPRITES ) == true and sprite_set_visible( kepi, true ) == true, m.name .. ": add_to_layer failed" )

    if not ruleAdded then      -- needs a loaded map: the rule checks its tile layer exists
        check( collision_add_tile_rule( __SPRITES, __OBSTACLES ) == true, "collision_add_tile_rule failed" )
        ruleAdded = true
    end
end

-- the frames: for each map, hold the sprite on an obstacle, then off the map, counting tile collisions
local index, step, held = 0, 0, 0
local reported           = false

-- How many obstacle tiles' collision rectangles a sprite rectangle overlaps (half-open, as sunlight tests them).
local function expected_hits( m, sx, sy )
    local hits = 0

    for r = 0, __ROWS - 1 do
        for c = 0, __COLS - 1 do
            local gid, x, y = tilemap_get_tile( r, c, __OBSTACLES )

            if gid and gid > 0 and sx < x + m.shape[1] and x < sx + __SPRITE_SIZE and sy < y + m.shape[2] and y < sy + __SPRITE_SIZE then
                hits = hits + 1
            end
        end
    end

    return hits
end

local expected = 0

local function next_map()
    index = index + 1
    step  = 0

    if __MAPS[index] then setup_map( __MAPS[index] ) end
end

next_map()

function on_update( dt )
    local m = __MAPS[index]

    if not m then
        if reported then return end       -- a failed run never quits: --max-frames runs out (exit 3)

        reported = true

        if #failures == 0 then
            print( "PROJECTION SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "PROJECTION SMOKE FAIL: " .. f ) end
        end

        return
    end

    local o = m.obstacle

    if step == 0 then           -- on the obstacle's centre
        local sx, sy = o[3] + o[5] // 2 - __SPRITE_SIZE // 2, o[4] + o[6] // 2 - __SPRITE_SIZE // 2

        expected = expected_hits( m, sx, sy )
        check( expected >= 1, m.name .. ": the sprite overlaps no obstacle shape" )
        check( sprite_set_pos( kepi, sx, sy ) == true, m.name .. ": set_pos failed" )
        step, held = 1, 0
    elseif step == 1 then       -- a frame for the move to take effect
        calls, step = 0, 2
    elseif step == 2 then
        held = held + 1

        if held == __HOLD_FRAMES then
            check( calls == expected * __HOLD_FRAMES, string.format( "%s: %d tile collisions over %d frames on the obstacle, expected %d",
                   m.name, calls, __HOLD_FRAMES, expected * __HOLD_FRAMES ) )
            check( sprite_set_pos( kepi, -__SPRITE_SIZE * 2, -__SPRITE_SIZE * 2 ) == true, m.name .. ": set_pos off the map failed" )
            step = 3
        end
    elseif step == 3 then
        calls, step, held = 0, 4, 0
    elseif step == 4 then
        held = held + 1

        if held == __HOLD_FRAMES then
            check( calls == 0, m.name .. ": " .. calls .. " tile collisions off the map, expected 0" )
            next_map()
        end
    end
end
