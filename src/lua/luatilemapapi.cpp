/*
 * luatilemapapi.cpp
 *
 *  Created on: Jul 6, 2026
 *      Author: popolony2k
 */

#include "lua/luatilemapapi.h"
#include "lua/luaengineutil.h"

extern "C"
{
  #include "lauxlib.h"
}

using namespace SunLight :: TileMap;


namespace Scarab  {
    namespace Engine  {
        namespace Lua  {

            int LuaTilemapApi :: LoadMap( lua_State *pLuaState )  {

                const char  *szPath      = lua_tostring( pLuaState, 1 );
                int         nAlignment   = ( int ) luaL_optinteger( pLuaState, 2, ITileMap :: MAP_ALIGNMENT_CENTER );
                bool        bResult      = LuaEngineUtil :: GetTileMap( pLuaState ) -> LoadMap( szPath, ( ITileMap :: MapAlignment ) nAlignment );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaTilemapApi :: UnloadMap( lua_State *pLuaState )  {

                bool  bResult = LuaEngineUtil :: GetTileMap( pLuaState ) -> UnloadMap();

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            int LuaTilemapApi :: GetMapInfo( lua_State *pLuaState )  {

                stMapInfo  mapInfo;
                bool       bResult = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetMapInfo( mapInfo );

                if( !bResult )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushinteger( pLuaState, mapInfo.mapSize.nWidth );
                lua_pushinteger( pLuaState, mapInfo.mapSize.nHeight );
                lua_pushinteger( pLuaState, mapInfo.tileSize.nWidth );
                lua_pushinteger( pLuaState, mapInfo.tileSize.nHeight );

                return 4;
            }

            int LuaTilemapApi :: GetLayer( lua_State *pLuaState )  {

                int      nLayerId = ( int ) lua_tointeger( pLuaState, 1 );
                stLayer  layer;
                bool     bResult  = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetLayer( nLayerId, layer );

                if( !bResult )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushboolean( pLuaState, layer.bVisible );
                lua_pushinteger( pLuaState, layer.nOpacity );
                lua_pushinteger( pLuaState, layer.offset.x );
                lua_pushinteger( pLuaState, layer.offset.y );

                return 4;
            }

            int LuaTilemapApi :: GetLayerByName( lua_State *pLuaState )  {

                const char  *szLayerName = lua_tostring( pLuaState, 1 );
                stLayer     layer;
                bool        bResult      = LuaEngineUtil :: GetTileMap( pLuaState ) -> GetLayer( szLayerName, layer );

                if( !bResult )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushboolean( pLuaState, layer.bVisible );
                lua_pushinteger( pLuaState, layer.nOpacity );
                lua_pushinteger( pLuaState, layer.offset.x );
                lua_pushinteger( pLuaState, layer.offset.y );

                return 4;
            }

            int LuaTilemapApi :: SetLayer( lua_State *pLuaState )  {

                ITileMap  *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );
                int       nLayerId  = ( int ) lua_tointeger( pLuaState, 1 );
                stLayer   layer;

                /*
                 * Fetch the existing layer first so it's internal tmx_layer pointer
                 * (opaque to Lua) is preserved when writing it back.
                 */
                if( !pTileMap -> GetLayer( nLayerId, layer ) )  {
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                layer.bVisible = lua_toboolean( pLuaState, 2 );
                layer.nOpacity = ( int ) lua_tointeger( pLuaState, 3 );
                layer.offset.x = ( int ) lua_tointeger( pLuaState, 4 );
                layer.offset.y = ( int ) lua_tointeger( pLuaState, 5 );

                lua_pushboolean( pLuaState, pTileMap -> SetLayer( nLayerId, layer ) );

                return 1;
            }

            int LuaTilemapApi :: SetLayerByName( lua_State *pLuaState )  {

                ITileMap    *pTileMap    = LuaEngineUtil :: GetTileMap( pLuaState );
                const char  *szLayerName = lua_tostring( pLuaState, 1 );
                stLayer     layer;

                if( !pTileMap -> GetLayer( szLayerName, layer ) )  {
                    lua_pushboolean( pLuaState, false );

                    return 1;
                }

                layer.bVisible = lua_toboolean( pLuaState, 2 );
                layer.nOpacity = ( int ) lua_tointeger( pLuaState, 3 );
                layer.offset.x = ( int ) lua_tointeger( pLuaState, 4 );
                layer.offset.y = ( int ) lua_tointeger( pLuaState, 5 );

                lua_pushboolean( pLuaState, pTileMap -> SetLayer( szLayerName, layer ) );

                return 1;
            }

            int LuaTilemapApi :: GetTile( lua_State *pLuaState )  {

                ITileMap          *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );
                stMatrixPosition  pos;

                pos.nTileRow = ( int ) lua_tointeger( pLuaState, 1 );
                pos.nTileCol = ( int ) lua_tointeger( pLuaState, 2 );

                int      nLayerId = ( int ) lua_tointeger( pLuaState, 3 );
                stLayer  layer;

                if( !pTileMap -> GetLayer( nLayerId, layer ) )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                stTile  tile;

                if( !pTileMap -> GetTile( pos, layer, tile ) )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushinteger( pLuaState, ( lua_Integer ) tile.nGID );
                lua_pushinteger( pLuaState, tile.dimension.pos.x );
                lua_pushinteger( pLuaState, tile.dimension.pos.y );
                lua_pushinteger( pLuaState, tile.dimension.size.nWidth );
                lua_pushinteger( pLuaState, tile.dimension.size.nHeight );

                return 5;
            }

            int LuaTilemapApi :: ToTileMatrix( lua_State *pLuaState )  {

                stCoordinate2D  coord;

                coord.x = ( int ) lua_tointeger( pLuaState, 1 );
                coord.y = ( int ) lua_tointeger( pLuaState, 2 );

                stMatrixPosition  pos;
                bool              bResult = LuaEngineUtil :: GetTileMap( pLuaState ) -> TileMapToTileMatrix( coord, pos );

                if( !bResult )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushinteger( pLuaState, pos.nTileRow );
                lua_pushinteger( pLuaState, pos.nTileCol );

                return 2;
            }

            void LuaTilemapApi :: RegisterEnums( lua_State *pLuaState )  {

                static const stNamedConstant  s_aMapAlignments[] = {
                    { "MAP_ALIGNMENT_CENTER", ITileMap :: MAP_ALIGNMENT_CENTER },
                    { "MAP_ALIGNMENT_TOP_RIGHT", ITileMap :: MAP_ALIGNMENT_TOP_RIGHT },
                    { "MAP_ALIGNMENT_TOP_LEFT", ITileMap :: MAP_ALIGNMENT_TOP_LEFT },
                    { "MAP_ALIGNMENT_BOTTOM_RIGHT", ITileMap :: MAP_ALIGNMENT_BOTTOM_RIGHT },
                    { "MAP_ALIGNMENT_BOTTOM_LEFT", ITileMap :: MAP_ALIGNMENT_BOTTOM_LEFT },
                    { "MAP_ALIGNMENT_CENTER_WIDTH_TOP", ITileMap :: MAP_ALIGNMENT_CENTER_WIDTH_TOP },
                    { "MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM", ITileMap :: MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM },
                    { "MAP_ALIGNMENT_CENTER_HEIGHT_LEFT", ITileMap :: MAP_ALIGNMENT_CENTER_HEIGHT_LEFT },
                    { "MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT", ITileMap :: MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT },
                };

                LuaEngineUtil :: RegisterConstants( pLuaState, s_aMapAlignments, sizeof( s_aMapAlignments ) / sizeof( s_aMapAlignments[0] ) );
            }

            /**
             * @brief Register the tile map management Lua-callable functions and enums.
             */
            void LuaTilemapApi :: Register( lua_State *pLuaState )  {

                lua_register( pLuaState, "tilemap_load_map", LuaTilemapApi :: LoadMap );
                lua_register( pLuaState, "tilemap_unload_map", LuaTilemapApi :: UnloadMap );
                lua_register( pLuaState, "tilemap_get_map_info", LuaTilemapApi :: GetMapInfo );
                lua_register( pLuaState, "tilemap_get_layer", LuaTilemapApi :: GetLayer );
                lua_register( pLuaState, "tilemap_get_layer_by_name", LuaTilemapApi :: GetLayerByName );
                lua_register( pLuaState, "tilemap_set_layer", LuaTilemapApi :: SetLayer );
                lua_register( pLuaState, "tilemap_set_layer_by_name", LuaTilemapApi :: SetLayerByName );
                lua_register( pLuaState, "tilemap_get_tile", LuaTilemapApi :: GetTile );
                lua_register( pLuaState, "tilemap_to_tile_matrix", LuaTilemapApi :: ToTileMatrix );

                RegisterEnums( pLuaState );
            }
        }
    }
}
