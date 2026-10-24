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
    Input constants smoke test - run by scripts/input_constants_smoke.py (and ci.yml, every
    platform), which passes the expected constant names in INPUT_SMOKE_NAMES (read from
    src/lua/luainputapi.cpp's own arrays, so the list can't drift):

        INPUT_SMOKE_NAMES="KEY_A,...;GAMEPAD_BUTTON_...;GAMEPAD_AXIS_...;CONTROLLER_..." \
            scarab --headless --fast --max-frames 200 scripts/input_constants_smoke/project.json

    Pins the KEY_* / GAMEPAD_BUTTON_* / GAMEPAD_AXIS_* / CONTROLLER_* constants Lua sees
    (doc/MASTER_PLAN.md item V4, sunlight BREAKAGES entry 2). Up to sunlight v0.38.0,
    KEY_MENU was 82 - the same number as KEY_R - so input_is_key_down( KEY_MENU ) fired on R
    and a key-indexed table mixed the two up. Now it is 5, raylib's own (Android's menu button).
    Every constant of a group must exist and no two may share a value, which catches that
    whole class of bug, not only this key.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

local function split( text, separator )
    local parts = {}
    for part in string.gmatch( text, "[^" .. separator .. "]+" ) do parts[#parts + 1] = part end
    return parts
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- 1. the values this item is about (raylib's own; sunlight translates keys by name now)
check( KEY_MENU == 5, "KEY_MENU is " .. tostring( KEY_MENU ) .. ", want 5" )
check( KEY_BACK == 4, "KEY_BACK is " .. tostring( KEY_BACK ) .. ", want 4" )
check( KEY_R == 82, "KEY_R is " .. tostring( KEY_R ) .. ", want 82" )
check( KEY_MENU ~= KEY_R, "KEY_MENU and KEY_R are the same number (" .. tostring( KEY_R ) .. ")" )

-- 2. every group: every expected constant exists, is an integer, and no two share a value.
-- Names are checked in the order luainputapi.cpp lists them (never pairs() - see CLAUDE.md).
local groups = split( os.getenv( "INPUT_SMOKE_NAMES" ) or "", ";" )
local GROUP_TITLES = { "keyboard keys", "gamepad buttons", "gamepad axes", "controller types" }

check( #groups == #GROUP_TITLES, "INPUT_SMOKE_NAMES has " .. #groups .. " groups, want " .. #GROUP_TITLES .. " (run through input_constants_smoke.py)" )

for index, title in ipairs( GROUP_TITLES ) do
    local names    = split( groups[index] or "", "," )
    local byValue  = {}

    check( #names > 0, title .. ": no names given" )

    for _, name in ipairs( names ) do
        local value = _G[name]

        if math.type( value ) ~= "integer" then
            check( false, title .. ": " .. name .. " is " .. tostring( value ) .. ", want an integer constant" )
        elseif byValue[value] then
            check( false, title .. ": " .. name .. " and " .. byValue[value] .. " are the same number (" .. value .. ")" )
        else
            byValue[value] = name
        end
    end
end

local frames = 0
function on_update( dt )
    frames = frames + 1
    if frames == 1 then
        if #failures == 0 then
            print( "INPUT CONSTANTS SMOKE OK" )
            app_quit()
        else
            for _, f in ipairs( failures ) do print( "INPUT CONSTANTS SMOKE FAIL: " .. f ) end
        end
    end
end
