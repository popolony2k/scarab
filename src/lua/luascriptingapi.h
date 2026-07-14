/*
 * luascriptingapi.h
 *
 *  Created on: Jul 11, 2026
 *      Author: popolony2k
 */

#ifndef __LUASCRIPTINGAPI_H__
#define __LUASCRIPTINGAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief ScriptProcessor queue-control Lua primitives - sequencing
             * only (waits, labels, wave-spawn/stage-load dispatch); song
             * playback commands live in LuaSoundApi instead, even the queued
             * ones, since they're topically about sound, not queue mechanics.
             */
            class LuaScriptingApi  {

                static int Wait( lua_State *pLuaState );
                static int Clear( lua_State *pLuaState );
                static int WaitQueueEmpty( lua_State *pLuaState );
                static int MoveSpritesToScreen( lua_State *pLuaState );
                static int AddLabel( lua_State *pLuaState );
                static int GotoLabel( lua_State *pLuaState );
                static int LoadStage( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUASCRIPTINGAPI_H__ */
