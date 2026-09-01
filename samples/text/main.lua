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
 text - HUD-style text layout (docs/lua-api/text.html), beyond
 hello-world's minimal single draw_text call.

 Demonstrates measure_text (right-aligning a growing counter against
 screen_get_width), screen_get_width/screen_get_height (drawing a border
 exactly on the render target's edges), and set_font three times over:
 first a path that can't exist, to show its documented graceful-failure
 behavior (returns false, leaves the previously-active font untouched,
 no error or crash); then "Press Start 2P" (resources/fonts/,
 OFL-licensed - see resources/fonts/README.md and ofl-pressstart2p.txt
 for full attribution), a single-file TrueType font; then
 "caravellius8x8" (resources/fonts/, an original bitmap font made for
 Caravellius), a multi-file AngelCode BMFont atlas - the last set_font
 call wins, so caravellius8x8 ends up the font actually used for the
 rest of this sample, same call shape either way regardless of format
 (set_font has no format-specific logic of it's own; raylib's LoadFont
 auto-detects TrueType/OpenType vs. a BMFont ".fnt" atlas purely from
 the file's own extension).

 NOTE: multi-file BMFont loading (like caravellius8x8.fnt's separate
 ".png" atlas) used to fail here - a real, verified engine bug in how
 Scarab's mount-based filesystem hook interacted with raylib's own
 internal BMFont loading, root-caused and fixed upstream in sunlight
 v0.17.3 (see root CLAUDE.md's "Known gotchas" and
 resources/fonts/README.md for the full story). This sample now
 demonstrates the real, working multi-file BMFont load as a result.
]]

app_set_name( "Scarab - text sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

-- This path can't exist - expected to fail and fall back to the engine's
-- own built-in default font, exactly as set_font's own doc comment
-- describes. bogusFontLoaded is only ever used for the HUD line below.
local bogusFontLoaded = set_font( "resources/fonts/this-font-does-not-exist.ttf" )

-- "Press Start 2P" by The Press Start 2P Project Authors, SIL Open Font
-- License 1.1 (resources/fonts/ofl-pressstart2p.txt) - used here
-- unmodified, under it's own real name, per OFL's own Reserved Font
-- Name terms (see resources/fonts/README.md). A single-file TrueType
-- font - was never affected by the BMFont bug mentioned above.
local pressStart2PLoaded = set_font( "resources/fonts/pressstart2p-regular.ttf" )

-- caravellius8x8 - an original bitmap font made for Caravellius
-- (resources/fonts/README.md). A multi-file AngelCode BMFont atlas -
-- this is the call that used to fail (see the module comment above);
-- fixed in sunlight v0.17.3. Called last, so this becomes the font
-- actually active for the rest of this sample.
local caravelliusLoaded = set_font( "resources/fonts/caravellius8x8.fnt" )

local screenWidth  = screen_get_width()
local screenHeight = screen_get_height()

local counter = 0
local __COUNT_INTERVAL_FRAMES = 30  -- ~0.5s at 60fps - see CLAUDE.md's dt gotcha for why frame-counting, not dt-accumulation, is used here
local framesSinceCount = 0

function on_update( dt )

    framesSinceCount = framesSinceCount + 1

    if framesSinceCount >= __COUNT_INTERVAL_FRAMES then
        counter          = counter + 1
        framesSinceCount = 0
    end

    -- A border drawn exactly on the fixed render target's own edges -
    -- confirms screen_get_width/screen_get_height describe that fixed
    -- resolution, not the live/resizeable OS window size.
    draw_filled_rectangle( 0, 0, screenWidth, 4, 80, 80, 80, 255 )
    draw_filled_rectangle( 0, screenHeight - 4, screenWidth, 4, 80, 80, 80, 255 )
    draw_filled_rectangle( 0, 0, 4, screenHeight, 80, 80, 80, 255 )
    draw_filled_rectangle( screenWidth - 4, 0, 4, screenHeight, 80, 80, 80, 255 )

    draw_text( "text sample - now using caravellius8x8 (a multi-file BMFont)", 20, 20, 20, 255, 255, 255, 255 )
    draw_text( "set_font: bad path=" .. tostring( bogusFontLoaded )
        .. "  Press Start 2P=" .. tostring( pressStart2PLoaded )
        .. "  caravellius8x8=" .. tostring( caravelliusLoaded ), 20, 50, 14, 200, 200, 200, 255 )

    -- Right-align "Count: N" against the right edge via measure_text,
    -- rather than a hardcoded x - the text's own width changes as the
    -- digit count grows (single digit vs. double digit, ...), and now
    -- also reflects the active font's own real glyph metrics.
    local countText = "Count: " .. tostring( counter )
    local textWidth = measure_text( countText, 20 )

    draw_text( countText, screenWidth - textWidth - 20, screenHeight - 44, 20, 255, 220, 120, 255 )
end
