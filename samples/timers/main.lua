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

local Hud = dofile( "samples/shared/hud.lua" )

local __SECOND_TIMER   <const> = 1       -- timer ids
local __QUARTER_TIMER  <const> = 2
local __SECOND_MILLI   <const> = 1000    -- their intervals
local __QUARTER_MILLI  <const> = 250

assert( app_set_name( "Scarab - timers sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local secondTicks   = 0
local quarterTicks  = 0
local secondRunning = true

-- Plain Lua data only, per the gotcha above - no engine calls in here.
local function on_second_tick()
    secondTicks = secondTicks + 1
end

local function on_quarter_tick()
    quarterTicks = quarterTicks + 1
end

assert( set_timer( __SECOND_TIMER, __SECOND_MILLI, on_second_tick ) )
assert( set_timer( __QUARTER_TIMER, __QUARTER_MILLI, on_quarter_tick ) )

function on_update( dt )

    if input_is_key_released( KEY_F1 ) then
        if secondRunning then
            assert( reset_timer( __SECOND_TIMER ) )
        else
            assert( set_timer( __SECOND_TIMER, __SECOND_MILLI, on_second_tick ) )
        end

        secondRunning = not secondRunning
    end

    Hud.line( "timers sample", 1, Hud.TITLE )
    Hud.line( __SECOND_MILLI .. "ms timer ticks: " .. tostring( secondTicks )
        .. "  (F1 to " .. ( secondRunning and "stop" or "restart" ) .. ")", 2, Hud.INFO )
    Hud.line( __QUARTER_MILLI .. "ms timer ticks: " .. tostring( quarterTicks ), 3, Hud.INFO )
end
