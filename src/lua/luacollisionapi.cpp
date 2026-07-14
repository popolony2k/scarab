/*
 * luacollisionapi.cpp
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#include "lua/luacollisionapi.h"
#include "lua/luaengineutil.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            int LuaCollisionApi :: AddRule( lua_State *pLuaState )  {

                int   nLayerA  = ( int ) lua_tointeger( pLuaState, 1 );
                int   nLayerB  = ( int ) lua_tointeger( pLuaState, 2 );
                bool  bResult  = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetCollisionManager().AddColliderToColliderRule( nLayerA, nLayerB );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaCollisionApi :: AddTileRule( lua_State *pLuaState )  {

                int   nLayerId     = ( int ) lua_tointeger( pLuaState, 1 );
                int   nTileLayerId = ( int ) lua_tointeger( pLuaState, 2 );
                bool  bResult      = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetCollisionManager().AddColliderToTileRule( nLayerId, nTileLayerId );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @brief Register the Lua function to call whenever two colliders
             * (at least one of them Lua-owned) hit one another - fn(handleA, handleB).
             * A side that isn't Lua-owned (e.g. a still-legacy Caravellius
             * sprite) is passed as INVALID_SPRITE_HANDLE (0).
             */
            int LuaCollisionApi :: SetHandler( lua_State *pLuaState )  {

                if( !lua_isfunction( pLuaState, 1 ) )  {
                    fprintf( stderr, "collision_set_handler: argument must be a function.\n" );

                    return 0;
                }

                lua_pushvalue( pLuaState, 1 );
                lua_setglobal( pLuaState, "__collision_handler" );

                return 0;
            }

            /**
             * @brief Register the Lua function to call whenever a Lua-owned
             * collider hits a tile - fn(handle, gid, x, y, width, height).
             */
            int LuaCollisionApi :: SetTileHandler( lua_State *pLuaState )  {

                if( !lua_isfunction( pLuaState, 1 ) )  {
                    fprintf( stderr, "collision_set_tile_handler: argument must be a function.\n" );

                    return 0;
                }

                lua_pushvalue( pLuaState, 1 );
                lua_setglobal( pLuaState, "__collision_tile_handler" );

                return 0;
            }

            /**
             * @brief Register the collision Lua-callable functions.
             */
            void LuaCollisionApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "collision_add_rule", LuaCollisionApi :: AddRule );
                lua_register( pLuaState, "collision_add_tile_rule", LuaCollisionApi :: AddTileRule );
                lua_register( pLuaState, "collision_set_handler", LuaCollisionApi :: SetHandler );
                lua_register( pLuaState, "collision_set_tile_handler", LuaCollisionApi :: SetTileHandler );
            }
        }
    }
}
