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

/*
 * luasoundapi.cpp
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#include "lua/luasoundapi.h"
#include "lua/luaengineutil.h"
#include <cstdio>


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            int LuaSoundApi :: Load( lua_State *pLuaState )  {

                int         nSoundId   = ( int ) lua_tointeger( pLuaState, 1 );
                const char  *szFileName = lua_tostring( pLuaState, 2 );
                bool        bResult    = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Load( nSoundId, szFileName );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaSoundApi :: Unload( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Unload( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaSoundApi :: Play( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Play( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaSoundApi :: Stop( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Stop( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaSoundApi :: Pause( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Pause( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaSoundApi :: Resume( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> Resume( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaSoundApi :: IsPlaying( lua_State *pLuaState )  {

                int   nSoundId = ( int ) lua_tointeger( pLuaState, 1 );
                bool  bResult  = LuaEngineUtil :: GetSoundManager( pLuaState ) -> IsPlaying( nSoundId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

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
