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

local CHECK_FRAME     = 300     -- 300 frames at 60 fps = exactly 5 virtual seconds
local EXPECTED_SECS   = 5.0
local TOLERANCE_SECS  = 0.001

local frames        = 0
local os_time_start = os.time()

function on_load_stage( stageId )
    return true
end

function on_update( dt )

    frames = frames + 1

    if frames == CHECK_FRAME then
        local app_time      = app_get_time()
        local os_clock      = os.clock()
        local os_time_delta = os.time() - os_time_start
        local idle_input    = ( not input_is_key_down( KEY_SPACE ) )
                              and ( not input_is_gamepad_button_down( 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) )
                              and ( input_get_gamepad_axis( 0, 0 ) == 0.0 )

        local ok = math.abs( app_time - EXPECTED_SECS ) < TOLERANCE_SECS
               and math.abs( os_clock - EXPECTED_SECS ) < TOLERANCE_SECS
               and os_time_delta == EXPECTED_SECS
               and idle_input

        print( string.format( "HEADLESS SMOKE %s: frame=%d app_get_time=%.6f os.clock=%.6f os.time_delta=%d idle_input=%s",
            ok and "OK" or "FAIL", frames, app_time, os_clock, os_time_delta, tostring( idle_input ) ) )

        if ok then
            app_quit()
        end
    end
end

sp_wait( 1 )
