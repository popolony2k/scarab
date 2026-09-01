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
             *
             * @luacategory{Input}
             * @luadoc
             * All input functions poll current state once per call —
             * there's no event/callback model for input (unlike
             * collisions, which do have a callback — see [collision.md](collision.md)).
             * Call these from your own per-frame update function.
             *
             * Every constant below (see the bottom of this page) is
             * registered as a plain Lua global (an integer) — no
             * `require`, no table lookup, just reference the name
             * directly. They match raylib's own
             * `KeyboardKey`/`GamepadButton`/`GamepadAxis`/`ControllerType`
             * enums 1:1.
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
