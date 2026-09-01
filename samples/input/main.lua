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
 input - keyboard and gamepad reading (docs/lua-api/input.html).

 Moves a plain rectangle (no sprite yet - see the "sprite" sample) around
 the screen with the arrow keys/WASD, or gamepad 0's left stick (with a
 deadzone, the same pattern input_is_gamepad_button_down's own doc
 example uses), and resets it's position on a keyboard key release or a
 gamepad face-down button press. input_add_gamepad must be called once,
 at startup, before any gamepad_* primitive reports real state for that
 id - it's harmless to call this and then read gamepad state every
 frame even with no gamepad actually connected, everything just reads
 as centered/not-pressed.
]]

app_set_name( "Scarab - input sample" )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

input_add_gamepad( 0 )

local screenWidth  = screen_get_width()
local screenHeight = screen_get_height()

local __SQUARE_SIZE = 40
local __SPEED        = 6  -- pixels/frame - fine for a short demo, see CLAUDE.md's dt gotcha for why real games shouldn't scale movement by dt's fake fixed value either
local __STICK_DEADZONE = 0.1  -- raw axis values jitter near 0 even when the stick is physically centered

local function startPos()
    return ( screenWidth - __SQUARE_SIZE ) / 2, ( screenHeight - __SQUARE_SIZE ) / 2
end

local squareX, squareY = startPos()

function on_update( dt )

    local axisX = input_get_gamepad_axis( 0, GAMEPAD_AXIS_LEFT_X )
    local axisY = input_get_gamepad_axis( 0, GAMEPAD_AXIS_LEFT_Y )
    local faceDown = input_is_gamepad_button_down( 0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN )

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then
        squareX = squareX - __SPEED
    end

    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then
        squareX = squareX + __SPEED
    end

    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then
        squareY = squareY - __SPEED
    end

    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then
        squareY = squareY + __SPEED
    end

    -- Same movement, driven by the left stick instead - deadzone first,
    -- since a raw axis value jitters near 0 even when physically centered.
    if math.abs( axisX ) > __STICK_DEADZONE then
        squareX = squareX + ( axisX * __SPEED )
    end

    if math.abs( axisY ) > __STICK_DEADZONE then
        squareY = squareY + ( axisY * __SPEED )
    end

    -- Keep the square fully on screen.
    squareX = math.max( 0, math.min( screenWidth - __SQUARE_SIZE, squareX ) )
    squareY = math.max( 0, math.min( screenHeight - __SQUARE_SIZE, squareY ) )

    if input_is_key_released( KEY_R ) or faceDown then
        squareX, squareY = startPos()
    end

    draw_filled_rectangle( math.floor( squareX ), math.floor( squareY ), __SQUARE_SIZE, __SQUARE_SIZE, 60, 160, 220, 255 )

    draw_text( "input sample - arrows/WASD or left stick to move, R or face-down button to reset", 20, 20, 20, 255, 255, 255, 255 )
    draw_text( "gamepad 0 left stick: " .. string.format( "%.2f, %.2f", axisX, axisY )
        .. "  face-down button: " .. tostring( faceDown ), 20, 50, 18, 200, 200, 200, 255 )
    draw_text( "(reads centered/false with no gamepad connected - that's expected, not an error)", 20, 72, 18, 150, 150, 150, 255 )
end
