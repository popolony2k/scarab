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
 hello-world - the smallest possible Scarab game.

 Demonstrates the bare minimum needed to get a window open and something
 drawn on screen: app_set_name (see docs/lua-api/app.html) and draw_text
 (see docs/lua-api/text.html), driven by on_update, the optional per-frame
 hook the engine calls every frame if it's defined as a global function.

 No tilemap, no sprites, no sound - see the other samples/ categories for
 those, each demonstrating one corner of the Lua API on its own.
]]

-- Runs once, before the first frame - see the "app" sample for the rest
-- of what app_* covers (fullscreen, target FPS, window resizing, ...).
app_set_name( "Hello, Scarab!" )

-- Every entry script must queue at least one ScriptProcessor command,
-- even one that does nothing meaningful - sunlight's own
-- ScriptProcessor::Compile() (called once, right after this whole script
-- finishes running) fails if the queue is still empty, and a failed
-- compile is a fatal engine-startup error. This sample has nothing worth
-- sequencing, so a single trivial 1ms sp_wait is the smallest command
-- that satisfies this without doing anything observable.
sp_wait( 1 )

-- draw_text has to be called every frame to keep showing anything - it's
-- not "issue once and it persists". set_font is deliberately not called
-- here: until it is, draw_text/measure_text just use the engine's own
-- built-in default font, so this sample needs zero font asset of its own.
function on_update( dt )
    draw_text( "Hello, Scarab!", 20, 20, 32, 255, 255, 255, 255 )
end
