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
 actions - rebindable controls, saved for the player, and rumble (docs/lua-api/input.html).

 A square runs left and right and jumps - but the game never asks for a
 key: it reads ACTIONS ("left", "right", "jump", input_is_action_active),
 and each action is bound to one key, gamepad button or stick direction.

 The game ships its default bindings in default_bindings.txt (sunlight's
 own text format). At start-up it loads the PLAYER's bindings from the
 save directory if there are any, else those defaults. R rebinds "jump"
 to the next key or pad button pressed; S saves the bindings into the
 save directory (project.json names it: "Scarab-actions-sample" in the
 OS's per-user data folder - app_get_save_directory shows where); D
 reloads the defaults. Saved bindings are back on the next launch.

 Rumble: a pad that can vibrate (input_is_rumble_supported) shakes for a
 moment when its jump is pressed; T shakes every connected pad.
]]

local Hud = dofile( "samples/shared/hud.lua" )

assert( app_set_name( "Scarab - actions sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local __SAVED          <const> = "bindings.txt"                          -- the player's own, in the save directory
local __DEFAULTS       <const> = "samples/actions/default_bindings.txt"  -- shipped with the game
local __ACTIONS        <const> = { "left", "right", "jump" }
local __SQUARE_SIZE    <const> = 40
local __SQUARE_COLOR   <const> = { r = 230, g = 140, b = 50 }
local __OPAQUE         <const> = 255
local __SPEED          <const> = 6      -- pixels per frame
local __JUMP_SPEED     <const> = 14     -- pixels per frame, upwards, when a jump starts
local __GRAVITY        <const> = 1      -- pixels per frame per frame
local __GROUND_HEIGHT  <const> = 3      -- the ground is this many square sizes above the window's bottom
local __JUMP_RUMBLE    <const> = { left = 0.3, right = 0.6, seconds = 0.15 }
local __TEST_RUMBLE    <const> = { left = 1.0, right = 1.0, seconds = 0.5 }
local __PAD_COUNT      <const> = 10     -- pad ids 0..9
local __BINDING_ROW    <const> = 3      -- HUD rows: the three bindings from here,
local __MESSAGE_ROW    <const> = 6      -- then the last message,
local __PAD_ROW        <const> = 7      -- then one row per connected pad, then the save folder

-- The inputs R can rebind "jump" to, with the names the HUD shows for them.
local __KEYS <const> = {
    { KEY_SPACE, "Space" }, { KEY_UP, "Up" }, { KEY_W, "W" }, { KEY_Z, "Z" }, { KEY_X, "X" },
    { KEY_LEFT_CONTROL, "Left Ctrl" }, { KEY_ENTER, "Enter" },
}
local __BUTTONS <const> = {
    { GAMEPAD_BUTTON_RIGHT_FACE_DOWN, "pad A" }, { GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, "pad B" },
    { GAMEPAD_BUTTON_RIGHT_FACE_LEFT, "pad X" }, { GAMEPAD_BUTTON_RIGHT_FACE_UP, "pad Y" },
    { GAMEPAD_BUTTON_LEFT_TRIGGER_1, "pad LB" }, { GAMEPAD_BUTTON_RIGHT_TRIGGER_1, "pad RB" },
}
local __OTHER_KEYS <const> = { [KEY_LEFT] = "Left", [KEY_RIGHT] = "Right" }

local screenWidth  = screen_get_width()
local screenHeight = screen_get_height()
local groundY      = screenHeight - __SQUARE_SIZE * __GROUND_HEIGHT
local square       = { x = ( screenWidth - __SQUARE_SIZE ) / 2, y = groundY, speedY = 0 }
local rebinding    = false
local message      = ""

local function load_bindings()
    if input_load_action_map( __SAVED ) then
        message = "loaded your saved bindings"
    else
        assert( input_load_action_map( __DEFAULTS ) )
        message = "no saved bindings yet: loaded the defaults"
    end
end

load_bindings()

-- What an action is bound to, as the HUD shows it.
local function binding_name( action )
    local kind, code, direction = input_get_action_binding( action )

    if kind == ACTION_BINDING_KEY then
        for _, key in ipairs( __KEYS ) do
            if key[1] == code then return key[2] end
        end

        return __OTHER_KEYS[code] or ( "key " .. code )
    elseif kind == ACTION_BINDING_GAMEPAD_BUTTON then
        for _, button in ipairs( __BUTTONS ) do
            if button[1] == code then return button[2] end
        end

        return "pad button " .. code
    elseif kind == ACTION_BINDING_GAMEPAD_AXIS then
        return "stick axis " .. code .. ( direction > 0 and " +" or " -" )
    end

    return "(not bound)"
end

-- R was pressed: the next listed key or pad button pressed becomes "jump".
local function try_rebind_jump()
    for _, key in ipairs( __KEYS ) do
        if input_is_key_released( key[1] ) then
            local ok, err = input_bind_action_key( "jump", key[1] )

            message   = ok and ( "jump is now " .. key[2] ) or err
            rebinding = false
            return
        end
    end

    for pad = 0, __PAD_COUNT - 1 do
        if input_is_gamepad_available( pad ) then
            for _, button in ipairs( __BUTTONS ) do
                if input_is_gamepad_button_down( pad, button[1] ) then
                    local ok, err = input_bind_action_button( "jump", button[1] )

                    message   = ok and ( "jump is now " .. button[2] ) or err
                    rebinding = false
                    return
                end
            end
        end
    end
end

-- Shake every pad that can, or report why not.
local function rumble_all( rumble )
    local shaken = 0

    for _, pad in ipairs( input_get_connected_gamepads() ) do
        if input_is_rumble_supported( pad.id ) then
            assert( input_set_rumble( pad.id, rumble.left, rumble.right, rumble.seconds ) )
            shaken = shaken + 1
        end
    end

    return shaken
end

function on_update( dt )

    if rebinding then
        try_rebind_jump()
    else
        if input_is_key_released( KEY_R ) then
            rebinding = true
            message   = "press a key or a pad button for jump..."
        elseif input_is_key_released( KEY_S ) then
            local ok, err = input_save_action_map( __SAVED )

            message = ok and "saved - they come back on the next launch" or err
        elseif input_is_key_released( KEY_D ) then
            assert( input_load_action_map( __DEFAULTS ) )
            message = "the defaults again (S to save them)"
        elseif input_is_key_released( KEY_T ) then
            message = "rumble test: " .. rumble_all( __TEST_RUMBLE ) .. " pad(s) shaking"
        end
    end

    -- Movement, through the actions only
    if input_is_action_active( "left" ) then square.x = square.x - __SPEED end
    if input_is_action_active( "right" ) then square.x = square.x + __SPEED end

    square.x = math.max( 0, math.min( screenWidth - __SQUARE_SIZE, square.x ) )

    -- A jump starts only on the ground; a pad's jump also shakes that pad.
    local onGround = ( square.y >= groundY )
    local jumpPad  = nil

    for pad = 0, __PAD_COUNT - 1 do
        if input_is_gamepad_available( pad ) and input_is_action_active( "jump", pad ) then jumpPad = pad end
    end

    if onGround and not rebinding and ( input_is_action_active( "jump" ) or jumpPad ) then
        square.speedY = -__JUMP_SPEED

        if jumpPad and input_is_rumble_supported( jumpPad ) then
            assert( input_set_rumble( jumpPad, __JUMP_RUMBLE.left, __JUMP_RUMBLE.right, __JUMP_RUMBLE.seconds ) )
        end
    end

    square.speedY = square.speedY + __GRAVITY
    square.y      = math.min( groundY, square.y + square.speedY )

    if square.y >= groundY then square.speedY = 0 end

    draw_filled_rectangle( math.floor( square.x ), math.floor( square.y ), __SQUARE_SIZE, __SQUARE_SIZE,
        __SQUARE_COLOR.r, __SQUARE_COLOR.g, __SQUARE_COLOR.b, __OPAQUE )

    -- HUD
    Hud.line( "actions sample - the square reads ACTIONS, not keys", 1, Hud.TITLE )
    Hud.line( "R rebind jump   S save   D defaults   T rumble test", 2, Hud.TITLE )

    for index, action in ipairs( __ACTIONS ) do
        Hud.line( action .. ": " .. binding_name( action ), __BINDING_ROW + index - 1, Hud.INFO )
    end

    Hud.line( message, __MESSAGE_ROW, rebinding and Hud.ALERT or Hud.INFO )

    local pads = input_get_connected_gamepads()

    for index, pad in ipairs( pads ) do
        Hud.line( "pad " .. pad.id .. ": " .. pad.name .. ( input_is_rumble_supported( pad.id ) and "  (can rumble)" or "  (no rumble)" ),
            __PAD_ROW + index - 1, Hud.DIM )
    end

    local folder = app_get_save_directory()

    Hud.lines( "saves: " .. tostring( folder ), __PAD_ROW + #pads, Hud.DIM )
end
