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

            /**
             * @luaname{collision_add_rule(layerA, layerB) -> success}
             * @luadoc
             * Declare that sprites on `layerA` should be checked against
             * sprites on `layerB` every frame. Order doesn't matter for
             * the check itself, but your handler always receives the two
             * handles in a fixed order matching how the *engine*
             * discovered the pair, not necessarily the order you declared
             * the rule in.
             *
             * Declaring `collision_add_rule(x, x)` with both arguments the
             * same layer causes every collider on that layer to "collide
             * with itself" every frame (trivially, at distance 0) —
             * always use two distinct layers.
             * @luaexample
             * collision_add_rule(LAYER_PLAYER_SHIP, LAYER_ENEMIES_SHIPS)
             * collision_add_rule(LAYER_PLAYER_SHIP_BULLETS, LAYER_ENEMIES_SHIPS)
             */
            int LuaCollisionApi :: AddRule( lua_State *pLuaState )  {

                int   nLayerA  = ( int ) lua_tointeger( pLuaState, 1 );
                int   nLayerB  = ( int ) lua_tointeger( pLuaState, 2 );
                bool  bResult  = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetCollisionManager().AddColliderToColliderRule( nLayerA, nLayerB );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{collision_add_tile_rule(layerId, tileLayerId) -> success}
             * @luadoc
             * Declare that sprites on `layerId` should be checked against
             * the static tiles of `tileLayerId` (e.g. terrain collision,
             * as opposed to sprite-vs-sprite).
             */
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
             *
             * @luaname{collision_set_handler(fn)}
             * @luadoc
             * Register the single global function called whenever two
             * sprites on a ruled pair of layers collide:
             * `fn(handleA, handleB)`.
             *
             * Only one handler can be registered at a time — a second
             * `collision_set_handler` call replaces the first, it doesn't
             * add a second listener.
             *
             * **Immunity gotcha**: if your handler doesn't guard against
             * re-triggering on a sprite that's already reacting to a
             * previous hit (already exploding, already blinking
             * invincible, etc.), two sprites overlapping for several
             * consecutive frames re-fires the handler every single frame
             * of that overlap — not just once per "touch." Track and
             * check hit/invincibility state yourself inside the handler;
             * the engine has no concept of "already handled this
             * collision."
             * @luaexample
             * collision_set_handler(function(handleA, handleB)
             *   -- dispatch based on which layer(s) these handles actually belong to -
             *   -- the engine doesn't tell you that here, your own game bookkeeping does
             *   if Enemies.owners[handleA] then
             *     Enemies.owners[handleA].on_hit(handleA)
             *   end
             * end)
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
             *
             * @luaname{collision_set_tile_handler(fn)}
             * @luadoc
             * Register the handler for sprite-vs-tile collisions (from
             * `collision_add_tile_rule`):
             * `fn(handle, gid, x, y, width, height)` — `gid` is the
             * tile's Tiled global id, `x`/`y`/`width`/`height` its
             * world-space position and size.
             * @luaexample
             * collision_set_tile_handler(function(handle, gid, x, y, width, height)
             *   print(("hit tile gid %d at %d,%d"):format(gid, x, y))
             * end)
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
