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

            /**
             * @luaname{tilemap_load_map(path, alignment) -> success}
             * @luadoc
             * Load a `.tmx` map file, replacing any currently loaded map.
             * `alignment` (optional, defaults to `MAP_ALIGNMENT_CENTER`)
             * controls how the map is positioned relative to the
             * viewport when it's smaller than the screen.
             *
             * Alignment constants: `MAP_ALIGNMENT_CENTER`,
             * `MAP_ALIGNMENT_TOP_LEFT`, `MAP_ALIGNMENT_TOP_RIGHT`,
             * `MAP_ALIGNMENT_BOTTOM_LEFT`, `MAP_ALIGNMENT_BOTTOM_RIGHT`,
             * `MAP_ALIGNMENT_CENTER_WIDTH_TOP`,
             * `MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM`,
             * `MAP_ALIGNMENT_CENTER_HEIGHT_LEFT`,
             * `MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT`.
             * @luaexample
             * tilemap_load_map(BASE_PATH .. "tilemap/corsair/corsair.tmx", MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM)
             */
            int LuaTilemapApi :: LoadMap( lua_State *pLuaState )  {

                const char  *szPath      = lua_tostring( pLuaState, 1 );
                int         nAlignment   = ( int ) luaL_optinteger( pLuaState, 2, ITileMap :: MAP_ALIGNMENT_CENTER );
                bool        bResult      = LuaEngineUtil :: GetTileMap( pLuaState ) -> LoadMap( szPath, ( ITileMap :: MapAlignment ) nAlignment );

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{tilemap_unload_map() -> success}
             * @luadoc
             * Unload the current map.
             */
            int LuaTilemapApi :: UnloadMap( lua_State *pLuaState )  {

                bool  bResult = LuaEngineUtil :: GetTileMap( pLuaState ) -> UnloadMap();

                lua_pushboolean( pLuaState, bResult );

                return 1;
            }

            /**
             * @luaname{tilemap_get_map_info() -> mapWidth, mapHeight, tileWidth, tileHeight}
             * @luadoc
             * Dimensions of the currently loaded map, in tiles
             * (`mapWidth`/`mapHeight`) and pixels-per-tile
             * (`tileWidth`/`tileHeight`). Returns `nil` if no map is
             * loaded.
             * @luaexample
             * local mapW, mapH, tileW, tileH = tilemap_get_map_info()
             */
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

            /**
             * @luaname{tilemap_get_layer(layerId) -> visible, opacity, offsetX, offsetY}
             * @luagroup{get_layer}
             * @luadoc
             * Every sprite lives on a numbered (or named) Tiled layer —
             * layer ids are whatever you assigned them to in the Tiled
             * editor, not something the engine invents (Caravellius
             * keeps its own names for them in
             * `caravellius/src/layerids.lua`).
             * @luaexample
             * local visible, opacity, offX, offY = tilemap_get_layer(2)
             */
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

            /**
             * @luaname{tilemap_get_layer_by_name(layerName) -> visible, opacity, offsetX, offsetY}
             * @luagroup{get_layer}
             */
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

            /**
             * @luaname{tilemap_set_layer(layerId, visible, opacity, offsetX, offsetY) -> success}
             * @luagroup{set_layer}
             * @luaexample
             * -- fade a layer out
             * tilemap_set_layer(2, true, 128, 0, 0)
             */
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

            /**
             * @luaname{tilemap_set_layer_by_name(layerName, visible, opacity, offsetX, offsetY) -> success}
             * @luagroup{set_layer}
             */
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

            /**
             * @luaname{tilemap_get_tile(row, col, layerId) -> gid, x, y, width, height}
             * @luadoc
             * Look up a single tile by its row/column position on a given
             * layer. `gid` is the tile's Tiled global id (`0`
             * conventionally means "no tile here" in Tiled, same as the
             * raw `.tmx` format). Returns `nil` if the layer or tile
             * position doesn't exist.
             * @luaexample
             * local gid, x, y, w, h = tilemap_get_tile(0, 0, 1)
             */
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

            /**
             * @luaname{tilemap_to_tile_matrix(worldX, worldY) -> row, col}
             * @luadoc
             * Convert a world/screen pixel coordinate to a tile
             * row/column — the inverse of the position math you'd
             * otherwise do by hand against `tilemap_get_map_info`'s tile
             * size. Returns `nil` if the coordinate is outside the map.
             * @luaexample
             * local row, col = tilemap_to_tile_matrix(100, 250)
             */
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

            /**
             * @luaname{tilemap_get_object_by_name(name) -> x, y, width, height}
             * @luadoc
             * Look up a level-design-authored object — a Tiled
             * `<object>` placed inside any `<objectgroup>` layer, e.g. a
             * trigger/marker — by name, as set in the map editor.
             * Searches every object-group layer in the loaded map.
             * Returns `nil` if no object with that name exists.
             * @luaexample
             * local x, y, w, h = tilemap_get_object_by_name("SubBossIntervalStart")
             *
             * if x ~= nil then
             *   camera_set_position(x, y)
             * end
             */
            int LuaTilemapApi :: GetObjectByName( lua_State *pLuaState )  {

                const char  *szObjectName = lua_tostring( pLuaState, 1 );
                stObject    object;

                if( !LuaEngineUtil :: GetTileMap( pLuaState ) -> GetObjectByName( szObjectName, object ) )  {
                    lua_pushnil( pLuaState );

                    return 1;
                }

                lua_pushinteger( pLuaState, object.dimension.pos.x );
                lua_pushinteger( pLuaState, object.dimension.pos.y );
                lua_pushinteger( pLuaState, object.dimension.size.nWidth );
                lua_pushinteger( pLuaState, object.dimension.size.nHeight );

                return 4;
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
                lua_register( pLuaState, "tilemap_get_object_by_name", LuaTilemapApi :: GetObjectByName );

                RegisterEnums( pLuaState );
            }
        }
    }
}
