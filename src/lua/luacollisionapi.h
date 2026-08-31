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
             *
             * @luacategory{Collision}
             * @luadoc
             * Also implemented in `src/lua/luacollisionlistener.cpp` (the
             * sole collision listener, decoding the packed handles a
             * collision callback receives - no Lua-callable primitives of
             * its own). Collision detection in this engine is entirely
             * **layer-based**: you declare which pairs of Tiled layers
             * should be checked against each other (`collision_add_rule`),
             * and register one global handler function that's called
             * whenever any two colliding sprites belong to a ruled-together
             * pair. There's no per-sprite "on hit" registration — dispatch
             * to the right game logic based on the two handles is entirely
             * up to your handler.
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
