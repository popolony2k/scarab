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
 projections - the four Tiled map styles: isometric, staggered, hexagonal
 and orthogonal (docs/lua-api/tilemap.html, "Tiles and map projections"),
 on sunlight's own maps (resources/tilemap/isometric/, staggered/,
 hexagonal/) and Scarab's orthogonal map of the same layout
 (resources/tilemap/orthogonal/).

 1-4 pick a map. Kepi, Scarab's mascot, stands on a cell; the arrow keys move
 Kepi one row or column, and the obstacles block the way. The camera follows
 Kepi. Kepi's cell is outlined in white (its rectangle from tilemap_get_tile,
 the cell's diamond, hexagon or square drawn with shape_draw_polygon).

 A controller works too, the first one connected: the D-pad moves Kepi,
 the left stick the cursor, L1/R1 pick the previous/next map, L2/R2 zoom
 and Y resets the zoom.

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
local __STICK_STEP    <const> = 3       -- map pixels the cursor moves per frame with the stick pushed all the way
local __ROUND         <const> = 0.5     -- added before math.floor, to round to the nearest whole pixel
local __OUTLINE       <const> = { thickness = 1.5 }
local __VIEW          <const> = 0       -- the default view
local __OPAQUE        <const> = 255
local __WHITE         <const> = { 255, 255, 255 }
local __YELLOW        <const> = { 255, 220, 60 }
local __MAPS          <const> = {       -- the maps 1-4 pick, their cell's outline, and a free cell to start on
    { key = KEY_ONE,   name = "isometric",  outline = "diamond", start = { 0, 0 } },
    { key = KEY_TWO,   name = "staggered",  outline = "diamond", start = { 0, 0 } },
    { key = KEY_THREE, name = "hexagonal",  outline = "hexagon", start = { 0, 0 } },
    { key = KEY_FOUR,  name = "orthogonal", outline = "square",  start = { 0, 0 } },
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

-- The outline of a cell's footprint, relative to its top-left: a diamond, a pointy-top hexagon or a square.
local function outline_points( w, h )
    if map.outline == "hexagon" then
        local edge = ( h - hexSide ) / 2

        return { w / 2, 0, w, edge, w, h - edge, w / 2, h, 0, h - edge, 0, edge }
    elseif map.outline == "square" then
        return { 0, 0, w, 0, w, h, 0, h }
    end

    return { w / 2, 0, w, h / 2, w / 2, h, 0, h / 2 }
end

-- The map keys, each with its map's name; the loaded one in brackets.
local function map_labels()
    local labels = {}

    for index, entry in ipairs( __MAPS ) do
        local label = index .. " " .. entry.name

        labels[#labels + 1] = entry == map and "[" .. label .. "]" or label
    end

    return table.concat( labels, "   " )
end

-- Outline a cell's footprint (its diamond, hexagon or square) in map units, in the given { r, g, b } colour.
local function draw_cell( row, col, color )
    local x, y, w, h = footprint( row, col )

    assert( shape_draw_polygon( x, y, outline_points( w, h ), color[1], color[2], color[3], __OPAQUE, __OUTLINE ) )
end

-- Whether Kepi may not step onto a cell: outside the map, or a cell with a tile on the obstacles layer.
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

-- Load one of __MAPS: read its size, set the preferred zoom on the first load, put Kepi on the new map's sprites
-- layer at the map's start cell, centre the cursor on that cell and centre the camera on Kepi.
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

-- The controller's buttons held on the previous frame, to act once per press (the gamepad functions only say
-- whether a button is down).
local held = {}

-- Whether a button of the first connected pad went down this frame.
local function pressed( pad, button )
    local down = pad ~= nil and input_is_gamepad_button_down( pad, button ) == true
    local was  = held[button]

    held[button] = down

    return down and not was
end

-- Load the map `step` places after the loaded one in __MAPS, wrapping around.
local function cycle_map( step )
    for index, entry in ipairs( __MAPS ) do
        if entry == map then
            load_map( __MAPS[( index - 1 + step ) % #__MAPS + 1] )
            return
        end
    end
end

-- Move Kepi by dRow rows and dCol columns, unless the cell there is blocked (then say so on the HUD).
local function try_move( dRow, dCol )
    local row, col = kepiRow + dRow, kepiCol + dCol

    if is_blocked( row, col ) then
        message = string.format( "row %d, col %d is blocked", row, col )
        return
    end

    kepiRow, kepiCol = row, col
    message = ""
end

-- Called by Scarab every frame (dt is a fixed step, not measured time): read the keyboard and the first
-- controller (map switch, Kepi's move, the cursor, zoom), keep the camera on Kepi, draw both cell outlines and
-- the cursor, and the HUD.
function on_update( dt )

    local pads = input_get_connected_gamepads()
    local pad  = pads[1] and pads[1].id      -- the first connected controller, if any

    for _, entry in ipairs( __MAPS ) do
        if input_is_key_released( entry.key ) then load_map( entry ) end
    end

    if pressed( pad, GAMEPAD_BUTTON_LEFT_TRIGGER_1 ) then cycle_map( -1 ) end
    if pressed( pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1 ) then cycle_map( 1 ) end

    -- Kepi: one cell per key or D-pad press
    if input_is_key_released( KEY_UP ) or pressed( pad, GAMEPAD_BUTTON_LEFT_FACE_UP ) then try_move( -1, 0 ) end
    if input_is_key_released( KEY_DOWN ) or pressed( pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN ) then try_move( 1, 0 ) end
    if input_is_key_released( KEY_LEFT ) or pressed( pad, GAMEPAD_BUTTON_LEFT_FACE_LEFT ) then try_move( 0, -1 ) end
    if input_is_key_released( KEY_RIGHT ) or pressed( pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT ) then try_move( 0, 1 ) end

    -- the cursor: free, in whole map pixels (the stick's dead zone is applied for us)
    if input_is_key_down( KEY_W ) then cursorY = cursorY - __CURSOR_STEP end
    if input_is_key_down( KEY_S ) then cursorY = cursorY + __CURSOR_STEP end
    if input_is_key_down( KEY_A ) then cursorX = cursorX - __CURSOR_STEP end
    if input_is_key_down( KEY_D ) then cursorX = cursorX + __CURSOR_STEP end

    if pad then
        cursorX = cursorX + math.floor( input_get_gamepad_axis( pad, GAMEPAD_AXIS_LEFT_X ) * __STICK_STEP + __ROUND )
        cursorY = cursorY + math.floor( input_get_gamepad_axis( pad, GAMEPAD_AXIS_LEFT_Y ) * __STICK_STEP + __ROUND )
    end

    -- zoom, as in samples/camera
    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )
    local zoomIn  = pad ~= nil and input_is_gamepad_button_down( pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2 ) == true
    local zoomOut = pad ~= nil and input_is_gamepad_button_down( pad, GAMEPAD_BUTTON_LEFT_TRIGGER_2 ) == true

    if zoomIn or input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or ( shift and input_is_key_down( KEY_EQUAL ) ) then zoom_in() end
    if zoomOut or input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then zoom_out() end
    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) or input_is_key_released( KEY_HOME )
       or pressed( pad, GAMEPAD_BUTTON_RIGHT_FACE_UP ) then
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
    Hud.line( map_labels(), 2, Hud.TITLE )
    Hud.line( "arrows/D-pad move Kepi   WASD/left stick move the cursor", 3, Hud.TITLE )
    Hud.line( "PageUp/PageDown or L2/R2 zoom   = or Y reset zoom   L1/R1 previous/next map", 4, Hud.TITLE )
    Hud.line( string.format( "map %dx%d cells of %dx%d   zoom %.2f", cols, rows, tileWidth, tileHeight, viewport_get_zoom_factor() ), 5, Hud.INFO )
    Hud.line( string.format( "Kepi: row %d, col %d   tile rectangle %d, %d, %dx%d", kepiRow, kepiCol, x, y, w, h ), 6, Hud.INFO )
    Hud.line( pickRow and string.format( "cursor %d, %d -> row %d, col %d", cursorX, cursorY, pickRow, pickCol )
                      or string.format( "cursor %d, %d -> no cell", cursorX, cursorY ), 7, Hud.INFO )
    Hud.line( message, 8, Hud.ALERT )
end
