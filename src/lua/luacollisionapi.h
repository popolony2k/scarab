/*
 * luacollisionapi.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#ifndef __LUACOLLISIONAPI_H__
#define __LUACOLLISIONAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Collision rule/handler registration Lua primitives.
             */
            class LuaCollisionApi  {

                static int AddRule( lua_State *pLuaState );
                static int AddTileRule( lua_State *pLuaState );
                static int SetHandler( lua_State *pLuaState );
                static int SetTileHandler( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUACOLLISIONAPI_H__ */
