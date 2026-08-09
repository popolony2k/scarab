/*
 * luatilemapapi.h
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#ifndef __LUATILEMAPAPI_H__
#define __LUATILEMAPAPI_H__

extern "C"
{
  #include "lua.h"
}


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {
            /**
             * @brief Tile map management Lua primitives, thin wrapper over the full ITileMap surface.
             */
            class LuaTilemapApi  {

                static int LoadMap( lua_State *pLuaState );
                static int UnloadMap( lua_State *pLuaState );
                static int GetMapInfo( lua_State *pLuaState );
                static int GetLayer( lua_State *pLuaState );
                static int GetLayerByName( lua_State *pLuaState );
                static int SetLayer( lua_State *pLuaState );
                static int SetLayerByName( lua_State *pLuaState );
                static int GetTile( lua_State *pLuaState );
                static int ToTileMatrix( lua_State *pLuaState );
                static int GetObjectByName( lua_State *pLuaState );

                static void RegisterEnums( lua_State *pLuaState );

                public:

                static void Register( lua_State *pLuaState );
            };
        }
    }
}

#endif  /* __LUATILEMAPAPI_H__ */
