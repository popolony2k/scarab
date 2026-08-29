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
