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
 saves - a game's options and high scores, kept between runs (docs/lua-api/save.html).

 Two saves, each a plain Lua table kept as JSON in the save directory
 (project.json names it: "Scarab-saves-sample" in the OS's per-user data
 folder - the HUD shows where):

   options.json  { volume = 0..10, show_fps = true/false, color = 1..n }
   scores.json   { { name = "P1", score = 1234 }, ... } - the best five

 At start-up each is read back with save_read, or the defaults are used
 when there is none yet. Left/Right change the volume, F shows or hides
 the FPS counter, C changes the square's colour, Space plays a round (a
 random score, kept if it is among the best five). S saves both, X
 deletes both (the defaults come back on the next launch). Quit, launch
 again, and what you saved is still there.
]]

local Hud = dofile( "samples/shared/hud.lua" )

assert( app_set_name( "Scarab - saves sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence.
sp_wait( 1 )

local __OPTIONS       <const> = "options.json"
local __SCORES        <const> = "scores.json"
local __VOLUME_MAX    <const> = 10
local __BEST_COUNT    <const> = 5        -- how many scores are kept
local __SCORE_MIN     <const> = 100      -- a round's random score range
local __SCORE_MAX     <const> = 9999
local __PLAYERS       <const> = { "P1", "P2", "P3" }
local __COLORS        <const> = {
    { name = "orange", r = 230, g = 140, b = 50 },
    { name = "teal",   r = 40,  g = 190, b = 170 },
    { name = "violet", r = 150, g = 90,  b = 220 },
}
local __OPAQUE        <const> = 255
local __SQUARE_SIZE   <const> = 120
local __OPTIONS_ROW   <const> = 4        -- HUD rows: the options from here,
local __SCORES_ROW    <const> = 8        -- then the best scores,
local __MESSAGE_ROW   <const> = 15       -- then the last message and the save folder
local __DEFAULT_OPTIONS <const> = { volume = 5, show_fps = false, color = 1 }

local screenWidth  = screen_get_width()
local screenHeight = screen_get_height()
local message      = ""
local round        = 0

-- A fresh copy of the default options (a <const> table must not be changed in place).
local function default_options()
    return { volume = __DEFAULT_OPTIONS.volume, show_fps = __DEFAULT_OPTIONS.show_fps, color = __DEFAULT_OPTIONS.color }
end

-- Read a save, or nil with what happened for the HUD.
local function read_or_default( name, default )
    local value, err = save_read( name )

    if value then return value, name .. " loaded" end

    return default, err
end

local options, optionsNote = read_or_default( __OPTIONS, default_options() )
local scores, scoresNote   = read_or_default( __SCORES, {} )

message = optionsNote .. ", " .. scoresNote
assert( app_set_draw_fps( options.show_fps ) )

-- Keep a round's score if it is among the best __BEST_COUNT.
local function add_score( name, score )
    scores[#scores + 1] = { name = name, score = score }
    table.sort( scores, function( a, b ) return a.score > b.score end )

    while #scores > __BEST_COUNT do table.remove( scores ) end
end

-- Save both tables, or report the first failure.
local function save_all()
    local ok, err = save_write( __OPTIONS, options )

    if ok then ok, err = save_write( __SCORES, scores ) end

    return ok and "saved - they are back on the next launch" or err
end

-- Delete both saves (a missing one is not an error here).
local function delete_all()
    for _, name in ipairs( { __OPTIONS, __SCORES } ) do
        if save_exists( name ) then assert( save_delete( name ) ) end
    end

    return "saves deleted - the defaults come back on the next launch"
end

function on_update( dt )

    if input_is_key_released( KEY_LEFT ) then options.volume = math.max( 0, options.volume - 1 ) end
    if input_is_key_released( KEY_RIGHT ) then options.volume = math.min( __VOLUME_MAX, options.volume + 1 ) end

    if input_is_key_released( KEY_F ) then
        options.show_fps = not options.show_fps
        assert( app_set_draw_fps( options.show_fps ) )
    end

    if input_is_key_released( KEY_C ) then options.color = options.color % #__COLORS + 1 end

    if input_is_key_released( KEY_SPACE ) then
        round = round + 1

        local player = __PLAYERS[( round - 1 ) % #__PLAYERS + 1]
        local score  = math.random( __SCORE_MIN, __SCORE_MAX )

        add_score( player, score )
        message = player .. " scored " .. score
    end

    if input_is_key_released( KEY_S ) then message = save_all() end
    if input_is_key_released( KEY_X ) then message = delete_all() end

    local color = __COLORS[options.color] or __COLORS[1]

    draw_filled_rectangle( screenWidth - __SQUARE_SIZE * 2, screenHeight - __SQUARE_SIZE * 2, __SQUARE_SIZE, __SQUARE_SIZE,   -- bottom right, below the HUD
        color.r, color.g, color.b, __OPAQUE )

    -- HUD
    Hud.line( "saves sample - options and high scores kept between runs", 1, Hud.TITLE )
    Hud.line( "Left/Right volume   F FPS   C colour   Space play a round   S save   X delete", 2, Hud.TITLE )

    Hud.line( "volume: " .. string.rep( "#", options.volume ) .. string.rep( ".", __VOLUME_MAX - options.volume ) ..
              " " .. options.volume, __OPTIONS_ROW, Hud.INFO )
    Hud.line( "show FPS: " .. tostring( options.show_fps ), __OPTIONS_ROW + 1, Hud.INFO )
    Hud.line( "colour: " .. color.name, __OPTIONS_ROW + 2, Hud.INFO )

    Hud.line( "best scores:", __SCORES_ROW, Hud.INFO )

    for index = 1, __BEST_COUNT do
        local entry = scores[index]

        Hud.line( index .. ". " .. ( entry and ( entry.name .. "  " .. entry.score ) or "---" ), __SCORES_ROW + index, Hud.DIM )
    end

    Hud.line( message, __MESSAGE_ROW, Hud.ALERT )
    Hud.lines( "saves: " .. tostring( app_get_save_directory() ), __MESSAGE_ROW + 1, Hud.DIM )
end
