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
 sprite - acquiring/configuring/moving a sprite (docs/lua-api/sprite.html),
 on top of the same map "tilemap" loads (resources/tilemap/).

 Demonstrates pool_register_type/sprite_acquire (the SpritePool CRUD
 pattern), sprite_configure_texture (a 4-frame auto-animating strip),
 sprite_add_to_layer (required before a sprite draws at all - see the
 layer-visibility gotcha in root CLAUDE.md), and keyboard-driven
 sprite_set_pos/sprite_get_pos/sprite_get_size, and a sprite's own
 opacity and tint (sprite_set_opacity/sprite_set_tint, keys O and T) - a
 second, untouched Sunny on the same texture shows they are the sprite's
 own, not its texture's.

 IMPORTANT - see root CLAUDE.md's own gotcha: a sprite added to a layer
 before the map is loaded is drawn right away, over an empty background,
 until the map appears (samples/mapless draws sprites with no map at
 all). This script loads its map first: tilemap_load_map is called
 synchronously, and every sprite call below runs strictly after it in
 the same top-level script.
]]

app_set_name( "Scarab - sprite sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_CENTER )

-- "sunny" is sunlight's own sample character (resources/sprites/, shared
-- from sunlight/samples/sprite/ - see resources/sprites/README.md), a
-- 128x32 strip of four 32x32 frames.
pool_register_type( "sunny", 2 )

local handle = sprite_acquire( "sunny" )
local twin   = sprite_acquire( "sunny" )   -- the same texture, never faded or tinted

-- The last argument is the frame delay in ms: WITHOUT it (the default, -1) the frame is held and sunny never animates.
sprite_configure_texture( handle, 0, "resources/sprites/sunny_idle_down.png", 4, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 100 )
sprite_set_active_sequence( handle, 0 )
-- World space: sunny's position is a MAP position in pixels, so the camera
-- can follow it across the whole map (by default a sprite's position is
-- relative to the view, and it would stay put on screen while the map moves).
sprite_set_world_space( handle, true )
sprite_set_pos( handle, 100, 100 )

-- Layer 4 ("clouds" in test.tmx) is visible by default (no visible="0")
-- - a sprite added to a layer authored invisible would never draw at
-- all, regardless of sprite_set_visible (see root CLAUDE.md).
sprite_add_to_layer( handle, 4 )

sprite_configure_texture( twin, 0, "resources/sprites/sunny_idle_down.png", 4, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 100 )
sprite_set_active_sequence( twin, 0 )
sprite_set_world_space( twin, true )
sprite_set_pos( twin, 52, 100 )    -- left of sunny, over the open clouds (to its right the house covers it)
sprite_add_to_layer( twin, 4 )

-- O cycles sunny's opacity (0 transparent .. 1 opaque), T its tint (r, g, b
-- multiply into the frame: white = none, a red hit flash, a blue team colour).
local OPACITIES = { 1, 0.5, 0.25, 0 }
local TINTS     = {
    { name = "none (white)",    r = 255, g = 255, b = 255 },
    { name = "red hit flash",   r = 255, g = 80,  b = 80 },
    { name = "blue team",       r = 120, g = 160, b = 255 },
}
local opacityIndex = 1
local tintIndex    = 1

local __SPEED = 3  -- pixels/frame, fine for a short demo - see CLAUDE.md's dt gotcha for why real games shouldn't scale movement by dt's fake fixed value either

-- The HUD's font: Caravellius 8x8, loaded once (the built-in font is hard to read at these sizes).
local hudFont = assert( load_font( "resources/fonts/caravellius8x8.fnt" ) )

local HUD_SIZE      = 16
local HUD_LINE      = 28    -- pixels between two HUD lines
local PLATE_PADDING = 4     -- pixels around a line of text on its plate
local PLATE_ALPHA   = 170   -- the plate's opacity, 0..255

local TITLE_COLOR = { r = 255, g = 255, b = 255 }
local INFO_COLOR  = { r = 255, g = 220, b = 120 }

-- A line of text on a dark translucent plate: the text crosses the white
-- clouds of the map, where light text alone can't be read.
local function draw_text_plate( text, line, color )
    local y = 20 + ( line - 1 ) * HUD_LINE

    draw_filled_rectangle( 20 - PLATE_PADDING, y - PLATE_PADDING,
        measure_text( text, HUD_SIZE, hudFont ) + 2 * PLATE_PADDING, HUD_SIZE + 2 * PLATE_PADDING, 0, 0, 0, PLATE_ALPHA )
    draw_text( text, 20, y, HUD_SIZE, color.r, color.g, color.b, 255, hudFont )
end

local mapWidth, mapHeight, tileWidth, tileHeight = tilemap_get_map_info()
local mapPixelWidth  = mapWidth * tileWidth
local mapPixelHeight = mapHeight * tileHeight

local function clamp( value, low, high )
    return math.max( low, math.min( value, high ) )
end

-- The camera follows sunny: it keeps sunny centred in the view, clamped so it
-- never shows anything past the map's edges (camera_set_position itself does
-- not clamp). The camera position is the map point shown at the view's
-- top-left; the view shows viewport size / zoom map pixels.
local function follow( x, y, width, height )
    local _, _, viewportWidth, viewportHeight = viewport_get_dimension()
    local zoom          = viewport_get_zoom_factor()
    local visibleWidth  = viewportWidth / zoom
    local visibleHeight = viewportHeight / zoom
    local cameraX = clamp( math.floor( x + width / 2 - visibleWidth / 2 ), 0, math.max( 0, math.floor( mapPixelWidth - visibleWidth ) ) )
    local cameraY = clamp( math.floor( y + height / 2 - visibleHeight / 2 ), 0, math.max( 0, math.floor( mapPixelHeight - visibleHeight ) ) )

    camera_set_position( cameraX, cameraY )

    return cameraX, cameraY
end

function on_update( dt )

    if input_is_key_released( KEY_O ) then
        opacityIndex = opacityIndex % #OPACITIES + 1
        sprite_set_opacity( handle, OPACITIES[opacityIndex] )
    end

    if input_is_key_released( KEY_T ) then
        tintIndex = tintIndex % #TINTS + 1
        local tint = TINTS[tintIndex]
        sprite_set_tint( handle, tint.r, tint.g, tint.b )
    end

    local x, y = sprite_get_pos( handle )

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then
        x = x - __SPEED
    end

    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then
        x = x + __SPEED
    end

    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then
        y = y - __SPEED
    end

    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then
        y = y + __SPEED
    end

    local width, height = sprite_get_size( handle )

    -- sunny stays on the map
    x = clamp( x, 0, mapPixelWidth - width )
    y = clamp( y, 0, mapPixelHeight - height )

    sprite_set_pos( handle, x, y )

    local cameraX, cameraY = follow( x, y, width, height )

    local r, g, b = sprite_get_tint( handle )

    draw_text_plate( "sprite sample - arrows/WASD move, the camera follows", 1, TITLE_COLOR )
    draw_text_plate( "O opacity, T tint - the twin on the left stays as it is", 2, TITLE_COLOR )
    draw_text_plate( "sprite_get_pos: " .. x .. ", " .. y .. "   sprite_get_size: " .. width .. "x" .. height, 3, INFO_COLOR )
    draw_text_plate( "camera_get_position: " .. cameraX .. ", " .. cameraY, 4, INFO_COLOR )
    draw_text_plate( string.format( "sprite_get_opacity: %g", sprite_get_opacity( handle ) ), 5, INFO_COLOR )
    draw_text_plate( string.format( "sprite_get_tint: %d, %d, %d (%s)", r, g, b, TINTS[tintIndex].name ), 6, INFO_COLOR )
end
