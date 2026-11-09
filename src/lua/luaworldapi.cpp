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
#include "lua/luaworldapi.h"
#include "lua/luaengineutil.h"
#include "lua/luaspriteapi.h"
#include "engine/irendererprovider.h"
#include "engine/spritepool.h"
#include "tilemap/itilemap.h"
#include "collision/icollisionmanager.h"
#include "base/primitives.h"
#include <climits>
#include <memory>
#include <string>
#include <utility>

/** The file name ending that makes world_load read a Tiled .world file, and the one for a single map. */
#define __WORLD_FILE_SUFFIX  ".world"
#define __MAP_FILE_SUFFIX    ".tmx"
#define __NO_LAYER           -1         // IWorld::GetLayerId with no layer of that name
#define __NO_MAP             -1         // IWorld::AddMap when the map cannot be loaded
#define __DEFAULT_WORLD_ID   0


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            /**
             * @brief The table of the worlds Lua made, from the global LuaEngine set.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return The table;
             */
            static WorldTable* GetWorldTable( lua_State* pLuaState )  {

                return static_cast<WorldTable *>( LuaEngineUtil :: GetGlobalPointer( pLuaState, LuaWorldApi :: WORLD_TABLE_GLOBAL ) );
            }

            /**
             * @brief Whether a string ends with a suffix (a file name's extension).
             *
             * @param strText The string;
             * @param szSuffix The suffix;
             * @return true if it does;
             */
            static bool EndsWith( const std :: string& strText, const char* szSuffix )  {

                std :: string  strSuffix( szSuffix );

                return ( strText.size() >= strSuffix.size() ) &&
                       ( strText.compare( strText.size() - strSuffix.size(), strSuffix.size(), strSuffix ) == 0 );
            }

            /**
             * @brief Keep a world Lua made: add a collision listener carrying its id to its own collision
             * manager, and put both in the world table.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param pWorld The world, just created;
             * @return The world's id;
             */
            static int KeepWorld( lua_State* pLuaState, const std :: shared_ptr<SunLight :: TileMap :: IWorld>& pWorld )  {

                stWorldEntry  entry;
                int           nWorldId = pWorld -> GetId();

                entry.pWorld    = pWorld;
                entry.pListener = std :: make_unique<LuaCollisionListener>( pLuaState, nWorldId );
                pWorld -> GetCollisionManager().AddCollisionListener( entry.pListener.get() );
                ( *GetWorldTable( pLuaState ) )[nWorldId] = std :: move( entry );

                return nWorldId;
            }

            /**
             * @brief Read a world handle argument: the default world (0, once a renderer exists) or a
             * world Lua made and has not unloaded. Never creates the renderer.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The argument's position;
             * @param strError Receives the reason on failure;
             * @return The world, or an empty pointer on failure;
             */
            std :: shared_ptr<SunLight :: TileMap :: IWorld> LuaWorldApi :: ReadWorldArg( lua_State* pLuaState, int nIndex, std :: string& strError )  {

                if( lua_type( pLuaState, nIndex ) != LUA_TNUMBER || !lua_isinteger( pLuaState, nIndex ) )  {
                    strError = "expected a world handle (an integer: 0 is the default world)";
                    return std :: shared_ptr<SunLight :: TileMap :: IWorld>();
                }

                lua_Integer                                       nWorldId = lua_tointeger( pLuaState, nIndex );
                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld;

                if( ( nWorldId >= INT_MIN ) && ( nWorldId <= INT_MAX ) )
                    pWorld = WorldById( pLuaState, ( int ) nWorldId );

                if( !pWorld )
                    strError = "unknown world " + std :: to_string( nWorldId );

                return pWorld;
            }

            /**
             * @brief A world by its id: the default world (0, once a renderer exists) or a world Lua made
             * and has not unloaded. Never creates the renderer.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nWorldId The world's id;
             * @return The world, or an empty pointer when there is none;
             */
            std :: shared_ptr<SunLight :: TileMap :: IWorld> LuaWorldApi :: WorldById( lua_State* pLuaState, int nWorldId )  {

                Engine :: IRendererProvider*  pProvider = LuaEngineUtil :: GetRendererProvider( pLuaState );

                if( pProvider -> GetOrigin() != Engine :: IRendererProvider :: ORIGIN_NONE )  {
                    if( nWorldId == __DEFAULT_WORLD_ID )
                        return pProvider -> GetTileMap( "world" ) -> GetDefaultWorld();

                    WorldTable*  pTable = GetWorldTable( pLuaState );
                    auto         it     = pTable -> find( nWorldId );

                    if( ( it != pTable -> end() ) && !it -> second.pWorld -> IsRemoved() )
                        return it -> second.pWorld;
                }

                return std :: shared_ptr<SunLight :: TileMap :: IWorld>();
            }

            /**
             * @brief Read an optional world argument: when it is left out (or nil) the default world -
             * creating the default renderer if none exists yet, as the single-world functions always
             * did - otherwise @see ReadWorldArg.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param nIndex The argument's position;
             * @param strError Receives the reason on failure;
             * @return The world, or an empty pointer on failure;
             */
            std :: shared_ptr<SunLight :: TileMap :: IWorld> LuaWorldApi :: ReadOptionalWorldArg( lua_State* pLuaState, int nIndex, std :: string& strError )  {

                if( lua_isnoneornil( pLuaState, nIndex ) )
                    return LuaEngineUtil :: GetTileMap( pLuaState ) -> GetDefaultWorld();

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, nIndex, strError );

                if( !pWorld )
                    strError = "argument 'world': " + strError;

                return pWorld;
            }

            /**
             * @brief Make an empty world (sunlight's ITileMap::CreateWorld).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_create() -> world | nil, message}
             * @luagroup{world_lifetime}
             * @luaheading{Making and removing worlds}
             * @luadoc
             * `world_create` makes an **empty** world: fill it with `world_add_map`, show it with
             * `view_set_world`. `world_load( path )` makes one from a single map (`.tmx`) or a Tiled
             * `.world` file - all or nothing: if any map of a `.world` cannot be loaded, no world is made
             * and the message says so. A `.world` file's maps are relative to its folder; only its `maps`
             * list is read, and a file placing maps by `patterns` is refused. Both create the default
             * renderer if none exists yet, as `tilemap_load_map` does.
             *
             * `world_unload( world )` removes a world: its maps are freed, every view showing it goes back
             * to the default world, and its sprites are **detached** - they leave the world (no longer
             * drawn, no longer colliding) but stay yours: their handles keep working, to be added to
             * another world or released. The default world cannot be unloaded. `world_get_count()` counts
             * the live worlds, the default one included (`0` before any renderer exists).
             * @luaexample
             * local dungeon = assert( world_load( "maps/dungeon.tmx" ) )
             * local corner  = assert( view_create( renderer_get_current(), 900, 20, 300, 200 ) )
             * assert( view_set_world( corner, dungeon ) )
             * assert( view_fit_to_map( corner ) )
             * -- later
             * assert( world_unload( dungeon ) )   -- the corner view shows the default world again
             */
            int LuaWorldApi :: WorldCreate( lua_State* pLuaState )  {

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaEngineUtil :: GetTileMap( pLuaState ) -> CreateWorld();

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot create a world" );

                lua_pushinteger( pLuaState, KeepWorld( pLuaState, pWorld ) );

                return 1;
            }

            /**
             * @brief Make a world from a map or a Tiled .world file (CreateWorld, then IWorld::LoadMap or
             * IWorld::LoadWorld); no world is kept when loading fails.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_load(path) -> world | nil, message}
             * @luagroup{world_lifetime}
             */
            int LuaWorldApi :: WorldLoad( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strPath;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                bool  bWorldFile = EndsWith( strPath, __WORLD_FILE_SUFFIX );

                if( !bWorldFile && !EndsWith( strPath, __MAP_FILE_SUFFIX ) )
                    return LuaEngineUtil :: PushError( pLuaState, "argument 'path' must name a .tmx map or a .world file" );

                SunLight :: TileMap :: ITileMap*                  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );
                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld   = pTileMap -> CreateWorld();

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot create a world" );

                if( !( bWorldFile ? pWorld -> LoadWorld( strPath.c_str() ) : pWorld -> LoadMap( strPath.c_str() ) ) )  {
                    pTileMap -> RemoveWorld( pWorld );

                    return LuaEngineUtil :: PushError( pLuaState, "cannot load " + strPath );
                }

                lua_pushinteger( pLuaState, KeepWorld( pLuaState, pWorld ) );

                return 1;
            }

            /**
             * @brief Remove a world Lua made (ITileMap::RemoveWorld): its maps freed, its sprites detached
             * (and forgotten by the sprite pool), every view showing it back on the default world.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_unload(world) -> true | nil, message}
             * @luagroup{world_lifetime}
             */
            int LuaWorldApi :: WorldUnload( lua_State* pLuaState )  {

                std :: string                                     strError;
                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                int  nWorldId = pWorld -> GetId();

                if( nWorldId == __DEFAULT_WORLD_ID )
                    return LuaEngineUtil :: PushError( pLuaState, "the default world cannot be unloaded" );

                LuaEngineUtil :: GetTileMap( pLuaState ) -> RemoveWorld( pWorld );
                static_cast<SpritePool *>( LuaEngineUtil :: GetGlobalPointer( pLuaState, LuaSpriteApi :: SPRITE_POOL_GLOBAL ) ) -> ForgetWorld( nWorldId );

                // The world first, then its listener: sunlight's collision manager cannot drop a listener,
                // so the manager (inside the world) must be gone before the listener is.
                WorldTable*    pTable = GetWorldTable( pLuaState );
                stWorldEntry&  entry  = ( *pTable )[nWorldId];

                pWorld.reset();
                entry.pWorld.reset();
                pTable -> erase( nWorldId );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief The number of live worlds, the default one included (ITileMap::GetWorldCount); 0 before
             * a renderer exists, which this never creates.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_get_count() -> count}
             * @luagroup{world_lifetime}
             */
            int LuaWorldApi :: WorldGetCount( lua_State* pLuaState )  {

                Engine :: IRendererProvider*  pProvider = LuaEngineUtil :: GetRendererProvider( pLuaState );

                lua_pushinteger( pLuaState, ( pProvider -> GetOrigin() == Engine :: IRendererProvider :: ORIGIN_NONE ) ?
                                            0 : pProvider -> GetTileMap( "world_get_count" ) -> GetWorldCount() );

                return 1;
            }

            /**
             * @brief Add a map to a world at an offset (IWorld::AddMap).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_add_map(world, path, x, y) -> mapId | nil, message}
             * @luagroup{world_maps}
             * @luaheading{Several maps in one world}
             * @luadoc
             * `world_add_map` places a map's top-left (the top-left of its pixel box, whatever its
             * orientation, as a Tiled `.world` places it) at `(x, y)` in the world's pixels - negative is
             * fine - and returns the map's id in that world (never reused). A world with no map takes it as
             * its first. `world_remove_map` takes one out again (the world's layer ids stay as they were),
             * `world_get_map_count` counts them, `world_get_map_offset` reads where one is, and
             * `world_get_bounds` gives the rectangle every map fits in (what a view's scroll limits and
             * `view_fit_to_map` use; it may start at a negative position). No camera moves when maps
             * change: place the views yourself. Map ids start at `0` in each world: the map `world_load`
             * loaded, or the first one added, is map `0`; a `.world` file's maps are `0, 1, 2, ...` in the
             * file's order.
             * @luaexample
             * local w = assert( world_create() )
             * local a = assert( world_add_map( w, "maps/west.tmx", 0, 0 ) )
             * local b = assert( world_add_map( w, "maps/east.tmx", 640, 0 ) )   -- right of the first
             * local x, y, width, height = world_get_bounds( w )                   -- 0, 0, 1280, 640
             */
            int LuaWorldApi :: WorldAddMap( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strPath;
                int            nX = 0;
                int            nY = 0;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 2, "path", strPath, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "x", nX, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 4, "y", nY, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                int  nMapId = pWorld -> AddMap( strPath.c_str(), nX, nY );

                if( nMapId == __NO_MAP )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot load map " + strPath );

                lua_pushinteger( pLuaState, nMapId );

                return 1;
            }

            /**
             * @brief Take a map out of a world (IWorld::RemoveMap).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_remove_map(world, mapId) -> true | nil, message}
             * @luagroup{world_maps}
             */
            int LuaWorldApi :: WorldRemoveMap( lua_State* pLuaState )  {

                std :: string  strError;
                int            nMapId = 0;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "mapId", nMapId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pWorld -> RemoveMap( nMapId ) )
                    return LuaEngineUtil :: PushError( pLuaState, "no map " + std :: to_string( nMapId ) + " in world " + std :: to_string( pWorld -> GetId() ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief The number of maps in a world (IWorld::GetMapCount).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_get_map_count(world) -> count | nil, message}
             * @luagroup{world_maps}
             */
            int LuaWorldApi :: WorldGetMapCount( lua_State* pLuaState )  {

                std :: string                                     strError;
                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                lua_pushinteger( pLuaState, pWorld -> GetMapCount() );

                return 1;
            }

            /**
             * @brief Where a map of a world is (IWorld::GetMapOffset).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_get_map_offset(world, mapId) -> x, y | nil, message}
             * @luagroup{world_maps}
             */
            int LuaWorldApi :: WorldGetMapOffset( lua_State* pLuaState )  {

                std :: string  strError;
                int            nMapId = 0;
                int            nX = 0;
                int            nY = 0;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "mapId", nMapId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pWorld -> GetMapOffset( nMapId, nX, nY ) )
                    return LuaEngineUtil :: PushError( pLuaState, "no map " + std :: to_string( nMapId ) + " in world " + std :: to_string( pWorld -> GetId() ) );

                lua_pushinteger( pLuaState, nX );
                lua_pushinteger( pLuaState, nY );

                return 2;
            }

            /**
             * @brief The rectangle every map of a world fits in (IWorld::GetBounds).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_get_bounds(world) -> x, y, width, height | nil, message}
             * @luagroup{world_maps}
             */
            int LuaWorldApi :: WorldGetBounds( lua_State* pLuaState )  {

                std :: string                                     strError;
                SunLight :: Base :: stDimension2D                 bounds;
                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pWorld -> GetBounds( bounds ) )
                    return LuaEngineUtil :: PushError( pLuaState, "world " + std :: to_string( pWorld -> GetId() ) + " has no map" );

                lua_pushinteger( pLuaState, bounds.pos.x );
                lua_pushinteger( pLuaState, bounds.pos.y );
                lua_pushinteger( pLuaState, bounds.size.nWidth );
                lua_pushinteger( pLuaState, bounds.size.nHeight );

                return 4;
            }

            /**
             * @brief The world layer id of a layer name (IWorld::GetLayerId).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_layer_id(world, name) -> layerId | nil, message}
             * @luagroup{world_tiles}
             * @luaheading{Layers and tiles of a world}
             * @luadoc
             * `world_layer_id` gives the id a world uses for a layer name across all its maps - the id
             * `sprite_add_to_layer`, the collision rules and `view_show_layer` take for that world. A world
             * of one map (any `.tmx`, and the default world) uses that map's own Tiled ids, so nothing
             * changes there. `world_get_tile` reads a tile of one of the world's maps, its rectangle in the
             * **world's** pixels (the map's position added); `world_to_tile_matrix` finds the map and cell
             * under a point of the world (every map that holds the point is tried, in the order they were
             * added, each with its own projection - so an isometric map's empty corners fall through to a
             * neighbour), and says so when no map has a cell there.
             * @luaexample
             * local objects = assert( world_layer_id( w, "objects" ) )
             * assert( sprite_add_to_layer( hero, objects, w ) )
             * local mapId, row, col = world_to_tile_matrix( w, 700, 120 )
             * local gid, x, y, width, height = world_get_tile( w, mapId, row, col, objects )
             */
            int LuaWorldApi :: WorldLayerId( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 2, "name", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                int  nLayerId = pWorld -> GetLayerId( strName.c_str() );

                if( nLayerId == __NO_LAYER )
                    return LuaEngineUtil :: PushError( pLuaState, "no layer named '" + strName + "' in world " + std :: to_string( pWorld -> GetId() ) );

                lua_pushinteger( pLuaState, nLayerId );

                return 1;
            }

            /**
             * @brief A tile of one of a world's maps, in the world's pixels (IWorld::GetTile with a map id).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_get_tile(world, mapId, row, col, layerId) -> gid, x, y, width, height | nil, message}
             * @luagroup{world_tiles}
             */
            int LuaWorldApi :: WorldGetTile( lua_State* pLuaState )  {

                std :: string                              strError;
                int                                        nMapId = 0;
                int                                        nLayerId = 0;
                SunLight :: TileMap :: stMatrixPosition    pos;
                SunLight :: TileMap :: stTile              tile;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "mapId", nMapId, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "row", pos.nTileRow, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 4, "col", pos.nTileCol, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 5, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pWorld -> GetTile( nMapId, pos, nLayerId, tile ) )
                    return LuaEngineUtil :: PushError( pLuaState, "no tile at row " + std :: to_string( pos.nTileRow ) + ", col " +
                                                       std :: to_string( pos.nTileCol ) + " of layer " + std :: to_string( nLayerId ) +
                                                       " in map " + std :: to_string( nMapId ) + " of world " + std :: to_string( pWorld -> GetId() ) );

                lua_pushinteger( pLuaState, ( lua_Integer ) tile.nGID );
                lua_pushinteger( pLuaState, tile.dimension.pos.x );
                lua_pushinteger( pLuaState, tile.dimension.pos.y );
                lua_pushinteger( pLuaState, tile.dimension.size.nWidth );
                lua_pushinteger( pLuaState, tile.dimension.size.nHeight );

                return 5;
            }

            /**
             * @brief The map and cell under a point of a world (IWorld::TileMapToTileMatrix with a map id).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{world_to_tile_matrix(world, x, y) -> mapId, row, col | nil, message}
             * @luagroup{world_tiles}
             */
            int LuaWorldApi :: WorldToTileMatrix( lua_State* pLuaState )  {

                std :: string                              strError;
                SunLight :: Base :: stCoordinate2D         coord;
                SunLight :: TileMap :: stMatrixPosition    pos;
                int                                        nMapId = 0;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "x", coord.x, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "y", coord.y, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = ReadWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pWorld -> TileMapToTileMatrix( coord, nMapId, pos ) )
                    return LuaEngineUtil :: PushError( pLuaState, "(" + std :: to_string( coord.x ) + ", " + std :: to_string( coord.y ) +
                                                       ") is not in a cell of world " + std :: to_string( pWorld -> GetId() ) );

                lua_pushinteger( pLuaState, nMapId );
                lua_pushinteger( pLuaState, pos.nTileRow );
                lua_pushinteger( pLuaState, pos.nTileCol );

                return 3;
            }

            /**
             * @brief Register the world functions.
             *
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaWorldApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "world_create", LuaWorldApi :: WorldCreate );
                lua_register( pLuaState, "world_load", LuaWorldApi :: WorldLoad );
                lua_register( pLuaState, "world_unload", LuaWorldApi :: WorldUnload );
                lua_register( pLuaState, "world_get_count", LuaWorldApi :: WorldGetCount );
                lua_register( pLuaState, "world_add_map", LuaWorldApi :: WorldAddMap );
                lua_register( pLuaState, "world_remove_map", LuaWorldApi :: WorldRemoveMap );
                lua_register( pLuaState, "world_get_map_count", LuaWorldApi :: WorldGetMapCount );
                lua_register( pLuaState, "world_get_map_offset", LuaWorldApi :: WorldGetMapOffset );
                lua_register( pLuaState, "world_get_bounds", LuaWorldApi :: WorldGetBounds );
                lua_register( pLuaState, "world_layer_id", LuaWorldApi :: WorldLayerId );
                lua_register( pLuaState, "world_get_tile", LuaWorldApi :: WorldGetTile );
                lua_register( pLuaState, "world_to_tile_matrix", LuaWorldApi :: WorldToTileMatrix );
            }
        }
    }
}
