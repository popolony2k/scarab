/*
 * luaspriteapi.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
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
