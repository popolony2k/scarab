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
 timers - set_timer/reset_timer (docs/lua-api/timers.html).

 Two independent background timers (1000ms and 250ms), plus a key to
 stop/restart the slow one via reset_timer/set_timer.

 IMPORTANT - see root CLAUDE.md's "Known gotchas": a set_timer callback
 runs on the Timer's own background thread, not the main thread it
 appears to. It's only safe to touch plain Lua data from inside one -
 flip a counter/boolean, nothing else. Never call a primitive that
 reaches the engine itself (draw_text, sound_*, sprite_*, ...) from
 inside a set_timer callback - read the flag back from on_update
 (always the main thread) instead, which is exactly what this sample
 does below.
]]

app_set_name( "Scarab - timers sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local secondTicks   = 0
local quarterTicks  = 0
local secondRunning = true

-- Plain Lua data only, per the gotcha above - no engine calls in here.
local function onSecondTick()
    secondTicks = secondTicks + 1
end

local function onQuarterTick()
    quarterTicks = quarterTicks + 1
end

set_timer( 1, 1000, onSecondTick )
set_timer( 2, 250, onQuarterTick )

function on_update( dt )

    if input_is_key_released( KEY_F1 ) then
        if secondRunning then
            reset_timer( 1 )
        else
            set_timer( 1, 1000, onSecondTick )
        end

        secondRunning = not secondRunning
    end

    draw_text( "timers sample", 20, 20, 24, 255, 255, 255, 255 )
    draw_text( "1000ms timer ticks: " .. tostring( secondTicks )
        .. "  (F1 to " .. ( secondRunning and "stop" or "restart" ) .. ")", 20, 54, 20, 200, 200, 200, 255 )
    draw_text( "250ms timer ticks: " .. tostring( quarterTicks ), 20, 78, 20, 200, 200, 200, 255 )
end
