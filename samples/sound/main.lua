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
 sound - direct sound effects and the three ways to play a "song"
 (docs/lua-api/sound.html), all against the same loaded audio.

 Demonstrates sound_load/sound_is_playing/sound_set_volume, and all
 three Play forms from the song-commands comparison table: play_song
 (direct, one-shot), play_song_looping (direct, tracked/looping), and
 sp_play_song (queued, also tracked/looping) - plus pause_song/
 stop_song/resume_song, which cover both tracked forms identically.
]]

local Hud = dofile( "samples/shared/hud.lua" )

assert( app_set_name( "Scarab - sound sample" ) )

-- See samples/hello-world/docs/README.md for why every entry script needs
-- at least one sp_* command, even one with nothing to sequence - nothing
-- else here is queued unconditionally at load time (sp_play_song only
-- runs from a key press, inside on_update).
sp_wait( 1 )

local __SOUND_ID    <const> = 1
local __VOLUME_STEP <const> = 0.1

-- Not asserted: with no audio device (eg. a CI machine) the load fails, and the sample still runs and
-- says so on its HUD.
local soundLoaded, soundError = sound_load( __SOUND_ID, "resources/sounds/stage_theme.wav" )

local volume = 1.0

if soundLoaded then
    sound_set_volume( __SOUND_ID, volume )
end

function on_update( dt )

    if input_is_key_released( KEY_ONE ) then
        play_song( __SOUND_ID )          -- direct, one-shot - never auto-repeats
    end

    if input_is_key_released( KEY_TWO ) then
        play_song_looping( __SOUND_ID )  -- direct, but tracked - engine auto-replays it once it ends
    end

    if input_is_key_released( KEY_THREE ) then
        sp_play_song( __SOUND_ID )       -- queued (participates in the sp_* command queue) - also tracked
    end

    if input_is_key_released( KEY_P ) then
        pause_song( __SOUND_ID )         -- covers both tracked forms (2 and 3) identically
    end

    if input_is_key_released( KEY_O ) then
        stop_song( __SOUND_ID )
    end

    if input_is_key_released( KEY_R ) then
        resume_song( __SOUND_ID )
    end

    if input_is_key_released( KEY_UP ) then
        volume = math.min( 1.0, volume + __VOLUME_STEP )
        sound_set_volume( __SOUND_ID, volume )
    end

    if input_is_key_released( KEY_DOWN ) then
        volume = math.max( 0.0, volume - __VOLUME_STEP )
        sound_set_volume( __SOUND_ID, volume )
    end

    Hud.line( "sound sample - 1/2/3 play (one-shot/looping/queued)", 1, Hud.TITLE )
    Hud.line( "P pause, O stop, R resume, Up/Down volume", 2, Hud.TITLE )

    if not soundLoaded then
        Hud.line( "sound_load FAILED: " .. tostring( soundError ), 3, Hud.ALERT )

        return
    end

    Hud.line( "sound_is_playing: " .. tostring( sound_is_playing( __SOUND_ID ) )
        .. "   volume: " .. string.format( "%.1f", volume ), 3, Hud.INFO )
end
