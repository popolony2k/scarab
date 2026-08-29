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

#ifndef __LUATIMERAPI_H__
#define __LUATIMERAPI_H__

#include <string>

extern "C"
{
  #include "lua.h"
  #include "lauxlib.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Background-thread periodic/delayed callback Lua
             * primitives (set_timer/reset_timer), backed by
             * SunLight::Concurrent::Timer. See LuaEngineUtil::s_LuaMutex's
             * own comment for the thread-safety invariant every timer
             * callback depends on.
             */
            class LuaTimerApi  {

                static std :: string GetTimerName( int nId );
                static void DeleteGlobal( lua_State *pLuaState, const char *szVarName );

                static int SetTimer( lua_State *pLuaState );
                static int ResetTimer( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUATIMERAPI_H__ */
