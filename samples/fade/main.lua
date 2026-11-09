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
 fade - view opacity (docs/lua-api/views.html, "View opacity"): a CROSS-FADE
 between two places of one map, and a MENU drawn over the map at a chosen
 opacity.

 Two views cover the same rectangle: the default view shows the map's top-left
 corner, the "scene" view (drawn above it) its bottom-right one. F fades the
 scene view in or out over one second (view_set_opacity, 0 -> 1 or back), so
 one place dissolves into the other. M opens or closes the menu: a smaller
 view in the middle showing the whole map, at 85% opacity, with the menu's
 text over it; [ and ] change its opacity.

 Kepi, Scarab's mascot, walks with the arrows; it is a world-space sprite, so
 each view draws it where that view shows its map position. A faded view's
 finished image fades, not each thing in it, and at 0 a view draws nothing,
 though its sprites keep animating.

 A controller works too, the first one connected: the D-pad or the left stick
 walks, A fades, Y opens the menu, L1/R1 change the menu's opacity.
]]

-- sp_* command: see samples/hello-world/docs/README.md for why every entry
-- script needs at least one, even one with nothing to sequence.
sp_wait( 1 )

local Hud = dofile( "samples/shared/hud.lua" )

local __RENDER_WIDTH    <const> = 960
local __RENDER_HEIGHT   <const> = 640
local __MARGIN          <const> = 10      -- pixels around the default view
local __ZOOM            <const> = 2.0     -- both scene views, so each shows a different part of the map
local __MENU            <const> = { x = 320, y = 160, w = 320, h = 320 }   -- square, like the map: it fits at zoom 0.5
local __MENU_OPACITY    <const> = 0.85    -- the menu's opacity when it opens
local __OPACITY_STEP    <const> = 0.05    -- what [ and ] change the menu's opacity by
local __FADE_SECONDS    <const> = 1.0     -- how long a cross-fade takes
local __MENU_ORDER      <const> = 10      -- the menu's draw order: above both scene views
local __KEPI            <const> = "resources/sprites/kepi_idle.png"
local __KEPI_FRAMES     <const> = 4
local __KEPI_DELAY      <const> = 100     -- ms per frame; without one the frame is held
local __KEPI_LAYER      <const> = 5       -- "houses" in resources/tilemap/test.tmx
local __SPEED           <const> = 3       -- map pixels per frame
local __STICK_DEAD      <const> = 0.5     -- how far the stick must lean to walk
local __MENU_TEXT_LEFT  <const> = 24      -- the menu's text, from its rectangle's corner
local __MENU_TEXT_TOP   <const> = 24
local __MENU_TEXT_STEP  <const> = 28
local __MENU_LINES      <const> = { "MENU", "Resume", "Options", "Quit" }

-- FIRST window-needing call (see samples/renderer for why it must be first).
local renderer = assert( renderer_create{
    title    = "Scarab - fade sample",
    width    = __RENDER_WIDTH,
    height   = __RENDER_HEIGHT,
    viewport = { x = __MARGIN, y = __MARGIN, w = __RENDER_WIDTH - 2 * __MARGIN, h = __RENDER_HEIGHT - 2 * __MARGIN },
    zoom     = __ZOOM,
} )

assert( tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT ) )

local mapTilesW, mapTilesH, tileW, tileH = assert( tilemap_get_map_info() )
local mapWidth, mapHeight = mapTilesW * tileW, mapTilesH * tileH
local viewX, viewY, viewW, viewH = viewport_get_dimension()
local shownW, shownH = viewW / __ZOOM, viewH / __ZOOM      -- how much of the map one scene view shows

-- The default view shows the map's top-left corner; the scene view, above it in the same rectangle, the
-- bottom-right one. It starts invisible (opacity 0): the default view's place shows.
assert( view_set_camera_position( 0, 0, 0 ) )

local scene = assert( view_create( renderer, viewX, viewY, viewW, viewH ) )

assert( view_set_zoom( scene, __ZOOM ) )
assert( view_set_camera_position( scene, math.floor( mapWidth - shownW ), math.floor( mapHeight - shownH ) ) )
assert( view_set_opacity( scene, 0 ) )

-- The menu: the whole map, smaller, above both; closed at first.
local menu = assert( view_create( renderer, __MENU.x, __MENU.y, __MENU.w, __MENU.h ) )

assert( view_fit_to_map( menu ) )
assert( view_set_draw_order( menu, __MENU_ORDER ) )
assert( view_set_background_color( menu, 0, 0, 0 ) )
assert( view_set_opacity( menu, __MENU_OPACITY ) )
assert( view_set_visible( menu, false ) )

-- Kepi, in map coordinates: every view draws it where it shows that point of the map.
assert( pool_register_type( "kepi", 1 ) )

local kepi = assert( sprite_acquire( "kepi" ) )

assert( sprite_configure_texture( kepi, 0, __KEPI, __KEPI_FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, __KEPI_DELAY ) )
assert( sprite_set_active_sequence( kepi, 0 ) )
assert( sprite_set_world_space( kepi, true ) )
assert( sprite_add_to_layer( kepi, __KEPI_LAYER ) )

local kepiW, kepiH = sprite_get_size( kepi )
local kepiX = math.floor( shownW / 2 - kepiW / 2 )   -- in the middle of what the default view shows
local kepiY = math.floor( shownH / 2 - kepiH / 2 )

local fadeFrom, fadeTo, fadeStart = 0, 0, nil   -- the scene view's cross-fade, while one runs
local held = {}                                   -- the controller's buttons held on the previous frame

-- Keep a value between low and high.
local function clamp( value, low, high )
    return math.max( low, math.min( high, value ) )
end

-- Whether a button of the first connected pad went down this frame (the gamepad functions only say whether a
-- button is down, so the state of the frame before is kept).
local function pressed( pad, button )
    local down = pad ~= nil and input_is_gamepad_button_down( pad, button ) == true
    local was  = held[button]

    held[button] = down

    return down and not was
end

-- Start fading the scene view in (if it is out) or out (if it is in), from its current opacity.
local function start_fade()
    fadeFrom  = view_get_opacity( scene )
    fadeTo    = fadeFrom < 1 and 1 or 0
    fadeStart = app_get_time()
end

-- Advance a running cross-fade by the time since it started (app_get_time, real seconds).
local function update_fade()
    if not fadeStart then return end

    local t = clamp( ( app_get_time() - fadeStart ) / __FADE_SECONDS, 0, 1 )

    assert( view_set_opacity( scene, fadeFrom + ( fadeTo - fadeFrom ) * t ) )

    if t >= 1 then fadeStart = nil end
end

-- Change the menu's opacity by `delta`, kept within 0..1.
local function change_menu_opacity( delta )
    assert( view_set_opacity( menu, clamp( view_get_opacity( menu ) + delta, 0, 1 ) ) )
end

-- Called by Scarab every frame (dt is a fixed step, not measured time): walk Kepi, run the cross-fade, open,
-- close and fade the menu, and draw the menu's text and the HUD.
function on_update( dt )

    local pads = input_get_connected_gamepads()
    local pad  = pads[1] and pads[1].id      -- the first connected controller, if any
    local sx   = pad and input_get_gamepad_axis( pad, GAMEPAD_AXIS_LEFT_X ) or 0
    local sy   = pad and input_get_gamepad_axis( pad, GAMEPAD_AXIS_LEFT_Y ) or 0

    -- Whether a key, or the same direction on the controller's D-pad, is held.
    local function down( key, button ) return input_is_key_down( key ) or ( pad ~= nil and input_is_gamepad_button_down( pad, button ) == true ) end

    if down( KEY_LEFT, GAMEPAD_BUTTON_LEFT_FACE_LEFT ) or sx < -__STICK_DEAD then kepiX = kepiX - __SPEED end
    if down( KEY_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT ) or sx > __STICK_DEAD then kepiX = kepiX + __SPEED end
    if down( KEY_UP, GAMEPAD_BUTTON_LEFT_FACE_UP ) or sy < -__STICK_DEAD then kepiY = kepiY - __SPEED end
    if down( KEY_DOWN, GAMEPAD_BUTTON_LEFT_FACE_DOWN ) or sy > __STICK_DEAD then kepiY = kepiY + __SPEED end

    kepiX = clamp( kepiX, 0, mapWidth - kepiW )
    kepiY = clamp( kepiY, 0, mapHeight - kepiH )
    assert( sprite_set_pos( kepi, kepiX, kepiY ) )

    if input_is_key_released( KEY_F ) or pressed( pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) then start_fade() end

    update_fade()

    if input_is_key_released( KEY_M ) or pressed( pad, GAMEPAD_BUTTON_RIGHT_FACE_UP ) then
        assert( view_set_visible( menu, not view_get_visible( menu ) ) )
    end

    if input_is_key_released( KEY_LEFT_BRACKET ) or pressed( pad, GAMEPAD_BUTTON_LEFT_TRIGGER_1 ) then change_menu_opacity( -__OPACITY_STEP ) end
    if input_is_key_released( KEY_RIGHT_BRACKET ) or pressed( pad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1 ) then change_menu_opacity( __OPACITY_STEP ) end

    -- the menu's text, over its view (text is drawn after every view, so it is not faded with it)
    if view_get_visible( menu ) then
        for index, line in ipairs( __MENU_LINES ) do
            Hud.line_at( line, __MENU.x + __MENU_TEXT_LEFT, __MENU.y + __MENU_TEXT_TOP + ( index - 1 ) * __MENU_TEXT_STEP,
                         index == 1 and Hud.ALERT or Hud.TITLE )
        end
    end

    Hud.line( "fade sample - arrows/D-pad walk Kepi", 1, Hud.TITLE )
    Hud.line( "F/A cross-fade   M/Y menu   [ ] or L1/R1 menu opacity", 2, Hud.TITLE )
    Hud.line( string.format( "default view %.2f   scene view %.2f   menu %.2f%s", view_get_opacity( 0 ), view_get_opacity( scene ),
                             view_get_opacity( menu ), view_get_visible( menu ) and "" or " (closed)" ), 3, Hud.INFO )
end
