/*
 Copyright (c) since 2021 by PopolonY2k and Leidson Campos A. Ferreira

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.
*/
#ifndef __LUAWORLDAPI_H__
#define __LUAWORLDAPI_H__

#include <map>
#include <memory>
#include <string>

extern "C"  {
  #include "lua.h"
}

#include "tilemap/iworld.h"
#include "lua/luacollisionlistener.h"


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief A world made by world_create or world_load, with the collision listener added to its
             * own collision manager - so every hit in it reaches the Lua handlers with the world's id.
             */
            struct stWorldEntry  {
                std :: shared_ptr<SunLight :: TileMap :: IWorld>   pWorld;
                std :: unique_ptr<LuaCollisionListener>            pListener;
            };

            /**
             * @brief Shared type for LuaEngine's m_Worlds member and LuaWorldApi's casts of the
             * WORLD_TABLE_GLOBAL global: the worlds Lua made, by sunlight's own world id (never reused).
             * The default world (id 0) is not in it: it is the renderer's own.
             */
            typedef std :: map<int, stWorldEntry> WorldTable;

            /**
             * @brief Worlds - several maps at once (plans/MASTER_PLAN.md item C7, sunlight I19).
             *
             * @luacategory{Worlds}
             * @luadoc
             * A **world** holds one or more maps, the sprites on its layers and its collision rules. A
             * view shows one world (`view_set_world`). Every renderer has a **default world**, id `0`:
             * the one `tilemap_load_map` loads into, the one the default view always shows, and the one
             * every function acts on when it is given no world - so a game that never makes a world is
             * exactly as before. More worlds come from `world_create` and `world_load`, and are shown by
             * extra views (`view_create`), for split screen, a world map beside the local one, a dungeon
             * in a corner of the screen, or a cross-fade between two maps.
             *
             * **Worlds are isolated.** Each has its own coordinates (its maps' pixels), its own sprites
             * and its own collision rules: a sprite in one is never drawn in, nor collides with anything
             * in, another. A sprite is in one world at a time (`sprite_add_to_layer( handle, layer,
             * world )`); to move it, take it off its layer in one world and add it in the other (the game
             * converts its position). Collision handlers receive the world a hit happened in as their
             * last argument. Every live world updates every frame, shown by a view or not.
             *
             * **Several maps in one world**, as a Tiled `.world` file places them: `world_add_map( world,
             * path, x, y )` puts a map's top-left at `(x, y)` in the world's pixels (negative is fine,
             * any orientation, mixed). Layers are matched by **name**: each name has one world layer id
             * (`world_layer_id`), the first map's ids kept and new names numbered above them; sprites,
             * collision rules and view layer masks use these ids, and a sprite on a layer is drawn over
             * that layer of every map, so it crosses map edges cleanly. A collider tests every map it
             * overlaps. A view's scroll limits and `view_fit_to_map` cover every map (`world_get_bounds`).
             *
             * World handles are opaque integers, never reused. Every function reports a failure as `nil,
             * "message"`: an unknown or unloaded world is `"unknown world N"`. A world exists only once a
             * renderer does: `world_create` and `world_load` create the default one if needed, like
             * `tilemap_load_map`; the other functions never do.
             *
             * **What stays on the default world:** `tilemap_load_map`/`tilemap_unload_map`, the layer
             * setters (`tilemap_set_layer`, tint, parallax) and `tilemap_get_tile`/
             * `tilemap_to_tile_matrix` (use `world_get_tile`/`world_to_tile_matrix` for another world).
             * sunlight's world has no layer setter yet; it is a known limit, asked for when a game needs it.
             */
            class LuaWorldApi  {

                static int WorldCreate( lua_State* pLuaState );
                static int WorldLoad( lua_State* pLuaState );
                static int WorldUnload( lua_State* pLuaState );
                static int WorldGetCount( lua_State* pLuaState );
                static int WorldAddMap( lua_State* pLuaState );
                static int WorldRemoveMap( lua_State* pLuaState );
                static int WorldGetMapCount( lua_State* pLuaState );
                static int WorldGetMapOffset( lua_State* pLuaState );
                static int WorldGetBounds( lua_State* pLuaState );
                static int WorldLayerId( lua_State* pLuaState );
                static int WorldGetTile( lua_State* pLuaState );
                static int WorldToTileMatrix( lua_State* pLuaState );

                public:

                /** @brief The Lua global LuaEngine stores its WorldTable's address in. */
                static constexpr const char*  WORLD_TABLE_GLOBAL = "worldTablePtr";

                static std :: shared_ptr<SunLight :: TileMap :: IWorld> ReadWorldArg( lua_State* pLuaState, int nIndex, std :: string& strError );
                static std :: shared_ptr<SunLight :: TileMap :: IWorld> WorldById( lua_State* pLuaState, int nWorldId );
                static std :: shared_ptr<SunLight :: TileMap :: IWorld> ReadOptionalWorldArg( lua_State* pLuaState, int nIndex, std :: string& strError );
                static void Register( lua_State* pLuaState );
            };
        }
    }
}

#endif /* __LUAWORLDAPI_H__ */
