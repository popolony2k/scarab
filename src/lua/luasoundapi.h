/*
 * luasoundapi.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#ifndef __LUASOUNDAPI_H__
#define __LUASOUNDAPI_H__

extern "C"
{
  #include "lua.h"
}

#include "scripting/iscriptlistener.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Sound Lua primitives. sound_* wrap SoundManager's
             * immediate playback state directly; the *_song/sp_*_song forms
             * (direct and queued respectively) instead go through the
             * ScriptProcessor/EngineHost, since "which song is the tracked
             * BGM" is state EngineHost::OnCommand owns, not SoundManager.
             */
            class LuaSoundApi  {

                static int Load( lua_State *pLuaState );
                static int Unload( lua_State *pLuaState );
                static int Play( lua_State *pLuaState );
                static int Stop( lua_State *pLuaState );
                static int Pause( lua_State *pLuaState );
                static int Resume( lua_State *pLuaState );
                static int IsPlaying( lua_State *pLuaState );
                static int SetVolume( lua_State *pLuaState );

                // Direct (non-queued) song commands - reach EngineHost::OnCommand synchronously
                static void CallPlayCommandDirect( lua_State *pLuaState, SunLight :: Scripting :: Commands cmd );
                static int PlaySong( lua_State *pLuaState );
                static int PlaySongLooping( lua_State *pLuaState );
                static int PauseSong( lua_State *pLuaState );
                static int StopSong( lua_State *pLuaState );
                static int ResumeSong( lua_State *pLuaState );

                // Queued song commands - paced by the ScriptProcessor like sp_wait/sp_move_sprites_to_screen
                static int QueuePlaySong( lua_State *pLuaState );
                static int QueuePauseSong( lua_State *pLuaState );
                static int QueueStopSong( lua_State *pLuaState );
                static int QueueResumeSong( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUASOUNDAPI_H__ */
