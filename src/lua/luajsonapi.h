/*
 * luajsonapi.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#ifndef __LUAJSONAPI_H__
#define __LUAJSONAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Generic, game-agnostic JSON <-> Lua bridge. Converts any JSON
             * document into an equivalent Lua table, with no knowledge of any
             * particular config schema.
             */
            class LuaJsonApi  {

                static int LoadJson( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAJSONAPI_H__ */
