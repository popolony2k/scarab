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
    Headless smoke test - run by ci.yml (every platform, straight after the
    build) as:

        scarab --headless --fast --max-frames 1000 scripts/headless_smoke/project.json

    It checks the parts of --headless that have to hold on every platform
    with no display: the virtual clock advances exactly 1/60 s per frame
    (app_get_time), Lua's own os.clock()/os.time() follow it, and input
    reads as "nothing pressed".

    Success is the script quitting ITSELF (app_quit -> exit 0). On any
    mismatch it prints the numbers and deliberately does NOT quit, so the
    --max-frames budget runs out and scarab exits 3 - failing the CI step
    with the offending values in its log, rather than depending on a
    separate log-grepping step.
]]

io.stdout:setvbuf( "line" )

local __CHECK_FRAME    <const> = 300     -- 300 frames at 60 fps = exactly 5 virtual seconds
local __EXPECTED_SECS  <const> = 5.0
local __TOLERANCE_SECS <const> = 0.001

local frames        = 0
local osTimeStart = os.time()

function on_load_stage( stageId )
    return true
end

function on_update( dt )

    frames = frames + 1

    if frames == __CHECK_FRAME then
        local appTime      = app_get_time()
        local osClock      = os.clock()
        local osTimeDelta = os.time() - osTimeStart
        local idleInput    = ( not input_is_key_down( KEY_SPACE ) )
                              and ( not input_is_gamepad_button_down( 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) )
                              and ( input_get_gamepad_axis( 0, 0 ) == 0.0 )

        local ok = math.abs( appTime - __EXPECTED_SECS ) < __TOLERANCE_SECS
               and math.abs( osClock - __EXPECTED_SECS ) < __TOLERANCE_SECS
               and osTimeDelta == __EXPECTED_SECS
               and idleInput

        print( string.format( "HEADLESS SMOKE %s: frame=%d app_get_time=%.6f os.clock=%.6f os.time_delta=%d idle_input=%s",
            ok and "OK" or "FAIL", frames, appTime, osClock, osTimeDelta, tostring( idleInput ) ) )

        if ok then
            app_quit()
        end
    end
end

sp_wait( 1 )
