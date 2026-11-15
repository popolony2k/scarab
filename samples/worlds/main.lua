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
 worlds - several maps on screen at once (docs/lua-api/worlds.html): Scarab's
 counterpart of sunlight's own "worlds" sample, on the shared maps.

 The screen is split in two. The LEFT half is the default view, which always
 shows the default world: the cloud map (resources/tilemap/test.tmx), loaded
 with tilemap_load_map. The RIGHT half is an extra view showing another world:
 - key 1: the isometric map, loaded with world_load into a world of its own;
 - key 2: a grid of four maps from a Tiled .world file
   (resources/tilemap/worlds/grid.world: three orthogonal maps and an
   isometric one), where Kepi walks across the maps' edges and the obstacles
   of every map block him (a tile collision rule of that world only).
 Each world has its own Kepi: WASD walk the left one, the arrows the right one.
 They never meet - worlds are isolated. Every view shows its whole world
 (view_fit_to_map, again whenever it changes world).

 T (X on a controller) tints the grid world's "ground" layer: one call, and
 all four maps change (a layer setter given a world reaches every map of it).

 F cross-fades the left half from the cloud map to the isometric world and
 back: an overlay view showing the isometric world, at opacity 0 -> 1. U
 unloads the isometric world and loads it again: while it is gone, the views
 that showed it show the default world, and its Kepi waits, detached.

 A controller works too, the first one connected: the left stick walks the left
 Kepi, the D-pad the right one; A cross-fades, Y switches the right world, B
 unloads or reloads.
]]

-- sp_* command: see samples/hello-world/docs/README.md for why every entry
-- script needs at least one, even one with nothing to sequence.
sp_wait( 1 )

local Hud = dofile( "samples/shared/hud.lua" )

local __RENDER_WIDTH  <const> = 1280
local __RENDER_HEIGHT <const> = 800
local __MARGIN        <const> = 10       -- pixels around the views, and between them
local __MARGINS       <const> = 3        -- across the window: left, middle and right
local __HUD_HEIGHT    <const> = 168      -- the HUD's five rows sit above the views
local __KEPIS         <const> = 3        -- one per world: clouds, isometric, grid
local __CLOUDS        <const> = "resources/tilemap/test.tmx"
local __ISO           <const> = "resources/tilemap/isometric/isometric.tmx"
local __GRID          <const> = "resources/tilemap/worlds/grid.world"
local __CLOUDS_LAYER  <const> = 5        -- "houses" in test.tmx: Kepi walks over the houses
local __KEPI          <const> = "resources/sprites/kepi_idle.png"
local __KEPI_FRAMES   <const> = 4
local __KEPI_DELAY    <const> = 100      -- ms per frame; without one the frame is held
local __KEPI_SIZE     <const> = 32       -- kepi_idle.png: 32x32 frames
local __CLOUD_START   <const> = { 160, 160 }   -- where each Kepi starts, in its world's pixels
local __ISO_START     <const> = { 208, 96 }
local __GRID_START    <const> = { 16, 16 }
local __SPEED         <const> = 2        -- map pixels per frame
local __STICK_DEAD    <const> = 0.5      -- how far the stick must lean to walk
local __FADE_SECONDS  <const> = 1.0
local __OVERLAY_ORDER <const> = 10       -- the overlay's draw order: above the left half
local __TINT          <const> = { 255, 150, 150, 255 }   -- T: the grid world's ground, reddened in every map
local __NO_TINT       <const> = { 255, 255, 255, 255 }

local halfWidth = ( __RENDER_WIDTH - __MARGINS * __MARGIN ) // 2
local viewH     = __RENDER_HEIGHT - __HUD_HEIGHT - __MARGIN
local left      = { x = __MARGIN, y = __HUD_HEIGHT, w = halfWidth, h = viewH }
local right     = { x = 2 * __MARGIN + halfWidth, y = __HUD_HEIGHT, w = halfWidth, h = viewH }

-- FIRST window-needing call (see samples/renderer for why it must be first): the default view is the left half.
local renderer = assert( renderer_create{
    title    = "Scarab - worlds sample",
    width    = __RENDER_WIDTH,
    height   = __RENDER_HEIGHT,
    viewport = { x = left.x, y = left.y, w = left.w, h = left.h },
} )

assert( tilemap_load_map( __CLOUDS, MAP_ALIGNMENT_TOP_LEFT ) )

local iso  = assert( world_load( __ISO ) )
local grid = assert( world_load( __GRID ) )

-- The right half, and the overlay over the left half that the cross-fade fades in.
local rightView = assert( view_create( renderer, right.x, right.y, right.w, right.h ) )
local overlay   = assert( view_create( renderer, left.x, left.y, left.w, left.h ) )

-- Every view shows its whole world (view_fit_to_map: the zoom and camera that fit every map of it), again each
-- time it changes world.
assert( view_set_world( rightView, iso ) )
assert( view_set_world( overlay, iso ) )
assert( view_fit_to_map( 0 ) and view_fit_to_map( rightView ) and view_fit_to_map( overlay ) )
assert( view_set_draw_order( overlay, __OVERLAY_ORDER ) )
assert( view_set_background_color( overlay, 0, 0, 0 ) )   -- opaque: the isometric map has no background colour of its own,
                                                         -- and at opacity 1 nothing of the cloud map may show around it
assert( view_set_opacity( overlay, 0 ) )

assert( pool_register_type( "kepi", __KEPIS ) )

-- A Kepi: a world-space sprite on `layer` of `world` (the default world when nil), at (x, y).
local function new_kepi( layer, world, x, y )
    local kepi = { handle = assert( sprite_acquire( "kepi" ) ), layer = layer, world = world, x = x, y = y, prevX = x, prevY = y }

    assert( sprite_configure_texture( kepi.handle, 0, __KEPI, __KEPI_FRAMES, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, __KEPI_DELAY ) )
    assert( sprite_set_active_sequence( kepi.handle, 0 ) )
    assert( sprite_set_world_space( kepi.handle, true ) )
    assert( sprite_add_to_layer( kepi.handle, layer, world ) )
    assert( sprite_set_pos( kepi.handle, x, y ) )

    return kepi
end

local cloudKepi = new_kepi( __CLOUDS_LAYER, nil, __CLOUD_START[1], __CLOUD_START[2] )
local isoKepi   = new_kepi( assert( world_layer_id( iso, "sprites" ) ), iso, __ISO_START[1], __ISO_START[2] )
local gridKepi  = new_kepi( assert( world_layer_id( grid, "sprites" ) ), grid, __GRID_START[1], __GRID_START[2] )

-- The grid world's obstacles block its Kepi: a rule of that world only, so the other worlds' Kepis walk freely.
assert( collision_add_tile_rule( gridKepi.layer, assert( world_layer_id( grid, "obstacles" ) ), grid ) )

local showGrid    = false        -- what the right half shows: the isometric world, or the grid world
local gridTinted  = false        -- T: whether the grid world's ground layer is tinted
local fadeFrom, fadeTo, fadeStart = 0, 0, nil
local blockedFrame = -1          -- the frame the grid Kepi was last pushed back (once per frame)
local frame        = 0
local hits         = 0           -- tile hits in the grid world, for the HUD
local held         = {}          -- the controller's buttons held on the previous frame

-- A tile hit: only the grid world has a tile rule, so `world` is always it - checked anyway, as a game would.
-- The grid Kepi's whole move this frame is undone, once, so he stops at the obstacle.
assert( collision_set_tile_handler( function( handle, gid, x, y, width, height, world )
    if world ~= grid or handle ~= gridKepi.handle then return end

    hits = hits + 1

    if blockedFrame ~= frame then
        gridKepi.x, gridKepi.y = gridKepi.prevX, gridKepi.prevY
        assert( sprite_set_pos( gridKepi.handle, gridKepi.x, gridKepi.y ) )
        blockedFrame = frame
    end
end ) )

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

-- Move a Kepi by (dx, dy) map pixels, kept inside its world's bounds (`bounds` = x, y, w, h).
local function walk( kepi, dx, dy, bx, by, bw, bh )
    kepi.prevX, kepi.prevY = kepi.x, kepi.y
    kepi.x = clamp( kepi.x + dx, bx, bx + bw - __KEPI_SIZE )
    kepi.y = clamp( kepi.y + dy, by, by + bh - __KEPI_SIZE )
    assert( sprite_set_pos( kepi.handle, kepi.x, kepi.y ) )
end

-- Unload the isometric world (its Kepi is detached and waits), or load it again and put its Kepi back.
local function toggle_iso()
    if iso then
        assert( world_unload( iso ) )   -- the views showing it fall back to the default world
        iso = nil
    else
        iso = assert( world_load( __ISO ) )
        assert( sprite_add_to_layer( isoKepi.handle, isoKepi.layer, iso ) )
        isoKepi.world = iso
        assert( view_set_world( overlay, iso ) )

        if not showGrid then assert( view_set_world( rightView, iso ) ) end
    end

    assert( view_fit_to_map( rightView ) and view_fit_to_map( overlay ) )
end

-- Show the isometric world (or the grid world) on the right.
local function show_on_right( grid_ )
    showGrid = grid_
    assert( view_set_world( rightView, showGrid and grid or ( iso or 0 ) ) )
    assert( view_fit_to_map( rightView ) )
end

-- Tint the grid world's "ground" layer, or take the tint off: one call, and every map of the world that has a
-- layer of that name changes (sunlight I21), so the four maps stay alike.
local function toggle_grid_tint()
    local tint = gridTinted and __NO_TINT or __TINT

    gridTinted = not gridTinted
    assert( tilemap_set_layer_tint_by_name( "ground", tint[1], tint[2], tint[3], tint[4], grid ) )
end

-- Start fading the overlay (the isometric world over the left half) in, or out.
local function start_fade()
    fadeFrom  = view_get_opacity( overlay )
    fadeTo    = fadeFrom < 1 and 1 or 0
    fadeStart = app_get_time()
end

-- Advance a running cross-fade by the real time since it started.
local function update_fade()
    if not fadeStart then return end

    local t = clamp( ( app_get_time() - fadeStart ) / __FADE_SECONDS, 0, 1 )

    assert( view_set_opacity( overlay, fadeFrom + ( fadeTo - fadeFrom ) * t ) )

    if t >= 1 then fadeStart = nil end
end

-- Called by Scarab every frame (dt is a fixed step, not measured time): walk the Kepis, switch, fade, unload or
-- reload worlds, keep each view's camera on its Kepi, and draw the HUD.
function on_update( dt )

    frame = frame + 1

    local pads = input_get_connected_gamepads()
    local pad  = pads[1] and pads[1].id      -- the first connected controller, if any
    local sx   = pad and input_get_gamepad_axis( pad, GAMEPAD_AXIS_LEFT_X ) or 0
    local sy   = pad and input_get_gamepad_axis( pad, GAMEPAD_AXIS_LEFT_Y ) or 0

    -- Whether a key, or a D-pad direction of the controller, is held.
    local function down( key, button ) return input_is_key_down( key ) or ( pad ~= nil and input_is_gamepad_button_down( pad, button ) == true ) end

    -- the left Kepi: WASD or the left stick
    local lx = ( input_is_key_down( KEY_A ) or sx < -__STICK_DEAD ) and -__SPEED or ( input_is_key_down( KEY_D ) or sx > __STICK_DEAD ) and __SPEED or 0
    local ly = ( input_is_key_down( KEY_W ) or sy < -__STICK_DEAD ) and -__SPEED or ( input_is_key_down( KEY_S ) or sy > __STICK_DEAD ) and __SPEED or 0
    local mapW, mapH, tileW, tileH = assert( tilemap_get_map_info() )

    walk( cloudKepi, lx, ly, 0, 0, mapW * tileW, mapH * tileH )

    -- the right Kepi: the arrows or the D-pad, in the world the right half shows
    local rx = down( KEY_LEFT, GAMEPAD_BUTTON_LEFT_FACE_LEFT ) and -__SPEED or down( KEY_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT ) and __SPEED or 0
    local ry = down( KEY_UP, GAMEPAD_BUTTON_LEFT_FACE_UP ) and -__SPEED or down( KEY_DOWN, GAMEPAD_BUTTON_LEFT_FACE_DOWN ) and __SPEED or 0
    local rightKepi  = showGrid and gridKepi or isoKepi
    local rightWorld = showGrid and grid or iso

    if rightWorld then
        local bx, by, bw, bh = assert( world_get_bounds( rightWorld ) )

        walk( rightKepi, rx, ry, bx, by, bw, bh )
    end

    if input_is_key_released( KEY_ONE ) then show_on_right( false ) end
    if input_is_key_released( KEY_TWO ) then show_on_right( true ) end
    if pressed( pad, GAMEPAD_BUTTON_RIGHT_FACE_UP ) then show_on_right( not showGrid ) end
    if input_is_key_released( KEY_F ) or pressed( pad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) then start_fade() end
    if input_is_key_released( KEY_U ) or pressed( pad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT ) then toggle_iso() end
    if input_is_key_released( KEY_T ) or pressed( pad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT ) then toggle_grid_tint() end

    update_fade()

    Hud.line( "worlds sample   WASD/left stick: left Kepi   arrows/D-pad: right Kepi", 1, Hud.TITLE )
    Hud.line( "1/2 or Y: right world   F or A: cross-fade   U or B: unload/reload", 2, Hud.TITLE )
    Hud.line( string.format( "worlds %d   left view: world %d   right view: world %d   overlay %.2f", world_get_count(),
                             view_get_world( 0 ), view_get_world( rightView ), view_get_opacity( overlay ) ), 3, Hud.INFO )
    Hud.line( string.format( "grid world: %d maps, obstacle hits %d   isometric world: %s", world_get_map_count( grid ), hits,
                             iso and "loaded" or "unloaded" ), 4, Hud.INFO )
    Hud.line( "T or X: tint the grid world's ground (every map)   grid tint " .. ( gridTinted and "on" or "off" ), 5, Hud.TITLE )
end
