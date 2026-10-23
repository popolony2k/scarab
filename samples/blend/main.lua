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

tilemap_load_map( "resources/tilemap/test.tmx", MAP_ALIGNMENT_TOP_LEFT )

local GLOW_LAYER    = 4     -- "clouds" in test.tmx: drawn before Sunny's layer
local SUNNY_LAYER   = 5     -- "houses"
local SUNNY_SIZE    = 32    -- one frame of sunny_idle_down.png
local GLOW_SIZE     = 128   -- glow.png
local SPEED         = 3     -- pixels/frame
local SUNNY_START   = { x = 250, y = 250 }
local SHADOW_POS    = { x = 60, y = 520 }

-- Every asset here is in resources/sprites/ - see its README.md.
pool_register_type( "sunny", 1 )
pool_register_type( "glow", 1 )
pool_register_type( "shadow", 1 )

local sunny  = sprite_acquire( "sunny" )
local glow   = sprite_acquire( "glow" )
local shadow = sprite_acquire( "shadow" )

sprite_configure_texture( sunny, 0, "resources/sprites/sunny_idle_down.png", 4, 0, TEXTURE_ANIMATION_MODE_AUTOMATIC_CIRCULAR, 100 )
sprite_set_active_sequence( sunny, 0 )
sprite_configure_texture( glow, 0, "resources/sprites/glow.png", 1, 0, TEXTURE_ANIMATION_MODE_MANUAL, 0 )
sprite_set_active_sequence( glow, 0 )
sprite_configure_texture( shadow, 0, "resources/sprites/shadow.png", 1, 0, TEXTURE_ANIMATION_MODE_MANUAL, 0 )
sprite_set_active_sequence( shadow, 0 )

-- The shadow always multiplies: it darkens what is under it. Its PNG's
-- transparent pixels are white on purpose - no white box may show around it.
assert( sprite_set_blend_mode( shadow, BLEND_MODE_MULTIPLY ) )
sprite_set_pos( shadow, SHADOW_POS.x, SHADOW_POS.y )

sprite_add_to_layer( glow, GLOW_LAYER )
sprite_add_to_layer( shadow, GLOW_LAYER )
sprite_add_to_layer( sunny, SUNNY_LAYER )

-- The glow's mode, chosen by the number keys. nil = no mode of its own (it
-- draws by whatever mode is in effect, which for a sprite is normal).
local GLOW_MODES = {
    { key = KEY_ONE,   mode = nil,                 name = "no mode of its own" },
    { key = KEY_TWO,   mode = BLEND_MODE_NORMAL,   name = "BLEND_MODE_NORMAL" },
    { key = KEY_THREE, mode = BLEND_MODE_ADD,      name = "BLEND_MODE_ADD" },
    { key = KEY_FOUR,  mode = BLEND_MODE_MULTIPLY, name = "BLEND_MODE_MULTIPLY" },
    { key = KEY_FIVE,  mode = BLEND_MODE_SUBTRACT, name = "BLEND_MODE_SUBTRACT" },
}
local DEFAULT_GLOW_MODE = 3   -- add

local glowModeName

local function set_glow_mode( entry )
    if entry.mode then
        assert( sprite_set_blend_mode( glow, entry.mode ) )
    else
        assert( sprite_clear_blend_mode( glow ) )
    end
    glowModeName = entry.name
end

set_glow_mode( GLOW_MODES[DEFAULT_GLOW_MODE] )

-- The panels: the same red, green and blue squares (alpha 200), one panel
-- per mode, each over its own backdrop - the sky, except the add panel,
-- over a dark night where added colours mix instead of saturating to white.
local PANEL        = { x = 700, y = 20, w = 280, h = 200, gap = 12 }
local SQUARE       = 80
local SQUARE_ALPHA = 200
local SKY          = { 150, 205, 235 }
local NIGHT        = { 20, 25, 55 }
local SQUARES      = {
    { dx = 40,  dy = 50,  color = { 230, 40, 40 } },
    { dx = 95,  dy = 85,  color = { 40, 200, 60 } },
    { dx = 150, dy = 50,  color = { 50, 80, 230 } },
}
local PANELS = {
    { mode = BLEND_MODE_NORMAL,   name = "NORMAL: the squares cover each other",  backdrop = SKY },
    { mode = BLEND_MODE_ADD,      name = "ADD: colours add up (over night)",      backdrop = NIGHT },
    { mode = BLEND_MODE_MULTIPLY, name = "MULTIPLY: each square tints the sky",  backdrop = SKY },
    { mode = BLEND_MODE_SUBTRACT, name = "SUBTRACT: each takes its colour out",  backdrop = SKY },
}
local LABEL_COLOR = { 255, 255, 255, 255 }
local HUD_PLATE   = { x = 10, y = 660, w = 680, h = 120 }

local function draw_panels()
    for index, panel in ipairs( PANELS ) do
        local x = PANEL.x
        local y = PANEL.y + ( index - 1 ) * ( PANEL.h + PANEL.gap )
        local b = panel.backdrop

        draw_filled_rectangle( x, y, PANEL.w, PANEL.h, b[1], b[2], b[3], 255 )   -- normal: the backdrop

        draw_begin_blend_mode( panel.mode )
        for _, square in ipairs( SQUARES ) do
            local c = square.color
            draw_filled_rectangle( x + square.dx, y + square.dy, SQUARE, SQUARE, c[1], c[2], c[3], SQUARE_ALPHA )
        end
        draw_end_blend_mode()

        -- the label is drawn in normal mode, after draw_end_blend_mode, so it looks the same in every panel
        draw_filled_rectangle( x, y, PANEL.w, 24, 0, 0, 0, 160 )
        draw_text( panel.name, x + 6, y + 4, 16, LABEL_COLOR[1], LABEL_COLOR[2], LABEL_COLOR[3], LABEL_COLOR[4] )
    end
end

local sunnyX, sunnyY = SUNNY_START.x, SUNNY_START.y

function on_update( dt )

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then sunnyX = sunnyX - SPEED end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then sunnyX = sunnyX + SPEED end
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then sunnyY = sunnyY - SPEED end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then sunnyY = sunnyY + SPEED end

    for _, entry in ipairs( GLOW_MODES ) do
        if input_is_key_released( entry.key ) then set_glow_mode( entry ) end
    end

    -- the glow is centred on Sunny (positions are top-left anchored)
    sprite_set_pos( sunny, sunnyX, sunnyY )
    sprite_set_pos( glow, sunnyX + ( SUNNY_SIZE - GLOW_SIZE ) // 2, sunnyY + ( SUNNY_SIZE - GLOW_SIZE ) // 2 )

    draw_panels()

    draw_filled_rectangle( HUD_PLATE.x, HUD_PLATE.y, HUD_PLATE.w, HUD_PLATE.h, 0, 0, 0, 200 )
    draw_text( "blend sample - arrows/WASD move Sunny", 20, HUD_PLATE.y + 10, 20, 255, 255, 255, 255 )
    draw_text( "glow: 1 none, 2 normal, 3 add, 4 multiply, 5 subtract", 20, HUD_PLATE.y + 40, 18, 220, 220, 220, 255 )
    draw_text( "glow mode: " .. glowModeName, 20, HUD_PLATE.y + 66, 18, 255, 220, 120, 255 )
    draw_text( "the shadow (bottom left) always multiplies", 20, HUD_PLATE.y + 92, 18, 220, 220, 220, 255 )
end
