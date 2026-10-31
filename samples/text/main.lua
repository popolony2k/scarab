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
 text - HUD-style text layout and fonts by handle (docs/lua-api/text.html),
 beyond hello-world's minimal single draw_text call.

 Demonstrates:
 - load_font: each font file is loaded ONCE into a handle - "caravellius8x8"
   regular and bold (resources/fonts/, the project's own multi-file
   AngelCode BMFont atlases) and "Press Start 2P" (a single-file TrueType
   font, SIL OFL 1.1 - see resources/fonts/README.md and ofl-pressstart2p.txt);
 - draw_text's optional last argument: one line in each font, all on screen
   at once, without switching the active font;
 - set_font's forms: a handle (keys 1-3 pick the active font), nil (key 0,
   the built-in font), and a path that can't exist (its error is shown);
 - measure_text with the active font: a counter right-aligned against
   screen_get_width, which follows whichever font is active;
 - a line with Portuguese and Spanish accents in the TrueType font (a TTF
   loads with Latin accents; a bitmap .fnt only has the glyphs its atlas
   holds);
 - screen_get_width/screen_get_height: a border drawn exactly on the
   render target's edges.
]]

assert( app_set_name( "Scarab - text sample" ) )

local Hud = dofile( "samples/shared/hud.lua" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

-- Each font is loaded once; the handle can be the active font, be passed to
-- draw_text/measure_text directly, and serve the map's text objects
-- (register_font, see samples/objects) - one file, one load.
local __FONTS <const> = {
    { key = KEY_ONE,   name = "caravellius8x8",      font = assert( load_font( "resources/fonts/caravellius8x8.fnt" ) ) },
    { key = KEY_TWO,   name = "caravellius8x8 bold", font = assert( load_font( "resources/fonts/caravellius8x8_bold.fnt" ) ) },
    { key = KEY_THREE, name = "Press Start 2P",      font = assert( load_font( "resources/fonts/pressstart2p-regular.ttf" ) ) },
}
local __PIXEL <const> = __FONTS[3].font

-- A path that can't exist: set_font returns nil and the reason, and the
-- active font stays as it was.
local _, badPathMessage = set_font( "resources/fonts/this-font-does-not-exist.ttf" )

local activeName = __FONTS[1].name
assert( set_font( __FONTS[1].font ) )

local screenWidth  = screen_get_width()
local screenHeight = screen_get_height()

local __COUNT_INTERVAL_FRAMES <const> = 30    -- ~0.5s at 60fps - see CLAUDE.md's dt gotcha for why frame-counting, not dt-accumulation, is used here
local __LINE_SIZE             <const> = 20    -- the demonstration lines' size
local __NOTE_SIZE             <const> = 14    -- the bad-path note's size
local __MARGIN                <const> = 20    -- pixels from the screen's edges
local __FONT_LIST_TOP         <const> = 60    -- y of the first "font: ..." line
local __FONT_LIST_SPACING     <const> = 40    -- pixels between them
local __ACCENTS_Y             <const> = 200
local __BAD_PATH_Y            <const> = 240
local __COUNT_FROM_BOTTOM     <const> = 44    -- pixels from the screen's bottom to the counter
local __BORDER                <const> = 4     -- the border's thickness
local __BORDER_COLOR          <const> = { r = 80, g = 80, b = 80 }
local __WHITE                 <const> = { r = 255, g = 255, b = 255 }
local __FONT_COLOR            <const> = { r = 200, g = 220, b = 255 }
local __ACCENT_COLOR          <const> = { r = 255, g = 220, b = 120 }
local __ERROR_COLOR           <const> = { r = 255, g = 140, b = 140 }
local __OPAQUE                <const> = 255

local counter          = 0
local framesSinceCount = 0

function on_update( dt )

    framesSinceCount = framesSinceCount + 1

    if framesSinceCount >= __COUNT_INTERVAL_FRAMES then
        counter          = counter + 1
        framesSinceCount = 0
    end

    -- keys 1-3 select the active font by handle, 0 the built-in font
    for _, entry in ipairs( __FONTS ) do
        if input_is_key_released( entry.key ) then
            assert( set_font( entry.font ) )
            activeName = entry.name
        end
    end

    if input_is_key_released( KEY_ZERO ) then
        assert( set_font( nil ) )
        activeName = "the built-in font"
    end

    -- A border drawn exactly on the fixed render target's own edges -
    -- confirms screen_get_width/screen_get_height describe that fixed
    -- resolution, not the live/resizeable OS window size.
    local b = __BORDER_COLOR

    draw_filled_rectangle( 0, 0, screenWidth, __BORDER, b.r, b.g, b.b, __OPAQUE )
    draw_filled_rectangle( 0, screenHeight - __BORDER, screenWidth, __BORDER, b.r, b.g, b.b, __OPAQUE )
    draw_filled_rectangle( 0, 0, __BORDER, screenHeight, b.r, b.g, b.b, __OPAQUE )
    draw_filled_rectangle( screenWidth - __BORDER, 0, __BORDER, screenHeight, b.r, b.g, b.b, __OPAQUE )

    -- with the active font (no font argument)
    draw_text( "active font: " .. activeName, __MARGIN, __MARGIN, __LINE_SIZE, __WHITE.r, __WHITE.g, __WHITE.b, __OPAQUE )
    Hud.line_from_bottom( "keys 1, 2, 3 pick the active font - 0 the built-in one", 3, Hud.DIM )

    -- one line in each loaded font, all at once (draw_text's optional last argument)
    for index, entry in ipairs( __FONTS ) do
        draw_text( "font: " .. entry.name, __MARGIN, __FONT_LIST_TOP + ( index - 1 ) * __FONT_LIST_SPACING, __LINE_SIZE,
            __FONT_COLOR.r, __FONT_COLOR.g, __FONT_COLOR.b, __OPAQUE, entry.font )
    end

    draw_text( "Ação, coração, açúcar - año, niño, mañana - café", __MARGIN, __ACCENTS_Y, __LINE_SIZE,
        __ACCENT_COLOR.r, __ACCENT_COLOR.g, __ACCENT_COLOR.b, __OPAQUE, __PIXEL )
    draw_text( "set_font( bad path ): " .. tostring( badPathMessage ), __MARGIN, __BAD_PATH_Y, __NOTE_SIZE,
        __ERROR_COLOR.r, __ERROR_COLOR.g, __ERROR_COLOR.b, __OPAQUE )

    -- Right-align "Count: N" against the right edge via measure_text,
    -- rather than a hardcoded x - the text's own width changes as the
    -- digit count grows, and follows whichever font is active.
    local countText = "Count: " .. tostring( counter )
    local textWidth = measure_text( countText, __LINE_SIZE )

    draw_text( countText, screenWidth - textWidth - __MARGIN, screenHeight - __COUNT_FROM_BOTTOM, __LINE_SIZE,
        __ACCENT_COLOR.r, __ACCENT_COLOR.g, __ACCENT_COLOR.b, __OPAQUE )
end
