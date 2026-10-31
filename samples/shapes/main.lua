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
 shapes - Tiled shape objects (rectangle, ellipse, polyline, polygon),
 rotated objects and point objects (master plan item V6), on three maps made
 for them (resources/tilemap/shapes/, from sunlight's own samples/shapes - see
 resources/tilemap/shapes/README.md), plus per-object opacity, layer tint and tile animation. Keys 1 to
 6 switch maps:

   1. shapes.tmx   - the four shapes, and how the viewport clips them;
   2. rotation.tmx - shapes, tile objects and text objects with a Tiled
                     rotation, each drawn turned clockwise about its stored
                     point (the small square marked "pivot"). Before sunlight's
                     master plan the rotation was ignored;
   3. thick.tmx    - outlines with a line_width property and point objects
                     with a point_size; both scale with the zoom;
   4. opacity.tmx  - per-object opacity (master plan item V7, from
                     resources/tilemap/objects/): tile, shape and text objects
                     fading out, an object's own "opacity" property overriding
                     its layer's. Before sunlight's master plan, shape and text
                     objects ignored opacity altogether. Its frame starts at
                     (100, 100), so this map starts the camera there;
   5. tint.tmx     - Tiled layer tint (master plan item V12, sunlight I8, from
                     resources/tilemap/tint/): tile, image and object layers,
                     alpha ignored, shapes and text untinted, a group's tint and
                     opacity multiplying into its children. T and G change a
                     tint and the group's opacity at runtime.
   6. animation.tmx - Tiled tile animation by time (master plan item V13,
                     sunlight I11, from resources/tilemap/animation/): one tile
                     animating over three monkeys for 100 / 50 / 300 ms, in
                     several cells, flipped, far to the right and as tile
                     objects - all in step - next to a static tile and an
                     all-0 ms animation (its first frame). The HUD reads the
                     frame back with tilemap_get_tile_frame.

 Switching loads another map over the one already loaded (tilemap_load_map
 again), which sunlight no longer leaks (master plan item V10). The maps'
 "Sans" text objects get the project's own Caravellius 8x8, loaded ONCE and
 registered; the same handle draws this sample's HUD. The registration is
 the renderer's, not the map's, so it survives every switch.
]]

-- The layout sunlight's sample uses: a 1000 x 800 viewport at (40, 40),
-- zoom 1.0, the camera at the map's origin.
local __VIEWPORT      <const> = { x = 40, y = 40, w = 1000, h = 800 }
local __ORIGIN_CAMERA <const> = { x = 0, y = 0 }
local __FRAME_CAMERA  <const> = { x = 100, y = 100 }   -- where opacity.tmx's frame rectangle starts
local __SCROLL_STEP   <const> = 4    -- map pixels per frame while an arrow key is held

local __SHAPES_DIR  <const> = "resources/tilemap/shapes/"
local __OBJECTS_DIR <const> = "resources/tilemap/objects/"
local __TINT_DIR    <const> = "resources/tilemap/tint/"
local __ANIM_DIR    <const> = "resources/tilemap/animation/"
local __MAPS        <const> = {
    { key = KEY_ONE,   file = __SHAPES_DIR .. "shapes.tmx",    camera = __ORIGIN_CAMERA, title = "1 shapes: the four shapes, clipped by the viewport" },
    { key = KEY_TWO,   file = __SHAPES_DIR .. "rotation.tmx",  camera = __ORIGIN_CAMERA, title = "2 rotation: each object turned about its pivot" },
    { key = KEY_THREE, file = __SHAPES_DIR .. "thick.tmx",     camera = __ORIGIN_CAMERA, title = "3 thick: line widths and points, scaled by zoom" },
    { key = KEY_FOUR,  file = __OBJECTS_DIR .. "opacity.tmx",  camera = __FRAME_CAMERA,  title = "4 opacity: an object's own opacity, else its layer's" },
    { key = KEY_FIVE,  file = __TINT_DIR .. "tint.tmx",        camera = __ORIGIN_CAMERA, title = "5 tint: Tiled tintcolor, changed at runtime with T and G", tint = true },
    { key = KEY_SIX,   file = __ANIM_DIR .. "animation.tmx",   camera = __ORIGIN_CAMERA, title = "6 animation: one tile, 100 / 50 / 300 ms, every copy in step", animation = true },
}

local renderer, err = renderer_create{
    title          = "Scarab - shapes sample",
    viewport       = __VIEWPORT,
    zoom           = 1.0,
    preferred_zoom = 1.0,
    scroll_step    = { w = __SCROLL_STEP, h = __SCROLL_STEP },
}
assert( renderer, err )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

-- One font, one load: every map's "Sans" text objects and the HUD below.
local Hud = dofile( "samples/shared/hud.lua" )

assert( register_font( "Sans", Hud.font() ) )

local current

local function show_map( entry )
    assert( tilemap_load_map( entry.file, MAP_ALIGNMENT_TOP_LEFT ) )
    assert( zoom_reset() )
    assert( camera_set_position( entry.camera.x, entry.camera.y ) )
    current = entry
end

show_map( __MAPS[1] )



-- Map 5 (tint.tmx): T cycles the "tiles tinted red" layer's tint, G steps the group's opacity -
-- both at runtime, through tilemap_set_layer_tint / tilemap_set_layer (the map file is unchanged).
local __TINTED_LAYER <const> = 2       -- "tiles tinted cyan" in tint.tmx
local __GROUP_LAYER  <const> = 10      -- "group tinted blue, opacity 0.6"
local __TINTS        <const> = {       -- r, g, b, a; the last one is the layer's authored tint
    { 255, 255, 128, 255 }, { 128, 255, 128, 255 }, { 255, 255, 255, 255 }, { 128, 255, 255, 255 },
}
local __ANIMATED_LAYER <const> = 1       -- animation.tmx's "animated" tile layer (map 6)
local __ANIMATED_CELL  <const> = { row = 2, col = 2 }    -- an animated copy of the tile
local __STATIC_CELL    <const> = { row = 2, col = 20 }   -- the static tile next to it

local __GROUP_OPACITIES <const> = { 1, 0.25, 0.6 }   -- 0..1; 0.6 is the authored value
local tintIndex      = #__TINTS
local opacityIndex   = #__GROUP_OPACITIES

local function runtime_tint_keys()
    if input_is_key_released( KEY_T ) then
        tintIndex = tintIndex % #__TINTS + 1
        tilemap_set_layer_tint( __TINTED_LAYER, table.unpack( __TINTS[tintIndex] ) )
    end

    if input_is_key_released( KEY_G ) then
        local visible, _, offsetX, offsetY = tilemap_get_layer( __GROUP_LAYER )

        opacityIndex = opacityIndex % #__GROUP_OPACITIES + 1
        tilemap_set_layer( __GROUP_LAYER, visible, __GROUP_OPACITIES[opacityIndex], offsetX, offsetY )
    end
end

function on_update( dt )

    for _, entry in ipairs( __MAPS ) do
        if input_is_key_released( entry.key ) and entry ~= current then
            show_map( entry )
            tintIndex, opacityIndex = #__TINTS, #__GROUP_OPACITIES   -- a reload restores the authored values
        end
    end

    if current.tint then
        runtime_tint_keys()
    end

    -- Same mapping as samples/camera: the arrow moves the map's content that way.
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then camera_move_up() end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then camera_move_down() end
    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then camera_move_left() end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then camera_move_right() end

    -- The zoom keys every sunlight/Scarab sample shares (see samples/camera):
    -- Page Up/Page Down or + / - zoom, = resets the zoom, Home resets the zoom
    -- and the camera. + and = are one physical key, told apart by Shift.
    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    if input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or
       ( shift and input_is_key_down( KEY_EQUAL ) ) then
        zoom_in()
    end

    if input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then
        zoom_out()
    end

    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) then
        zoom_reset()
    end

    if input_is_key_released( KEY_HOME ) then
        zoom_reset()
        camera_set_position( current.camera.x, current.camera.y )
    end

    local camX, camY = camera_get_position()

    Hud.line_from_bottom( current.title, 3, Hud.TITLE )
    local mapLine = string.format( "camera %d, %d   zoom %.2f   keys 1-6 switch maps", camX, camY, viewport_get_zoom_factor() )

    if current.tint then
        local r, g, b = tilemap_get_layer_tint( __TINTED_LAYER )
        local _, groupOpacity = tilemap_get_layer( __GROUP_LAYER )

        mapLine = string.format( "keys 1-6 maps   T: layer 2 tint %d,%d,%d   G: group opacity %g", r, g, b, groupOpacity )
    end

    if current.animation then
        mapLine = string.format( "keys 1-6 maps   tilemap_get_tile_frame: gid %d   object gid %d   static gid %d",
            tilemap_get_tile_frame( __ANIMATED_CELL.row, __ANIMATED_CELL.col, __ANIMATED_LAYER ), tilemap_get_object_tile_frame( "anim_object" ),
            tilemap_get_tile_frame( __STATIC_CELL.row, __STATIC_CELL.col, __ANIMATED_LAYER ) )
    end

    Hud.line_from_bottom( mapLine, 2, Hud.INFO )
    Hud.line_from_bottom( "arrows/WASD pan   PageUp/PageDown or +/- zoom   = reset zoom   Home reset", 1, Hud.DIM )
end
