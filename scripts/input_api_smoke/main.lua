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
    Input API smoke test - run by scripts/input_api_smoke.py (and ci.yml, every platform):

        scarab --headless --fast --max-frames 200 scripts/input_api_smoke/project.json

    Pins plans/MASTER_PLAN.md item C1a: connected pads, stick dead zones and player slots. Headless
    input sees no pad at all, so this pins everything that doesn't need one: the empty pad list and
    names, the connection handler being accepted, the automatic and raw stick reads, the dead zone
    per pad and axis (default 0.1, round trips, triggers refused), and the player slots in full
    (claim, conflict, replace, release, both lookups). The dead-zone RULE itself is a doctest
    (tests/test_deadzone.cpp); a real pad, hot-plug and the filtered values are checked by hand
    with samples/input.

    The slot functions and the connection handler need no window: they run first and the script
    checks no renderer was created by them.

    The script quits ITSELF only if every check passed (exit 0); otherwise it prints every
    failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __DEFAULT_DEADZONE <const> = 0.1
local __STICKS <const> = { GAMEPAD_AXIS_LEFT_X, GAMEPAD_AXIS_LEFT_Y, GAMEPAD_AXIS_RIGHT_X, GAMEPAD_AXIS_RIGHT_Y }

local failures = {}

-- Record a failure message when `ok` is false. Every check runs; the failures are printed together at the end.
local function check( ok, message )
    if not ok then failures[#failures + 1] = message end
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

-- The engine asks Lua to claim each stage id an sp_load_stage queues: claim any (this test loads its maps itself),
-- so an unclaimed id can never stop the run.
function on_load_stage( stageId ) return true end
sp_wait( 1 )

-- player slots and the connection handler: no window needed
check( input_set_gamepad_connection_handler( function( gamepadId, connected ) end ) == true, "set_gamepad_connection_handler failed" )

check( input_get_gamepad_for_slot( 1 ) == false, "an unclaimed slot must read false" )
check( input_get_slot_for_gamepad( 0 ) == false, "an unclaimed pad must read false" )
check( input_claim_player_slot( 1, 0 ) == true, "claim 1 <- pad 0 failed" )
check( input_get_gamepad_for_slot( 1 ) == 0, "slot 1 must hold pad 0" )
check( input_get_slot_for_gamepad( 0 ) == 1, "pad 0 must be in slot 1" )
check( input_claim_player_slot( 1, 0 ) == true, "claiming the same pair again must succeed" )
expect_error( "conflict", "gamepad 0 is already claimed by slot 1", input_claim_player_slot( 2, 0 ) )
check( input_get_gamepad_for_slot( 2 ) == false, "a refused claim must leave slot 2 empty" )
check( input_claim_player_slot( 2, 5 ) == true, "claim 2 <- pad 5 failed" )
check( input_claim_player_slot( 1, 3 ) == true, "claim 1 <- pad 3 (replacing pad 0) failed" )
check( input_get_gamepad_for_slot( 1 ) == 3, "slot 1 must hold pad 3 now" )
check( input_get_slot_for_gamepad( 0 ) == false, "pad 0 must be free after slot 1 took pad 3" )
check( input_claim_player_slot( 0, 0 ) == true, "slot 0 is a valid slot" )
check( input_release_player_slot( 1 ) == true, "release slot 1 failed" )
check( input_get_gamepad_for_slot( 1 ) == false and input_get_slot_for_gamepad( 3 ) == false, "release must free slot 1 and pad 3" )
check( input_get_gamepad_for_slot( 2 ) == 5, "releasing slot 1 must leave slot 2 alone" )
check( input_release_player_slot( 7 ) == true, "releasing an empty slot is a no-op success" )
expect_error( "slot -1", "argument 'slot' must be between 0 and", input_claim_player_slot( -1, 0 ) )
expect_error( "slot fraction", "argument 'slot' must be an integer", input_release_player_slot( 1.5 ) )
expect_error( "slot pad", "argument 'gamepadId' must be between 0 and 9", input_claim_player_slot( 1, 10 ) )
expect_error( "slot type", "argument 'slot'", input_get_gamepad_for_slot( "one" ) )
expect_error( "handler type", "argument 'fn' must be a function", input_set_gamepad_connection_handler( "f" ) )

check( renderer_get_current() == nil, "player slots or the connection handler created a renderer" )

-- connected pads: headless input has none
local pads = input_get_connected_gamepads()

check( type( pads ) == "table" and #pads == 0, "headless: expected no connected pads" )
check( input_is_gamepad_available( 0 ) == false, "headless: pad 0 must be unavailable" )
check( input_get_gamepad_name( 0 ) == "", "headless: an absent pad's name must be ''" )
check( input_get_gamepad_name( 9 ) == "", "pad 9 is a valid id" )
expect_error( "available id", "argument 'gamepadId' must be between 0 and 9", input_is_gamepad_available( 10 ) )

-- stick dead zones: default, round trips, per pad and axis
for _, axis in ipairs( __STICKS ) do
    check( input_get_axis_deadzone( 0, axis ) == __DEFAULT_DEADZONE, "default dead zone of axis " .. axis .. " must be 0.1" )
end

check( input_set_axis_deadzone( 0, GAMEPAD_AXIS_LEFT_X, 0.15 ) == true, "set dead zone failed" )
check( input_get_axis_deadzone( 0, GAMEPAD_AXIS_LEFT_X ) == 0.15, "0.15 must read back as 0.15" )
check( input_get_axis_deadzone( 0, GAMEPAD_AXIS_LEFT_Y ) == __DEFAULT_DEADZONE, "setting LEFT_X must not touch LEFT_Y" )
check( input_get_axis_deadzone( 1, GAMEPAD_AXIS_LEFT_X ) == __DEFAULT_DEADZONE, "setting pad 0 must not touch pad 1" )
check( input_set_axis_deadzone( 2, GAMEPAD_AXIS_RIGHT_Y, 0 ) == true and input_get_axis_deadzone( 2, GAMEPAD_AXIS_RIGHT_Y ) == 0,
       "a dead zone of 0 must be accepted" )
check( input_set_axis_deadzone( 2, GAMEPAD_AXIS_RIGHT_X, 1 ) == true and input_get_axis_deadzone( 2, GAMEPAD_AXIS_RIGHT_X ) == 1,
       "a dead zone of 1 must be accepted" )
expect_error( "dead zone > 1", "argument 'deadzone' must be between 0 and 1", input_set_axis_deadzone( 0, GAMEPAD_AXIS_LEFT_X, 1.01 ) )
expect_error( "dead zone < 0", "argument 'deadzone' must be between 0 and 1", input_set_axis_deadzone( 0, GAMEPAD_AXIS_LEFT_X, -0.1 ) )
expect_error( "dead zone type", "argument 'deadzone'", input_set_axis_deadzone( 0, GAMEPAD_AXIS_LEFT_X, "big" ) )
expect_error( "trigger set", "argument 'axis' must be a stick axis", input_set_axis_deadzone( 0, GAMEPAD_AXIS_LEFT_TRIGGER, 0.2 ) )
expect_error( "trigger get", "argument 'axis' must be a stick axis", input_get_axis_deadzone( 0, GAMEPAD_AXIS_RIGHT_TRIGGER ) )
expect_error( "dead zone pad", "argument 'gamepadId'", input_get_axis_deadzone( 10, GAMEPAD_AXIS_LEFT_X ) )
check( input_get_axis_deadzone( 0, GAMEPAD_AXIS_LEFT_X ) == 0.15, "a refused set must keep the dead zone" )

-- the automatic and raw reads: headless sticks are exactly 0, triggers readable through both
for _, axis in ipairs( __STICKS ) do
    check( input_get_gamepad_axis( 0, axis ) == 0 and input_get_gamepad_axis_raw( 0, axis ) == 0, "headless stick " .. axis .. " must read 0" )
end

check( type( input_get_gamepad_axis( 0, GAMEPAD_AXIS_LEFT_TRIGGER ) ) == "number", "the automatic read takes a trigger" )
check( type( input_get_gamepad_axis_raw( 0, GAMEPAD_AXIS_RIGHT_TRIGGER ) ) == "number", "the raw read takes a trigger" )
expect_error( "raw axis", "unknown GAMEPAD_AXIS_* value 999", input_get_gamepad_axis_raw( 0, 999 ) )
expect_error( "raw pad", "argument 'gamepadId'", input_get_gamepad_axis_raw( 10, GAMEPAD_AXIS_LEFT_X ) )

local reported = false

-- Called by Scarab every frame: the checks ran at load time, so on the first frame print OK and quit if none
-- failed; otherwise print every failure, once, and keep running, so --max-frames runs out and the run fails (exit 3).
function on_update( dt )
    if reported then return end
    reported = true

    if #failures == 0 then
        print( "INPUT API SMOKE OK" )
        app_quit()
    else
        for _, message in ipairs( failures ) do
            print( "INPUT API SMOKE FAIL: " .. message )
        end
    end
end
