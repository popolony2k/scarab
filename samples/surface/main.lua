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
 surface - shapes in map units (docs/lua-api/shape.html), Scarab's
 counterpart of sunlight's own "surface" sample.

 Every shape function, drawn each frame from on_update over the map
 "tilemap" loads: a thick square turning about its centre, an ellipse ring
 turning more slowly, a rectangle, a sine wave travelling along a thick
 polyline, and a row of points growing in size. A sprite walks back and
 forth with a red hitbox outline drawn around it. Shapes are in MAP units:
 they scroll and zoom with the camera, like the map's own tiles, and the
 viewport clips them; the sprite is in world space (sprite_set_world_space)
 so its position is a map position too, and the hitbox stays on it.

 M unloads the map (and loads it again): shapes follow the camera with or
 without a map (sunlight I16), so they stay where they were and only the
 map's tiles go; panning needs a map, so the camera keys then do nothing. The
 HUD lines and the bar at the bottom are screen space (draw_text,
 draw_filled_rectangle) for contrast: they never move.
]]

local Hud = dofile( "samples/shared/hud.lua" )

local __MAP           <const> = "resources/tilemap/test.tmx"
local __SPRITE_LAYER  <const> = 4                     -- "clouds" in test.tmx; with no map, just a grouping key
local __SUNNY         <const> = "resources/sprites/sunny_idle_down.png"
local __SUNNY_FRAMES  <const> = 4
local __SUNNY_SIZE    <const> = 32                    -- one frame, in map units
local __SUNNY_DELAY   <const> = 100                   -- ms per frame; without one the frame is held
local __WALK          <const> = { from = 40, to = 260, y = 200, speed = 1 }
local __OPAQUE        <const> = 255
local __SQUARE        <const> = { x = 60, y = 60, half = 20, thickness = 3, spin = 2 }
local __RING          <const> = { x = 110, y = 30, w = 70, h = 40, thickness = 2, spin = 1 }
local __BOX           <const> = { x = 210, y = 30, w = 60, h = 34, thickness = 1.5 }
local __WAVE          <const> = { x = 20, y = 130, count = 36, step = 8, height = 12, speed = 0.1, thickness = 1.5 }
local __POINTS        <const> = { x = 20, y = 165, count = 9, gap = 16, sizeStep = 0.75 }
local __HITBOX_THICK  <const> = 1
local __CYAN          <const> = { 0, 220, 255 }
local __ORANGE        <const> = { 255, 150, 0 }
local __GREEN         <const> = { 60, 220, 60 }
local __MAGENTA       <const> = { 255, 0, 255 }
local __YELLOW        <const> = { 255, 230, 0 }
local __RED           <const> = { 255, 40, 40 }
local __BAR_BACK      <const> = { 40, 40, 40 }
local __BAR_FILL      <const> = { 80, 160, 255 }
local __BAR           <const> = { x = 20, height = 14, margin = 30, cycle = 240 }   -- screen-space progress bar

assert( app_set_name( "Scarab - surface sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

assert( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) )

-- zoom_reset returns to the PREFERRED zoom, not the one the renderer starts at (see samples/camera):
-- make the starting zoom the one "=" and Home come back to.
assert( view_set_preferred_zoom( 0, viewport_get_zoom_factor() ) )
assert( pool_register_type( "sunny", 1 ) )

local mapLoaded = true
local sunny     = nil
local sunnyX    = __WALK.from
local direction = 1
local frame     = 0

-- (Re)create the walking sprite: unloading a map unloads its sprites, so it is set up again after each switch.
local function setup_sprite()
    if sunny then assert( sprite_release( sunny ) ) end

    sunny = assert( sprite_acquire( "sunny" ) )
    assert( sprite_configure_texture( sunny, 0, __SUNNY, __SUNNY_FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, __SUNNY_DELAY ) )
    assert( sprite_set_active_sequence( sunny, 0 ) )
    assert( sprite_set_world_space( sunny, true ) )     -- a MAP position, like the shapes
    assert( sprite_add_to_layer( sunny, __SPRITE_LAYER ) )
    assert( sprite_set_visible( sunny, true ) )
end

setup_sprite()

-- A square's corners about its centre, as a flat point list.
local function square_points( half )
    return { -half, -half, half, -half, half, half, -half, half }
end

-- A sine wave, as a flat point list relative to the wave's start.
local function wave_points( phase )
    local points = {}

    for i = 0, __WAVE.count - 1 do
        points[#points + 1] = i * __WAVE.step
        points[#points + 1] = __WAVE.height * math.sin( i * 0.5 + phase )   -- lint: allow lua-magic-number (the wave's spatial frequency)
    end

    return points
end

-- Every shape, in map units.
local function draw_shapes()
    assert( shape_draw_polygon( __SQUARE.x, __SQUARE.y, square_points( __SQUARE.half ), __CYAN[1], __CYAN[2], __CYAN[3], __OPAQUE,
                                { thickness = __SQUARE.thickness, rotation = frame * __SQUARE.spin } ) )
    assert( shape_draw_ellipse( __RING.x, __RING.y, __RING.w, __RING.h, __ORANGE[1], __ORANGE[2], __ORANGE[3], __OPAQUE,
                                { thickness = __RING.thickness, rotation = frame * __RING.spin } ) )
    assert( shape_draw_rectangle( __BOX.x, __BOX.y, __BOX.w, __BOX.h, __GREEN[1], __GREEN[2], __GREEN[3], __OPAQUE, { thickness = __BOX.thickness } ) )
    assert( shape_draw_polyline( __WAVE.x, __WAVE.y, wave_points( frame * __WAVE.speed ), __MAGENTA[1], __MAGENTA[2], __MAGENTA[3], __OPAQUE,
                                 { thickness = __WAVE.thickness } ) )

    for i = 1, __POINTS.count do
        assert( shape_draw_point( __POINTS.x + ( i - 1 ) * __POINTS.gap, __POINTS.y, __YELLOW[1], __YELLOW[2], __YELLOW[3], __OPAQUE, i * __POINTS.sizeStep ) )
    end

    -- the hitbox around the walking sprite: both are in map units
    assert( shape_draw_rectangle( sunnyX, __WALK.y, __SUNNY_SIZE, __SUNNY_SIZE, __RED[1], __RED[2], __RED[3], __OPAQUE, { thickness = __HITBOX_THICK } ) )
end

function on_update( dt )
    frame = frame + 1

    -- the sprite walks back and forth
    sunnyX = sunnyX + direction * __WALK.speed

    if sunnyX >= __WALK.to or sunnyX <= __WALK.from then direction = -direction end

    assert( sprite_set_pos( sunny, sunnyX, __WALK.y ) )

    -- camera and zoom, as in samples/camera (the camera calls fail harmlessly with no map loaded)
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then camera_move_up() end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then camera_move_down() end
    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then camera_move_left() end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then camera_move_right() end

    local shift = input_is_key_down( KEY_LEFT_SHIFT ) or input_is_key_down( KEY_RIGHT_SHIFT )

    if input_is_key_down( KEY_PAGE_UP ) or input_is_key_down( KEY_KP_ADD ) or ( shift and input_is_key_down( KEY_EQUAL ) ) then zoom_in() end
    if input_is_key_down( KEY_PAGE_DOWN ) or input_is_key_down( KEY_MINUS ) or input_is_key_down( KEY_KP_SUBTRACT ) then zoom_out() end
    if ( not shift and input_is_key_released( KEY_EQUAL ) ) or input_is_key_released( KEY_KP_EQUAL ) then zoom_reset() end

    if input_is_key_released( KEY_HOME ) then
        camera_reset()
        zoom_reset()
    end

    -- M: no map / the map again
    if input_is_key_released( KEY_M ) then
        if mapLoaded then
            assert( tilemap_unload_map() )
        else
            assert( tilemap_load_map( __MAP, MAP_ALIGNMENT_TOP_LEFT ) )
        end

        mapLoaded = not mapLoaded
        setup_sprite()
    end

    draw_shapes()

    -- HUD and a progress bar, in screen space: they never move
    local camX, camY = camera_get_position()

    Hud.line( "surface sample - shapes in map units: they scroll and zoom with the camera", 1, Hud.TITLE )
    Hud.line( "arrows/WASD pan   PageUp/PageDown or +/- zoom   = reset zoom   Home reset   M map on/off", 2, Hud.TITLE )
    Hud.line( string.format( "map %s   camera %d, %d   zoom %.2f", mapLoaded and "loaded" or "NOT loaded", camX or 0, camY or 0,
                             viewport_get_zoom_factor() ), 3, Hud.INFO )

    local barWidth = screen_get_width() - __BAR.x * 2
    local barY     = screen_get_height() - __BAR.margin

    assert( draw_filled_rectangle( __BAR.x, barY, barWidth, __BAR.height, __BAR_BACK[1], __BAR_BACK[2], __BAR_BACK[3], __OPAQUE ) )
    assert( draw_filled_rectangle( __BAR.x, barY, math.floor( barWidth * ( frame % __BAR.cycle ) / __BAR.cycle ), __BAR.height,
                                   __BAR_FILL[1], __BAR_FILL[2], __BAR_FILL[3], __OPAQUE ) )
end
