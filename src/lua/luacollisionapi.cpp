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
            static bool ReadCollisionLayerArg( lua_State* pLuaState, int nIndex, const char* szName, int& nLayerId, std :: string& strError )  {

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
            int LuaCollisionApi :: AddRule( lua_State* pLuaState )  {

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
             *
             * **Only a tile with a collision shape collides**: a rectangle drawn
             * on it in Tiled's Tile Collision Editor (in its tileset). Every
             * frame, each sprite is checked against every such tile it
             * overlaps, in **map space**: a view-relative sprite's position is
             * taken with the camera, a world-space one's as it is, so the camera
             * and the zoom never change where a sprite collides. Rectangles that
             * only share an edge don't touch, so a sprite can rest flush against
             * a wall. Up to Scarab v0.1.25 (sunlight up to I6) only the tile
             * under the sprite's top-left corner was checked, shifted by the
             * viewport's origin, and world-space sprites got the camera added -
             * and Scarab's handler never ran at all.
             */
            int LuaCollisionApi :: AddTileRule( lua_State* pLuaState )  {

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
            int LuaCollisionApi :: SetHandler( lua_State* pLuaState )  {

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
             * tile's Tiled global id, `x`/`y`/`width`/`height` the tile's
             * rectangle in **map pixels** (the same numbers
             * `tilemap_get_tile` returns).
             *
             * It fires **once per tile touched**, every frame the sprite
             * overlaps it: a sprite in a corner of two walls gets two calls in
             * one frame. A handler that blocks movement must therefore undo the
             * frame's movement ONCE, and all of it: a diagonal (two keys held)
             * is two steps in one frame, and undoing only the last one lets the
             * sprite work its way through a wall. The handler runs after
             * `on_update`, in the same frame, so a position set here is the one
             * drawn. `samples/collision` shows the pattern.
             * @luaexample
             * local before = {}     -- the player's position before this frame's movement, saved in on_update
             * local undoneFrame = -1
             *
             * collision_set_tile_handler(function(handle, gid, x, y, width, height)
             *   if handle == player and undoneFrame ~= frame then
             *     sprite_set_pos(player, before.x, before.y)   -- the whole frame's movement, once
             *     undoneFrame = frame
             *   end
             * end)
             */
            int LuaCollisionApi :: SetTileHandler( lua_State* pLuaState )  {

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
            void LuaCollisionApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "collision_add_rule", LuaCollisionApi :: AddRule );
                lua_register( pLuaState, "collision_add_tile_rule", LuaCollisionApi :: AddTileRule );
                lua_register( pLuaState, "collision_set_handler", LuaCollisionApi :: SetHandler );
                lua_register( pLuaState, "collision_set_tile_handler", LuaCollisionApi :: SetTileHandler );
            }
        }
    }
}
