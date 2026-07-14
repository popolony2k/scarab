/*
 * luatimerapi.h
 *
 *  Created on: Jul 11, 2026
 *      Author: popolony2k
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
