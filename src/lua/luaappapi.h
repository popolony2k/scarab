/*
 * luaappapi.h
 *
 *  Created on: Jul 11, 2026
 *      Author: popolony2k
 */

#ifndef __LUAAPPAPI_H__
#define __LUAAPPAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Application-level Lua primitives - concerns that belong
             * to the running app/game itself, not to any one engine
             * subsystem (camera, sound, sprites, ...).
             */
            class LuaAppApi  {

                static int SetAppName( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAAPPAPI_H__ */
