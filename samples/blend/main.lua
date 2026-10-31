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
 blend - blend modes (docs/lua-api/blend.html), over the same map
 "tilemap" loads (resources/tilemap/ - see it's own README.md), modelled
 on sunlight's own samples/blend.

 Demonstrates the two ways to use a blend mode:
 - per sprite (sprite_set_blend_mode / sprite_clear_blend_mode): Sunny's
   glow, whose mode the number keys choose, and a soft shadow that always
   multiplies;
 - around what the script draws itself (draw_begin_blend_mode /
   draw_end_blend_mode): four panels, one per mode, drawn with
   draw_filled_rectangle.
]]

-- Zoom 1.0 (not Scarab's default 3.8125), so the whole map and the
-- 128x128 glow fit on screen next to the panels.
local renderer, err = renderer_create{ title = "Scarab - blend sample", zoom = 1.0, preferred_zoom = 1.0 }
assert( renderer, err )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local Hud = dofile( "samples/shared/hud.lua" )

assert( tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT ) )

local __GLOW_LAYER        <const> = 4     -- "clouds" in test.tmx: drawn before Sunny's layer
local __SUNNY_LAYER       <const> = 5     -- "houses"
local __SUNNY_SIZE        <const> = 32    -- one frame of sunny_idle_down.png
local __GLOW_SIZE         <const> = 128   -- glow.png
local __SPEED             <const> = 3     -- pixels/frame
local __SUNNY_START       <const> = { x = 250, y = 250 }
local __SHADOW_POS        <const> = { x = 60, y = 520 }
local __SUNNY_FRAMES      <const> = 4     -- sunny_idle_down.png: four frames side by side
local __SUNNY_FRAME_MILLI <const> = 100   -- per frame

-- Every asset here is in resources/sprites/ - see its README.md.
assert( pool_register_type( "sunny", 1 ) )
assert( pool_register_type( "glow", 1 ) )
assert( pool_register_type( "shadow", 1 ) )

local sunny  = assert( sprite_acquire( "sunny" ) )
local glow   = assert( sprite_acquire( "glow" ) )
local shadow = assert( sprite_acquire( "shadow" ) )

assert( sprite_configure_texture( sunny, 0, "resources/sprites/sunny_idle_down.png", __SUNNY_FRAMES, 0,
    TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, __SUNNY_FRAME_MILLI ) )
assert( sprite_set_active_sequence( sunny, 0 ) )
assert( sprite_configure_texture( glow, 0, "resources/sprites/glow.png", 1, 0, TEXTURE_ANIMATION_MODE_MANUAL, 0 ) )
assert( sprite_set_active_sequence( glow, 0 ) )
assert( sprite_configure_texture( shadow, 0, "resources/sprites/shadow.png", 1, 0, TEXTURE_ANIMATION_MODE_MANUAL, 0 ) )
assert( sprite_set_active_sequence( shadow, 0 ) )

-- The shadow always multiplies: it darkens what is under it. Its PNG's
-- transparent pixels are white on purpose - no white box may show around it.
assert( sprite_set_blend_mode( shadow, BLEND_MODE_MULTIPLY ) )
assert( sprite_set_pos( shadow, __SHADOW_POS.x, __SHADOW_POS.y ) )

assert( sprite_add_to_layer( glow, __GLOW_LAYER ) )
assert( sprite_add_to_layer( shadow, __GLOW_LAYER ) )
assert( sprite_add_to_layer( sunny, __SUNNY_LAYER ) )

-- The glow's mode, chosen by the number keys. nil = no mode of its own (it
-- draws by whatever mode is in effect, which for a sprite is normal).
local __GLOW_MODES <const> = {
    { key = KEY_ONE,   mode = nil,                 name = "no mode of its own" },
    { key = KEY_TWO,   mode = BLEND_MODE_NORMAL,   name = "BLEND_MODE_NORMAL" },
    { key = KEY_THREE, mode = BLEND_MODE_ADD,      name = "BLEND_MODE_ADD" },
    { key = KEY_FOUR,  mode = BLEND_MODE_MULTIPLY, name = "BLEND_MODE_MULTIPLY" },
    { key = KEY_FIVE,  mode = BLEND_MODE_SUBTRACT, name = "BLEND_MODE_SUBTRACT" },
}
local __DEFAULT_GLOW_MODE <const> = 3   -- add

local glowModeName

local function set_glow_mode( entry )
    if entry.mode then
        assert( sprite_set_blend_mode( glow, entry.mode ) )
    else
        assert( sprite_clear_blend_mode( glow ) )
    end
    glowModeName = entry.name
end

set_glow_mode( __GLOW_MODES[__DEFAULT_GLOW_MODE] )

-- The panels: the same red, green and blue squares (alpha 200), one panel
-- per mode, each over its own backdrop - the sky, except the add panel,
-- over a dark night where added colours mix instead of saturating to white.
local __PANEL        <const> = { x = 700, y = 20, w = 280, h = 200, gap = 12 }
local __SQUARE       <const> = 80
local __SQUARE_ALPHA <const> = 200
local __SKY          <const> = { 150, 205, 235 }
local __NIGHT        <const> = { 20, 25, 55 }
local __SQUARES      <const> = {
    { dx = 40,  dy = 50,  color = { 230, 40, 40 } },
    { dx = 95,  dy = 85,  color = { 40, 200, 60 } },
    { dx = 150, dy = 50,  color = { 50, 80, 230 } },
}
local __PANELS <const> = {
    { mode = BLEND_MODE_NORMAL,   name = "NORMAL: covers",    backdrop = __SKY },
    { mode = BLEND_MODE_ADD,      name = "ADD: adds (night)", backdrop = __NIGHT },
    { mode = BLEND_MODE_MULTIPLY, name = "MULTIPLY: tints",   backdrop = __SKY },
    { mode = BLEND_MODE_SUBTRACT, name = "SUBTRACT: removes", backdrop = __SKY },
}
local __LABEL_INSET <const> = 6     -- pixels from a panel's corner to its label
local __OPAQUE      <const> = 255

local function draw_panels()
    for index, panel in ipairs( __PANELS ) do
        local x = __PANEL.x
        local y = __PANEL.y + ( index - 1 ) * ( __PANEL.h + __PANEL.gap )
        local b = panel.backdrop

        draw_filled_rectangle( x, y, __PANEL.w, __PANEL.h, b[1], b[2], b[3], __OPAQUE )   -- normal: the backdrop

        draw_begin_blend_mode( panel.mode )
        for _, square in ipairs( __SQUARES ) do
            local c = square.color
            draw_filled_rectangle( x + square.dx, y + square.dy, __SQUARE, __SQUARE, c[1], c[2], c[3], __SQUARE_ALPHA )
        end
        draw_end_blend_mode()

        -- the label is drawn in normal mode, after draw_end_blend_mode, so it looks the same in every panel
        Hud.line_at( panel.name, x + __LABEL_INSET, y + __LABEL_INSET, Hud.TITLE )
    end
end

local sunnyX, sunnyY = __SUNNY_START.x, __SUNNY_START.y

function on_update( dt )

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then sunnyX = sunnyX - __SPEED end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then sunnyX = sunnyX + __SPEED end
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then sunnyY = sunnyY - __SPEED end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then sunnyY = sunnyY + __SPEED end

    for _, entry in ipairs( __GLOW_MODES ) do
        if input_is_key_released( entry.key ) then set_glow_mode( entry ) end
    end

    -- the glow is centred on Sunny (positions are top-left anchored)
    sprite_set_pos( sunny, sunnyX, sunnyY )
    sprite_set_pos( glow, sunnyX + ( __SUNNY_SIZE - __GLOW_SIZE ) // 2, sunnyY + ( __SUNNY_SIZE - __GLOW_SIZE ) // 2 )

    draw_panels()

    Hud.line_from_bottom( "blend sample - arrows/WASD move Sunny", 5, Hud.TITLE )
    Hud.line_from_bottom( "glow: 1 none  2 normal  3 add", 4, Hud.DIM )
    Hud.line_from_bottom( "      4 multiply  5 subtract", 3, Hud.DIM )
    Hud.line_from_bottom( "glow mode: " .. glowModeName, 2, Hud.INFO )
    Hud.line_from_bottom( "the shadow (bottom left) multiplies", 1, Hud.DIM )
end
