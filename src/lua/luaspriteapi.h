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

#ifndef __LUASPRITEAPI_H__
#define __LUASPRITEAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Sprite handle pool + rendering Lua primitives.
             */
            class LuaSpriteApi  {

                static int RegisterType( lua_State *pLuaState );
                static int Acquire( lua_State *pLuaState );
                static int Release( lua_State *pLuaState );
                static int ConfigureTexture( lua_State *pLuaState );
                static int SetActiveSequence( lua_State *pLuaState );
                static int GetActiveSequence( lua_State *pLuaState );
                static int SetAnimationMode( lua_State *pLuaState );
                static int SetVisible( lua_State *pLuaState );
                static int GetVisible( lua_State *pLuaState );
                static int GetPos( lua_State *pLuaState );
                static int SetPos( lua_State *pLuaState );
                static int GetSize( lua_State *pLuaState );
                static int SetCollisionInset( lua_State *pLuaState );
                static int AddToLayer( lua_State *pLuaState );
                static int RemoveFromLayer( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUASPRITEAPI_H__ */
