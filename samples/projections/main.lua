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
 projections - isometric, staggered and hexagonal maps
 (docs/lua-api/tilemap.html, "Map projections"), on sunlight's own maps
 (resources/tilemap/isometric/, staggered/, hexagonal/).

 1-3 pick a map. Kepi, Scarab's mascot, stands on a cell; the arrow keys move
 Kepi one row or column, and the obstacles block the way. The camera follows
 Kepi. Kepi's cell is outlined in white (its rectangle from tilemap_get_tile,
 the cell's diamond or hexagon drawn with shape_draw_polygon).

 WASD move a cursor freely, in map pixels: tilemap_to_tile_matrix picks the
 cell under it, outlined in yellow, or says "no cell" where there is none (an
 isometric map's empty corners, or off the map).
]]

local Hud = dofile( "samples/shared/hud.lua" )

local __KEPI          <const> = "resources/sprites/kepi_idle.png"   -- Kepi, Scarab's mascot (temporary art)
local __KEPI_FRAMES   <const> = 4
local __KEPI_DELAY    <const> = 100     -- ms per frame; without one the frame is held
local __KEPI_SIZE     <const> = 32
local __KEPI_FEET     <const> = 28      -- pixels from the sprite's top to where Kepi stands
local __GROUND        <const> = 1       -- the maps' layers (see their READMEs)
local __OBSTACLES     <const> = 2
local __SPRITES       <const> = 3
local __CURSOR_STEP   <const> = 2       -- map pixels the cursor moves per frame while a key is held
local __CURSOR_SIZE   <const> = 3       -- the cursor point's size, in map pixels
local __OUTLINE       <const> = { thickness = 1.5 }
local __VIEW          <const> = 0       -- the default view
local __OPAQUE        <const> = 255
local __WHITE         <const> = { 255, 255, 255 }
local __YELLOW        <const> = { 255, 220, 60 }
local __MAPS          <const> = {       -- the maps 1-3 pick, and a free cell to start on
    { key = KEY_ONE,   name = "isometric", start = { 0, 0 } },
    { key = KEY_TWO,   name = "staggered", start = { 0, 0 } },
    { key = KEY_THREE, name = "hexagonal", start = { 0, 0 } },
}

assert( app_set_name( "Scarab - projections sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

assert( pool_register_type( "kepi", 1 ) )

local kepi = assert( sprite_acquire( "kepi" ) )

local map                  = nil      -- the loaded entry of __MAPS
local rows, cols           = 0, 0
local tileWidth, tileHeight = 0, 0
local hexSide              = 0        -- a hexagonal map's hexsidelength, read from its cells
local kepiRow, kepiCol     = 0, 0
local cursorX, cursorY     = 0, 0
local message              = ""
local preferredSet         = false

-- A cell's footprint - the part of its rectangle the cell covers: the bottom tileHeight pixels of the tile image.
local function footprint( row, col )
    local _, x, y, w, h = assert( tilemap_get_tile( row, col, __GROUND ) )

    return x, y + h - tileHeight, w, tileHeight
end

-- The outline of a cell's footprint, relative to its top-left: a diamond, or a pointy-top hexagon.
local function outline_points( w, h )
    if map.name == "hexagonal" then
        local edge = ( h - hexSide ) / 2

        return { w / 2, 0, w, edge, w, h - edge, w / 2, h, 0, h - edge, 0, edge }
    end

    return { w / 2, 0, w, h / 2, w / 2, h, 0, h / 2 }
end

local function draw_cell( row, col, color )
    local x, y, w, h = footprint( row, col )

    assert( shape_draw_polygon( x, y, outline_points( w, h ), color[1], color[2], color[3], __OPAQUE, __OUTLINE ) )
end

local function is_blocked( row, col )
    if row < 0 or col < 0 or row >= rows or col >= cols then return true end

    local gid = tilemap_get_tile( row, col, __OBSTACLES )

    return gid ~= nil and gid > 0
end

-- Stand Kepi on its cell (a world-space sprite: its position is a map position) and centre the camera on it.
local function place_kepi()
    local x, y, w, h = footprint( kepiRow, kepiCol )
    local feetX, feetY = x + w // 2, y + h // 2

    assert( sprite_set_pos( kepi, feetX - __KEPI_SIZE // 2, feetY - __KEPI_FEET ) )

    local zoom = viewport_get_zoom_factor()
    local _, _, viewWidth, viewHeight = viewport_get_dimension()

    assert( camera_set_position( math.max( 0, math.floor( feetX - viewWidth / zoom / 2 ) ),
                                 math.max( 0, math.floor( feetY - viewHeight / zoom / 2 ) ) ) )
end

local function load_map( entry )
    assert( tilemap_load_map( "resources/tilemap/" .. entry.name .. "/" .. entry.name .. ".tmx", MAP_ALIGNMENT_TOP_LEFT ) )

    map = entry
    cols, rows, tileWidth, tileHeight = assert( tilemap_get_map_info() )

    -- a hexagonal map's row step is tileHeight - (tileHeight - hexsidelength) / 2: read the side back from it
    local _, _, rowStep = assert( tilemap_get_tile( 1, 0, __GROUND ) )

    hexSide = 2 * rowStep - tileHeight

    -- zoom_reset returns to the PREFERRED zoom (see samples/camera): make it the starting one.
    if not preferredSet then
        assert( view_set_preferred_zoom( __VIEW, viewport_get_zoom_factor() ) )
        preferredSet = true
    end

    -- a loaded map keeps no sprites of the map before it: put Kepi on this one's sprites layer
    assert( sprite_configure_texture( kepi, 0, __KEPI, __KEPI_FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, __KEPI_DELAY ) )
    assert( sprite_set_active_sequence( kepi, 0 ) )
    assert( sprite_set_world_space( kepi, true ) )
    sprite_remove_from_layer( kepi, __SPRITES )   -- not on it yet the first time: the failure is expected
    assert( sprite_add_to_layer( kepi, __SPRITES ) )
    assert( sprite_set_visible( kepi, true ) )

    kepiRow, kepiCol = entry.start[1], entry.start[2]
    place_kepi()

    local x, y, w, h = footprint( kepiRow, kepiCol )

    cursorX, cursorY = x + w // 2, y + h // 2
    message = entry.name .. " loaded"
end

load_map( __MAPS[1] )

local function try_move( dRow, dCol )
    local row, col = kepiRow + dRow, kepiCol + dCol

    if is_blocked( row, col ) then
        message = string.format( "row %d, col %d is blocked", row, col )
        return
    end

    kepiRow, kepiCol = row, col
    message = ""
end

function on_update( dt )

    for _, entry in ipairs( __MAPS ) do
        if input_is_key_released( entry.key ) then load_map( entry ) end
    end

    -- Kepi: one cell per key press
    if input_is_key_released( KEY_UP ) then try_move( -1, 0 ) end
    if input_is_key_released( KEY_DOWN ) then try_move( 1, 0 ) end
    if input_is_key_released( KEY_LEFT ) then try_move( 0, -1 ) end
    if input_is_key_released( KEY_RIGHT ) then try_move( 0, 1 ) end

    -- the cursor: free, in map pixels
    if input_is_key_down( KEY_W ) then cursorY = cursorY - __CURSOR_STEP end
    if input_is_key_down( KEY_S ) then cursorY = cursorY + __CURSOR_STEP end
    if input_is_key_down( KEY_A ) then cursorX = cursorX - __CURSOR_STEP end
    if input_is_key_down( KEY_D ) then cursorX = cursorX + __CURSOR_STEP end

    -- zoom, as in samples/camera
    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    if input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or ( shift and input_is_key_down( KEY_EQUAL ) ) then zoom_in() end
    if input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then zoom_out() end
    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) or input_is_key_released( KEY_HOME ) then
        zoom_reset()
    end

    place_kepi()

    -- outlines: Kepi's cell, then the picked one
    draw_cell( kepiRow, kepiCol, __WHITE )

    local pickRow, pickCol = tilemap_to_tile_matrix( cursorX, cursorY )

    if pickRow then draw_cell( pickRow, pickCol, __YELLOW ) end

    assert( shape_draw_point( cursorX - __CURSOR_SIZE // 2, cursorY - __CURSOR_SIZE // 2, __YELLOW[1], __YELLOW[2], __YELLOW[3],
                              __OPAQUE, __CURSOR_SIZE ) )

    -- HUD
    local _, x, y, w, h = tilemap_get_tile( kepiRow, kepiCol, __GROUND )

    Hud.line( "projections sample - " .. map.name .. " map", 1, Hud.TITLE )
    Hud.line( "1-3 pick a map   arrows move Kepi   WASD move the cursor", 2, Hud.TITLE )
    Hud.line( "PageUp/PageDown zoom   = reset zoom", 3, Hud.TITLE )
    Hud.line( string.format( "map %dx%d cells of %dx%d   zoom %.2f", cols, rows, tileWidth, tileHeight, viewport_get_zoom_factor() ), 4, Hud.INFO )
    Hud.line( string.format( "Kepi: row %d, col %d   tile rectangle %d, %d, %dx%d", kepiRow, kepiCol, x, y, w, h ), 5, Hud.INFO )
    Hud.line( pickRow and string.format( "cursor %d, %d -> row %d, col %d", cursorX, cursorY, pickRow, pickCol )
                      or string.format( "cursor %d, %d -> no cell", cursorX, cursorY ), 6, Hud.INFO )
    Hud.line( message, 7, Hud.ALERT )
end
