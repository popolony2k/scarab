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
