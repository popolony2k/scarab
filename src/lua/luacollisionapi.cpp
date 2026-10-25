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
#include "collision/collisionmanager.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief Read a collision layer id: sunlight keeps one collider list per id in a fixed
             * array of MAX_COLLIDER_LAYERS. CollisionManager refuses an id outside it since sunlight's
             * I5 (before, it read past the array); checked here too so a script gets the reason
             * ("argument 'layerB' must be between 0 and 254") rather than a plain refusal that reads
             * like "the rule is already declared".
             */
            static bool ReadCollisionLayerArg( lua_State *pLuaState, int nIndex, const char *szName, int &nLayerId, std :: string &strError )  {

                lua_Integer  nValue = 0;

                if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, nIndex, szName, 0, MAX_COLLIDER_LAYERS - 1, nValue, strError ) )
                    return false;

                nLayerId = ( int ) nValue;

                return true;
            }

            /**
             * @luaname{collision_add_rule(layerA, layerB) -> true | nil, message}
             * @luadoc
             * Every collision function reports a failure as `nil, "message"`
             * and never raises a Lua error. Layer ids are integers from `0` to
             * `254`.
             *
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
             * always use two distinct layers. Declaring the same rule twice is
             * an error.
             * @luaexample
             * collision_add_rule(LAYER_PLAYER_SHIP, LAYER_ENEMIES_SHIPS)
             * collision_add_rule(LAYER_PLAYER_SHIP_BULLETS, LAYER_ENEMIES_SHIPS)
             */
            int LuaCollisionApi :: AddRule( lua_State *pLuaState )  {

                std :: string  strError;
                int            nLayerA = 0;
                int            nLayerB = 0;

                if( !ReadCollisionLayerArg( pLuaState, 1, "layerA", nLayerA, strError ) ||
                    !ReadCollisionLayerArg( pLuaState, 2, "layerB", nLayerB, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaEngineUtil :: GetTileMap( pLuaState ) -> GetCollisionManager().AddColliderToColliderRule( nLayerA, nLayerB ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the rule for layers " + std :: to_string( nLayerA ) + " and " +
                                                       std :: to_string( nLayerB ) + " is already declared" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{collision_add_tile_rule(layerId, tileLayerId) -> true | nil, message}
             * @luadoc
             * Declare that sprites on `layerId` should be checked against
             * the static tiles of `tileLayerId` (e.g. terrain collision,
             * as opposed to sprite-vs-sprite). `tileLayerId` must be a layer
             * of the loaded map, so declare it after `tilemap_load_map`.
             */
            int LuaCollisionApi :: AddTileRule( lua_State *pLuaState )  {

                std :: string  strError;
                int            nLayerId     = 0;
                int            nTileLayerId = 0;

                if( !ReadCollisionLayerArg( pLuaState, 1, "layerId", nLayerId, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "tileLayerId", nTileLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaEngineUtil :: GetTileMap( pLuaState ) -> GetCollisionManager().AddColliderToTileRule( nLayerId, nTileLayerId ) )
                    return LuaEngineUtil :: PushError( pLuaState, "layer " + std :: to_string( nTileLayerId ) + " is not a layer of the loaded map" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Register the Lua function to call whenever two Lua-owned
             * colliders hit one another - fn(handleA, handleB). Every sprite
             * in the game is Lua/SpritePool-owned today, so this always fires
             * with two real handles; see LuaCollisionListener::OnCollision's
             * own comment for the defensive (non-Lua-owned-side) check this
             * still guards against.
             *
             * @luaname{collision_set_handler(fn) -> true | nil, message}
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

                std :: string  strError;

                if( !LuaEngineUtil :: CheckFunctionArg( pLuaState, 1, "fn", strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushvalue( pLuaState, 1 );
                lua_setglobal( pLuaState, "__collision_handler" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Register the Lua function to call whenever a Lua-owned
             * collider hits a tile - fn(handle, gid, x, y, width, height).
             *
             * @luaname{collision_set_tile_handler(fn) -> true | nil, message}
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

                std :: string  strError;

                if( !LuaEngineUtil :: CheckFunctionArg( pLuaState, 1, "fn", strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushvalue( pLuaState, 1 );
                lua_setglobal( pLuaState, "__collision_tile_handler" );

                return LuaEngineUtil :: PushTrue( pLuaState );
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
