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

 Two squares, one per player. The keyboard (arrows/WASD) always drives
 player 1. A gamepad joins by pressing A (GAMEPAD_BUTTON_RIGHT_FACE_DOWN):
 it claims the next free player slot (input_claim_player_slot), and from
 then on its left stick and D-pad drive that player's square; B
 (RIGHT_FACE_RIGHT) leaves the slot again. Unplugging a pad keeps its
 slot - plugged back in, it is still that player.

 The HUD lists the connected pads (input_get_connected_gamepads) and the
 last hot-plug event (input_set_gamepad_connection_handler, which also
 reports the pads already connected at start-up). For player 1's pad it
 shows the stick both ways: input_get_gamepad_axis applies the pad's dead
 zone (a stick at rest reads exactly 0), input_get_gamepad_axis_raw does
 not; [ and ] change that dead zone for every pad.

 NOTE: raylib's own gamepad button naming (which this engine's
 GAMEPAD_BUTTON_* constants mirror exactly) calls the D-pad
 "LEFT_FACE" (GAMEPAD_BUTTON_LEFT_FACE_UP/DOWN/LEFT/RIGHT) - the face
 buttons (A/B/X/Y or equivalent) are "RIGHT_FACE". Easy to mix up the
 two at a glance.
]]

local Hud = dofile( "samples/shared/hud.lua" )

assert( app_set_name( "Scarab - input sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local __PLAYERS        <const> = 2      -- player slots 1 and 2
local __PAD_COUNT      <const> = 10     -- pad ids 0..9
local __SQUARE_SIZE    <const> = 40
local __SQUARE_COLORS  <const> = { { r = 60, g = 160, b = 220 }, { r = 230, g = 140, b = 50 } }
local __OPAQUE         <const> = 255
local __SPEED          <const> = 6      -- pixels/frame - fine for a short demo, see CLAUDE.md's dt gotcha for why real games shouldn't scale movement by dt's fake fixed value either
local __DEADZONE_STEP  <const> = 0.05
local __DEADZONE_START <const> = 0.1    -- the engine's default
local __STICKS         <const> = { GAMEPAD_AXIS_LEFT_X, GAMEPAD_AXIS_LEFT_Y, GAMEPAD_AXIS_RIGHT_X, GAMEPAD_AXIS_RIGHT_Y }
local __PAD_ROW        <const> = 5      -- the first HUD row of the pad list

local screenWidth  = screen_get_width()
local screenHeight = screen_get_height()
local deadzone     = __DEADZONE_START
local lastEvent    = "none yet"

-- Player n's square starts at the n-th third of the screen's width.
local function start_pos( player )
    return screenWidth * player / ( __PLAYERS + 1 ) - __SQUARE_SIZE / 2, ( screenHeight - __SQUARE_SIZE ) / 2
end

local squares = {}

for player = 1, __PLAYERS do
    local x, y = start_pos( player )

    squares[player] = { x = x, y = y }
end

-- Every connect and disconnect; the first frame reports the pads already plugged in.
assert( input_set_gamepad_connection_handler( function( gamepadId, connected )
    lastEvent = "pad " .. gamepadId .. ( connected and " connected" or " disconnected" )
end ) )

-- Set every pad's stick dead zone to `value` (kept within 0..1), for both sticks of all __PAD_COUNT pads.
local function set_deadzone( value )
    deadzone = math.max( 0, math.min( 1, value ) )

    for pad = 0, __PAD_COUNT - 1 do
        for _, axis in ipairs( __STICKS ) do
            input_set_axis_deadzone( pad, axis, deadzone )
        end
    end
end

-- The first slot no pad holds, or nil.
local function free_slot()
    for player = 1, __PLAYERS do
        if input_get_gamepad_for_slot( player ) == false then return player end
    end

    return nil
end

-- Move a player's square by (dx, dy) pixels, kept inside the screen.
local function move( square, dx, dy )
    square.x = math.max( 0, math.min( screenWidth - __SQUARE_SIZE, square.x + dx ) )
    square.y = math.max( 0, math.min( screenHeight - __SQUARE_SIZE, square.y + dy ) )
end

-- Player 1's step this frame from the keyboard (arrows/WASD), as dx, dy in pixels.
local function keyboard_step()
    local dx, dy = 0, 0

    if input_is_key_down( KEY_LEFT ) or input_is_key_down( KEY_A ) then dx = dx - __SPEED end
    if input_is_key_down( KEY_RIGHT ) or input_is_key_down( KEY_D ) then dx = dx + __SPEED end
    if input_is_key_down( KEY_UP ) or input_is_key_down( KEY_W ) then dy = dy - __SPEED end
    if input_is_key_down( KEY_DOWN ) or input_is_key_down( KEY_S ) then dy = dy + __SPEED end

    return dx, dy
end

-- A pad's step: the stick (the automatic read: 0 inside the dead zone, so no threshold here)
-- plus the D-pad, a digital input.
local function pad_step( pad )
    local dx = input_get_gamepad_axis( pad, GAMEPAD_AXIS_LEFT_X ) * __SPEED
    local dy = input_get_gamepad_axis( pad, GAMEPAD_AXIS_LEFT_Y ) * __SPEED

    if input_is_gamepad_button_down( pad, GAMEPAD_BUTTON_LEFT_FACE_LEFT ) then dx = dx - __SPEED end
    if input_is_gamepad_button_down( pad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT ) then dx = dx + __SPEED end
    if input_is_gamepad_button_down( pad, GAMEPAD_BUTTON_LEFT_FACE_UP ) then dy = dy - __SPEED end
    if input_is_gamepad_button_down( pad, GAMEPAD_BUTTON_LEFT_FACE_DOWN ) then dy = dy + __SPEED end

    return dx, dy
end

-- Called by Scarab every frame (dt is a fixed step, not measured time): let pads join (A) and leave (B),
-- change the dead zone ([ and ]), move each player's square (player 1 by keyboard, the others by their pad),
-- reset them (R), and draw the squares and the HUD.
function on_update( dt )

    local pads = input_get_connected_gamepads()

    -- Joining and leaving
    for _, pad in ipairs( pads ) do
        local slot = input_get_slot_for_gamepad( pad.id )

        if slot == false and input_is_gamepad_button_down( pad.id, GAMEPAD_BUTTON_RIGHT_FACE_DOWN ) then
            local player = free_slot()

            if player then
                input_claim_player_slot( player, pad.id )
            end
        elseif slot ~= false and input_is_gamepad_button_down( pad.id, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT ) then
            input_release_player_slot( slot )
        end
    end

    if input_is_key_released( KEY_LEFT_BRACKET ) then set_deadzone( deadzone - __DEADZONE_STEP ) end
    if input_is_key_released( KEY_RIGHT_BRACKET ) then set_deadzone( deadzone + __DEADZONE_STEP ) end

    -- Movement: the keyboard is always player 1; each claimed pad drives its own player.
    move( squares[1], keyboard_step() )

    for player = 1, __PLAYERS do
        local pad = input_get_gamepad_for_slot( player )

        if pad ~= false and input_is_gamepad_available( pad ) then
            move( squares[player], pad_step( pad ) )
        end
    end

    if input_is_key_released( KEY_R ) then
        for player = 1, __PLAYERS do
            squares[player].x, squares[player].y = start_pos( player )
        end
    end

    for player = 1, __PLAYERS do
        local color = __SQUARE_COLORS[player]

        draw_filled_rectangle( math.floor( squares[player].x ), math.floor( squares[player].y ), __SQUARE_SIZE, __SQUARE_SIZE,
            color.r, color.g, color.b, __OPAQUE )
    end

    -- HUD
    Hud.line( "input sample - arrows/WASD move player 1 (blue); a pad's A joins, B leaves", 1, Hud.TITLE )
    Hud.line( string.format( "R resets   [ / ] stick dead zone: %.2f", deadzone ), 2, Hud.TITLE )

    local players = {}

    for player = 1, __PLAYERS do
        local pad = input_get_gamepad_for_slot( player )
        local who = ( pad == false ) and "press A on a pad" or ( "pad " .. pad .. ( input_is_gamepad_available( pad ) and "" or " (unplugged)" ) )

        players[#players + 1] = "player " .. player .. ": " .. who
    end

    Hud.line( table.concat( players, "   " ), 3, Hud.INFO )
    Hud.line( "last hot-plug event: " .. lastEvent, 4, Hud.INFO )

    if #pads == 0 then
        Hud.line( "no gamepad connected", __PAD_ROW, Hud.DIM )
    else
        for index, pad in ipairs( pads ) do
            local slot = input_get_slot_for_gamepad( pad.id )

            Hud.line( "pad " .. pad.id .. ": " .. pad.name .. ( slot == false and "" or ( "  (player " .. slot .. ")" ) ), __PAD_ROW + index - 1, Hud.DIM )
        end
    end

    local firstPad = input_get_gamepad_for_slot( 1 )

    if firstPad ~= false then
        Hud.line_from_bottom( string.format( "player 1 stick: automatic %.2f, %.2f   raw %.2f, %.2f",
            input_get_gamepad_axis( firstPad, GAMEPAD_AXIS_LEFT_X ), input_get_gamepad_axis( firstPad, GAMEPAD_AXIS_LEFT_Y ),
            input_get_gamepad_axis_raw( firstPad, GAMEPAD_AXIS_LEFT_X ), input_get_gamepad_axis_raw( firstPad, GAMEPAD_AXIS_LEFT_Y ) ), 1, Hud.INFO )
    end
end
