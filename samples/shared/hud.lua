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
 hud - the samples' shared heads-up display: lines of text in Caravellius 8x8
 on dark translucent plates, readable over any map. Every sample loads it with

     local Hud = dofile( "samples/shared/hud.lua" )

 and draws its HUD from on_update:

     Hud.line( "camera sample - arrows pan", 1, Hud.TITLE )     -- row 1, from the top
     Hud.line( "camera_get_position: 10, 20", 2, Hud.INFO )
     Hud.line_from_bottom( "keys: 1-5 maps", 1, Hud.DIM )        -- the bottom row
     local used = Hud.lines( longMessage, 3, Hud.INFO )           -- wrapped to the window's width

 Hud.row_y( row ) gives a row's y, to place something else under the HUD.

 The font is loaded on the first line drawn, not when this file is loaded: a
 font needs a renderer, and a sample that calls renderer_create must do so
 before any window-needing call (see CLAUDE.md).
]]

local Hud = {}

local __FONT_PATH      <const> = "resources/fonts/caravellius8x8.fnt"
local __TEXT_SIZE      <const> = 16     -- pixels; the 8x8 font drawn at twice its size
local __ROW_SPACING    <const> = 28     -- pixels from one row to the next
local __LEFT           <const> = 20     -- pixels from the window's left edge
local __TOP            <const> = 20     -- pixels from the window's top to row 1
local __BOTTOM         <const> = 20     -- pixels from the bottom row to the window's bottom
local __PLATE_PADDING  <const> = 4      -- pixels of plate around the text
local __PLATE_ALPHA    <const> = 170    -- the plate's opacity, 0..255
local __OPAQUE         <const> = 255

-- The colours the samples use for their HUD lines.
Hud.TITLE = { r = 255, g = 255, b = 255 }   -- what the sample is and its keys
Hud.INFO  = { r = 255, g = 220, b = 120 }   -- values read back from the engine
Hud.DIM   = { r = 200, g = 200, b = 200 }   -- secondary hints
Hud.ALERT = { r = 255, g = 110, b = 110 }   -- a failure the sample reports
Hud.SIZE  = __TEXT_SIZE

local hudFont = nil

--[[
 The HUD's font, loaded on first use.
]]
function Hud.font()

    if not hudFont then
        hudFont = assert( load_font( __FONT_PATH ) )
    end

    return hudFont
end

--[[
 One line of text on its plate, at a screen position.
]]
function Hud.line_at( text, x, y, color )

    local font = Hud.font()

    draw_filled_rectangle( x - __PLATE_PADDING, y - __PLATE_PADDING,
        measure_text( text, __TEXT_SIZE, font ) + 2 * __PLATE_PADDING, __TEXT_SIZE + 2 * __PLATE_PADDING,
        0, 0, 0, __PLATE_ALPHA )
    draw_text( text, x, y, __TEXT_SIZE, color.r, color.g, color.b, __OPAQUE, font )
end

--[[
 The y of row `row` counted from the top (1 is the first). While the engine's FPS counter is on
 (app_set_draw_fps), it sits in the top-left corner, so the rows start one row lower.
]]
function Hud.row_y( row )

    local top = __TOP + ( app_get_draw_fps() and __ROW_SPACING or 0 )

    return top + ( row - 1 ) * __ROW_SPACING
end

--[[
 One line of text on its plate, in row `row` counted from the top (1 is the first).
]]
function Hud.line( text, row, color )

    Hud.line_at( text, __LEFT, Hud.row_y( row ), color )
end

--[[
 A text too long for one row, wrapped at spaces to the window's width, from row `row` down. Returns
 how many rows it took, so the next line can go after it.
]]
function Hud.lines( text, row, color )

    local font     = Hud.font()
    local maxWidth = screen_get_width() - 2 * __LEFT
    local current  = ""
    local rows     = 0

    for word in string.gmatch( text, "%S+" ) do
        local candidate = ( current == "" ) and word or ( current .. " " .. word )

        if current ~= "" and measure_text( candidate, __TEXT_SIZE, font ) > maxWidth then
            Hud.line( current, row + rows, color )
            rows    = rows + 1
            current = word
        else
            current = candidate
        end
    end

    if current ~= "" then
        Hud.line( current, row + rows, color )
        rows = rows + 1
    end

    return rows
end

--[[
 One line of text on its plate, in row `row` counted from the bottom (1 is the last).
]]
function Hud.line_from_bottom( text, row, color )

    Hud.line_at( text, __LEFT, screen_get_height() - __BOTTOM - __TEXT_SIZE - ( row - 1 ) * __ROW_SPACING, color )
end

return Hud
