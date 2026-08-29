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
             * @brief Register the Lua function to call whenever two Lua-owned
             * colliders hit one another - fn(handleA, handleB). Every sprite
             * in the game is Lua/SpritePool-owned today, so this always fires
             * with two real handles; see LuaCollisionListener::OnCollision's
             * own comment for the defensive (non-Lua-owned-side) check this
             * still guards against.
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
