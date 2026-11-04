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
    Action map smoke test - run by scripts/action_map_smoke.py (and ci.yml, every platform), which picks
    the scenario through SMOKE_SCENARIO and passes the folders it expects:

        api       --save-dir <T>: the command line wins over project.json and app_set_save_directory;
                  every binding type, the conflict refusal, unbind, get; save -> unbind -> load; a failed
                  load changes nothing; rumble unsupported headless; every error; and that the save
                  directory, binding and file functions create no renderer
        priority  no --save-dir, HOME/APPDATA/XDG_DATA_HOME pointed at a temp folder by the driver:
                  project.json's "save_directory" is used, app_set_save_directory then wins
        nodir     a project with no "save_directory": no save directory, a plain save is refused, an
                  hpath:// save still works
        default / saved   the game shipped as a .zip (and an encrypted pack): the first run has no save,
                  loads the default shipped under APP_DIR, rebinds and saves; the second run loads the
                  save

    Pins plans/MASTER_PLAN.md item C1b. The script quits ITSELF only if every check passed (exit 0);
    otherwise it prints every failure and never quits, so --max-frames runs out and scarab exits 3.
]]

io.stdout:setvbuf( "line" )

local __SAVES       <const> = "bindings.txt"                  -- the player's bindings, in the save directory
local __DEFAULTS    <const> = "/default_bindings.txt"         -- shipped with the game, under APP_DIR
local __SHIPPED_KEY <const> = KEY_A                           -- what the shipped defaults bind "jump" to
local __SAVED_KEY   <const> = KEY_B                           -- what the first archive run rebinds it to

local scenario = os.getenv( "SMOKE_SCENARIO" ) or "(unset)"
local failures = {}

local function check( ok, message )
    if not ok then failures[#failures + 1] = scenario .. ": " .. message end
end

-- a failing call returns nil + a message containing `needle`
local function expect_error( label, needle, value, message )
    check( value == nil, label .. ": expected nil, got " .. tostring( value ) )
    check( type( message ) == "string" and message:find( needle, 1, true ) ~= nil,
           label .. ": message '" .. tostring( message ) .. "' lacks '" .. needle .. "'" )
end

local function binding_is( label, name, wantType, wantCode, wantDirection )
    local kind, code, direction = input_get_action_binding( name )

    check( kind == wantType and code == wantCode and direction == wantDirection,
           string.format( "%s: %s is %s,%s,%s - expected %s,%s,%s", label, name, tostring( kind ), tostring( code ),
                          tostring( direction ), wantType, wantCode, wantDirection ) )
end

function on_load_stage( stageId ) return true end
sp_wait( 1 )

if scenario == "api" then
    local expected = os.getenv( "SMOKE_EXPECT_DIR" )

    check( app_get_save_directory() == expected, "--save-dir must be the save directory, got " .. tostring( app_get_save_directory() ) )
    check( app_set_save_directory( "Other" ) == expected, "with --save-dir, app_set_save_directory must change nothing and return it" )
    expect_error( "bad save name", "may hold only letters, digits", app_set_save_directory( "../escape" ) )
    expect_error( "empty save name", "can't be empty", app_set_save_directory( "" ) )
    expect_error( "save name type", "argument 'name'", app_set_save_directory( 7 ) )

    -- every binding type, and get
    check( input_bind_action_key( "jump", KEY_SPACE ) == true, "bind key" )
    check( input_bind_action_button( "fire", GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) == true, "bind button" )
    check( input_bind_action_axis( "left", GAMEPAD_AXIS_LEFT_X, -1 ) == true, "bind axis" )
    check( input_bind_action_axis( "brake", GAMEPAD_AXIS_LEFT_TRIGGER, 1 ) == true, "bind a trigger" )
    binding_is( "key", "jump", ACTION_BINDING_KEY, KEY_SPACE, 0 )
    binding_is( "button", "fire", ACTION_BINDING_GAMEPAD_BUTTON, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, 0 )
    binding_is( "axis", "left", ACTION_BINDING_GAMEPAD_AXIS, GAMEPAD_AXIS_LEFT_X, -1 )
    binding_is( "unbound", "nothing", ACTION_BINDING_NONE, 0, 0 )

    -- one input, one action; rebinding an action replaces its binding; the same pair again is fine
    expect_error( "key conflict", "already bound to another action", input_bind_action_key( "duck", KEY_SPACE ) )
    expect_error( "button conflict", "already bound to another action", input_bind_action_button( "use", GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) )
    expect_error( "axis conflict", "already bound to another action", input_bind_action_axis( "west", GAMEPAD_AXIS_LEFT_X, -1 ) )
    check( input_bind_action_axis( "right", GAMEPAD_AXIS_LEFT_X, 1 ) == true, "the other direction of the same axis is another input" )
    check( input_bind_action_key( "jump", KEY_SPACE ) == true, "the same pair again" )
    check( input_bind_action_key( "jump", KEY_UP ) == true, "rebind jump" )
    binding_is( "rebound", "jump", ACTION_BINDING_KEY, KEY_UP, 0 )
    check( input_bind_action_key( "duck", KEY_SPACE ) == true, "KEY_SPACE is free again" )

    -- save -> unbind -> load restores; a failed load changes nothing
    check( input_save_action_map( __SAVES ) == true, "save" )
    check( input_unbind_action( "jump" ) == true and input_unbind_action( "fire" ) == true, "unbind" )
    binding_is( "after unbind", "jump", ACTION_BINDING_NONE, 0, 0 )
    check( input_unbind_action( "never-bound" ) == true, "unbinding an unbound action is a no-op success" )
    check( input_load_action_map( __SAVES ) == true, "load" )
    binding_is( "after load", "jump", ACTION_BINDING_KEY, KEY_UP, 0 )
    binding_is( "after load", "fire", ACTION_BINDING_GAMEPAD_BUTTON, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, 0 )
    binding_is( "after load", "left", ACTION_BINDING_GAMEPAD_AXIS, GAMEPAD_AXIS_LEFT_X, -1 )
    expect_error( "missing file", "the bindings are unchanged", input_load_action_map( "missing.txt" ) )
    binding_is( "after a failed load", "jump", ACTION_BINDING_KEY, KEY_UP, 0 )

    -- argument errors
    expect_error( "name =", "may hold only letters, digits", input_bind_action_key( "a=b", KEY_X ) )
    expect_error( "empty name", "can't be empty", input_bind_action_key( "", KEY_X ) )
    expect_error( "name type", "argument 'name'", input_get_action_binding( 3 ) )
    expect_error( "bad key", "unknown KEY_* value 99999", input_bind_action_key( "x", 99999 ) )
    expect_error( "bad button", "GAMEPAD_BUTTON_*", input_bind_action_button( "x", 999 ) )
    expect_error( "bad axis", "GAMEPAD_AXIS_*", input_bind_action_axis( "x", 999, 1 ) )
    expect_error( "direction 0", "must be 1 or -1", input_bind_action_axis( "x", GAMEPAD_AXIS_LEFT_Y, 0 ) )
    expect_error( "direction 2", "argument 'direction'", input_bind_action_axis( "x", GAMEPAD_AXIS_LEFT_Y, 2 ) )
    expect_error( "path type", "argument 'path'", input_save_action_map( nil ) )

    check( renderer_get_current() == nil, "the save directory, binding and file functions created a renderer" )

    -- reading input and rumble need the window (headless: no pad, nothing rumbles)
    check( input_is_action_active( "jump" ) == false, "headless: nothing is pressed" )
    check( input_is_action_active( "left", 3, 0.25 ) == false, "pad and threshold arguments" )
    check( input_is_action_active( "never-bound" ) == false, "an unbound action is never active" )
    expect_error( "active pad", "argument 'gamepadId'", input_is_action_active( "jump", 10 ) )
    expect_error( "active threshold", "argument 'threshold'", input_is_action_active( "jump", 0, 1.5 ) )
    check( input_is_rumble_supported( 0 ) == false, "headless: no pad can rumble" )
    expect_error( "rumble unsupported", "gamepad 0 can't rumble", input_set_rumble( 0, 1, 1, 0.5 ) )
    check( input_stop_rumble( 0 ) == true, "stopping a pad that isn't rumbling is a no-op success" )
    expect_error( "rumble strength", "argument 'left'", input_set_rumble( 0, 1.5, 1, 0.5 ) )
    expect_error( "rumble seconds", "must be more than 0", input_set_rumble( 0, 1, 1, 0 ) )
    expect_error( "rumble pad", "argument 'gamepadId'", input_is_rumble_supported( 10 ) )

elseif scenario == "priority" then
    check( app_get_save_directory() == os.getenv( "SMOKE_EXPECT_PROJECT" ),
           "project.json's name must be used, got " .. tostring( app_get_save_directory() ) )
    check( app_set_save_directory( "FromScript" ) == os.getenv( "SMOKE_EXPECT_SCRIPT" ), "app_set_save_directory must win over project.json" )
    check( app_get_save_directory() == os.getenv( "SMOKE_EXPECT_SCRIPT" ), "and stay" )
    check( input_bind_action_key( "jump", KEY_SPACE ) == true and input_save_action_map( __SAVES ) == true, "save into it" )

elseif scenario == "nodir" then
    expect_error( "no save directory", "no save directory", app_get_save_directory() )
    check( input_bind_action_key( "jump", KEY_SPACE ) == true, "bind" )
    expect_error( "plain save refused", "no save directory", input_save_action_map( __SAVES ) )
    local hostFile = "hpath://" .. os.getenv( "SMOKE_HOST_FILE" )
    check( input_save_action_map( hostFile ) == true, "an hpath:// save needs no save directory" )
    check( input_unbind_action( "jump" ) == true and input_load_action_map( hostFile ) == true, "and loads back" )
    binding_is( "hpath round trip", "jump", ACTION_BINDING_KEY, KEY_SPACE, 0 )

elseif scenario == "default" then
    check( input_load_action_map( __SAVES ) == nil, "a fresh save directory has no saved bindings" )
    check( input_load_action_map( APP_DIR .. __DEFAULTS ) == true, "the defaults shipped in the archive must load" )
    binding_is( "shipped default", "jump", ACTION_BINDING_KEY, __SHIPPED_KEY, 0 )
    check( input_bind_action_key( "jump", __SAVED_KEY ) == true and input_save_action_map( __SAVES ) == true, "rebind and save" )

elseif scenario == "saved" then
    check( input_load_action_map( __SAVES ) == true, "the second run must find the save" )
    binding_is( "the player's save", "jump", ACTION_BINDING_KEY, __SAVED_KEY, 0 )

else
    check( false, "unknown SMOKE_SCENARIO" )
end

local reported = false

function on_update( dt )
    if reported then return end
    reported = true

    if #failures == 0 then
        print( "ACTION MAP SMOKE OK" )
        app_quit()
    else
        for _, message in ipairs( failures ) do
            print( "ACTION MAP SMOKE FAIL: " .. message )
        end
    end
end
