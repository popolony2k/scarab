/*
 * Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "lua/luasoundapi.h"
#include "lua/luaengineutil.h"
#include <cstdio>


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @luaname{sound_load(id, path) -> success}
             * @luadoc
             * These are unconditional, immediate calls — no queue
             * involved, and they don't distinguish "background music"
             * from "sound effect." Use these for anything you want to
             * control precisely and immediately.
             *
             * Load a sound file and associate it with `id`. Must be
             * called once before any other `sound_*`/`*_song` call for
             * that id.
             * @luaexample
             * sound_load(1, BASE_PATH .. "audio/global/caravellius-shot.wav")
             */
            int LuaSoundApi :: Load( lua_State *pLuaState )  {

                int         nSoundId   = ( int ) lua_tointeger( pLuaState, 1 );
                const char  *szFileName = lua_tostring( pLuaState, 2 );
                bool        bResult    = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Load( nSoundId, szFileName );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sound_unload(id) -> success}
             * @luadoc
             * Free the sound previously loaded for `id`.
             */
            int LuaSoundApi :: Unload( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Unload( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sound_play(id) -> success}
             * @luadoc
             * Start playing `id` from the beginning (or resume if
             * already loaded and stopped).
             * @luaexample
             * sound_play(1)
             */
            int LuaSoundApi :: Play( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Play( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sound_stop(id) -> success}
             * @luadoc
             * Stop `id` if it's currently playing.
             */
            int LuaSoundApi :: Stop( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Stop( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sound_pause(id) -> success}
             * @luadoc
             * Pause `id` without resetting its playback position.
             */
            int LuaSoundApi :: Pause( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Pause( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sound_resume(id) -> success}
             * @luadoc
             * Resume a previously paused `id`.
             */
            int LuaSoundApi :: Resume( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Resume( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sound_is_playing(id) -> playing}
             * @luaexample
             * if sound_is_playing(1) then
             *   print("still playing")
             * end
             */
            int LuaSoundApi :: IsPlaying( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> IsPlaying( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{sound_set_volume(id, volume) -> success}
             * @luadoc
             * Set `id`'s playback volume, independently of every other
             * loaded sound — `0.0` (silent) to `1.0` (max). Intended for
             * crossfading between two songs (e.g. fading a stage's BGM
             * out while a boss's BGM fades in): ramp each song's volume
             * in opposite directions over several frames from
             * `on_update`.
             * @luaexample
             * sound_set_volume(ID_DESTRUCTION_ALIENS_ATTACK_BGM, 0.5)
             */
            int LuaSoundApi :: SetVolume( lua_State *pLuaState )  {

                int    nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                float  fVolume  = ( float ) lua_tonumber( pLuaState, 2 );
                bool   bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> SetVolume( nSoundId, fVolume );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @brief Call a player routine using direct mode;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @param cmd The command to add to queue;
             */
            void LuaSoundApi :: CallPlayCommandDirect( lua_State *pLuaState, SunLight :: Scripting :: Commands cmd )  {

                if( lua_gettop( pLuaState ) == 1 )  {

                    lua_Integer nParm = lua_tonumber( pLuaState, 1 );
                    SunLight :: Scripting :: IScriptListener *pScriptListener = LuaEngineUtil :: GetScriptProcessor( pLuaState ) -> GetScriptListener();

                    pScriptListener -> OnCommand( cmd, nParm );
                }
                else  {
                    fprintf( stderr, "Invalid number of arguments on song player call.\n" );
                }
            }

            /**
             * @brief Implement the direct Play song routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{play_song(id)}
             * @luagroup{song_commands}
             * @luaheading{Song commands — queued vs. direct}
             * @luadoc
             * Both forms end up calling the same underlying playback, but
             * only the **queued** (`sp_*`) forms — and
             * `play_song_looping` — mark a song as "the currently tracked
             * background music": the engine automatically re-triggers
             * that tracked song every frame once it finishes
             * (`EngineHost::RunScriptMachine`'s BGM-loop check), giving
             * free looping. Plain `play_song` plays once and is never
             * auto-repeated, which is what you want for a one-off sound
             * effect (a shot, an explosion) rather than music.
             *
             * | | Queued (participates in the `sp_*` command queue, becomes the looping BGM) | Direct, looping (immediate, still becomes the looping BGM) | Direct, one-shot (immediate, never looped) |
             * | --- | --- | --- | --- |
             * | Play | `sp_play_song(id)` | `play_song_looping(id)` | `play_song(id)` |
             * | Pause | `sp_pause_song(id)` | — | `pause_song(id)` |
             * | Stop | `sp_stop_song(id)` | — | `stop_song(id)` |
             * | Resume | `sp_resume_song(id)` | — | `resume_song(id)` |
             *
             * ```lua
             * -- Background music: looping, sequenced with the rest of the stage's queue
             * sp_play_song(ID_DESTRUCTION_ALIENS_ATTACK_BGM)
             *
             * -- Background music: looping, but needs to start immediately rather than
             * -- wait for a possibly-stuck sp_* queue (eg. a stage script's own
             * -- perpetual wave-spawn loop) - a boss BGM crossfade, for example
             * play_song_looping(ID_BOSS_TIME_BGM)
             *
             * -- A one-off sound effect: fires immediately, never looped
             * play_song(ID_CARAVELLIUS_SHOOT_AUDIO)
             * ```
             *
             * `play_song_looping` only exists for the Play case —
             * pausing/stopping/resuming a tracked BGM works the same way
             * (and un-tracks it, if applicable) whether it was started
             * via `sp_play_song` or `play_song_looping`, so
             * `pause_song`/`stop_song`/`resume_song` cover both.
             *
             * None of the 9 song functions return a value — check
             * `sound_is_playing(id)` if you need to know playback state.
             */
            int LuaSoundApi :: PlaySong( lua_State *pLuaState )  {

                CallPlayCommandDirect( pLuaState, SunLight :: Scripting :: Commands :: PLAY_SONG_DIRECT_CMD );

                return 0;
            }

            /**
             * @brief Like PlaySong, but also marks this song as the tracked
             * BGM for EngineHost::RunScriptMachine's own per-frame auto-
             * reloop check (raylib's Sound has no native looping - see
             * bgm.lua's own header comment). PLAY_SONG_DIRECT_CMD never
             * sets EngineHost::m_CurrentSong (only the QUEUED PLAY_SONG_CMD
             * does, as a side effect of it's own switch-case falling
             * through into PLAY_SONG_DIRECT_CMD's shared body) - reusing
             * PLAY_SONG_CMD here gets that same tracking/auto-reloop
             * behavior while still dispatching immediately via
             * CallPlayCommandDirect (OnCommand called synchronously, NOT
             * enqueued through ScriptProcessor - a stage script's own
             * perpetual sp_goto_label wave loop can otherwise starve a
             * genuinely queued sp_play_song, same reasoning bgm.lua's own
             * header comment already gives for why STOP_SONG_CMD/etc. use
             * their direct forms). Needed once Cephalon's boss BGM (started
             * via play_song, not sp_play_song, for that same immediacy
             * reason) was found live to go permanently silent after it's
             * own ~173s natural length instead of looping, since plain
             * play_song was never tracked for auto-reloop at all.
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{play_song_looping(id)}
             * @luagroup{song_commands}
             */
            int LuaSoundApi :: PlaySongLooping( lua_State *pLuaState )  {

                CallPlayCommandDirect( pLuaState, SunLight :: Scripting :: Commands :: PLAY_SONG_CMD );

                return 0;
            }

            /**
             * @brief Implement the direct Pause song routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{pause_song(id)}
             * @luagroup{song_commands}
             */
            int LuaSoundApi :: PauseSong( lua_State *pLuaState )  {

                CallPlayCommandDirect( pLuaState, SunLight :: Scripting :: Commands :: PAUSE_SONG_DIRECT_CMD );

                return 0;
            }

            /**
             * @brief Implement the direct Stop song routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{stop_song(id)}
             * @luagroup{song_commands}
             */
            int LuaSoundApi :: StopSong( lua_State *pLuaState )  {

                CallPlayCommandDirect( pLuaState, SunLight :: Scripting :: Commands :: STOP_SONG_DIRECT_CMD );

                return 0;
            }

            /**
             * @brief Implement the direct Resume song routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{resume_song(id)}
             * @luagroup{song_commands}
             */
            int LuaSoundApi :: ResumeSong( lua_State *pLuaState )  {

                CallPlayCommandDirect( pLuaState, SunLight :: Scripting :: Commands :: RESUME_SONG_DIRECT_CMD );

                return 0;
            }

            /**
             * @brief Implement the script processing Play song routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_play_song(id)}
             * @luagroup{song_commands}
             */
            int LuaSoundApi :: QueuePlaySong( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: PLAY_SONG_CMD );

                return 0;
            }

            /**
             * @brief Implement the script processing Pause song routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_pause_song(id)}
             * @luagroup{song_commands}
             */
            int LuaSoundApi :: QueuePauseSong( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: PAUSE_SONG_CMD );

                return 0;
            }

            /**
             * @brief Implement the script processing Stop song routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_stop_song(id)}
             * @luagroup{song_commands}
             */
            int LuaSoundApi :: QueueStopSong( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: STOP_SONG_CMD );

                return 0;
            }

            /**
             * @brief Implement the script processing Resume song routine wrapper;
             *
             * @param pLuaState Lua state to be used by engine call.
             * @return int number os return data (if any - required by lua engine)
             *
             * @luaname{sp_resume_song(id)}
             * @luagroup{song_commands}
             */
            int LuaSoundApi :: QueueResumeSong( lua_State *pLuaState )  {

                LuaEngineUtil :: AddOneParmCommandScript( pLuaState, SunLight :: Scripting :: Commands :: RESUME_SONG_CMD );

                return 0;
            }

            /**
             * @brief Register the sound Lua-callable functions.
             */
            void LuaSoundApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "sound_load", LuaSoundApi :: Load );
                lua_register( pLuaState, "sound_unload", LuaSoundApi :: Unload );
                lua_register( pLuaState, "sound_play", LuaSoundApi :: Play );
                lua_register( pLuaState, "sound_stop", LuaSoundApi :: Stop );
                lua_register( pLuaState, "sound_pause", LuaSoundApi :: Pause );
                lua_register( pLuaState, "sound_resume", LuaSoundApi :: Resume );
                lua_register( pLuaState, "sound_is_playing", LuaSoundApi :: IsPlaying );
                lua_register( pLuaState, "sound_set_volume", LuaSoundApi :: SetVolume );
                lua_register( pLuaState, "play_song", LuaSoundApi :: PlaySong );
                lua_register( pLuaState, "play_song_looping", LuaSoundApi :: PlaySongLooping );
                lua_register( pLuaState, "pause_song", LuaSoundApi :: PauseSong );
                lua_register( pLuaState, "stop_song", LuaSoundApi :: StopSong );
                lua_register( pLuaState, "resume_song", LuaSoundApi :: ResumeSong );
                lua_register( pLuaState, "sp_play_song", LuaSoundApi :: QueuePlaySong );
                lua_register( pLuaState, "sp_pause_song", LuaSoundApi :: QueuePauseSong );
                lua_register( pLuaState, "sp_stop_song", LuaSoundApi :: QueueStopSong );
                lua_register( pLuaState, "sp_resume_song", LuaSoundApi :: QueueResumeSong );
            }
        }
    }
}
