/*
 * luainputapi.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#ifndef __LUAINPUTAPI_H__
#define __LUAINPUTAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Input polling Lua primitives, thin wrapper over IInputHandler.
             */
            class LuaInputApi  {

                static int IsKeyDown( lua_State *pLuaState );
                static int IsKeyUp( lua_State *pLuaState );
                static int IsKeyReleased( lua_State *pLuaState );
                static int IsGamepadButtonDown( lua_State *pLuaState );
                static int IsGamepadButtonUp( lua_State *pLuaState );
                static int GetGamepadAxisMovement( lua_State *pLuaState );
                static int AddGamepad( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUAINPUTAPI_H__ */
