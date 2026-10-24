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
             * @luaconstants{Map alignments}
             * @luadoc
             * The `alignment` argument `tilemap_load_map` optionally
             * takes, controlling how the map is positioned relative
             * to the viewport when it's smaller than the screen.
             */
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

            /** @brief The "no map" failure every map query shares. */
            static const char * const  NO_MAP_LOADED = "no map is loaded";

            /**
             * @brief Fetch layer nLayerId of the loaded map, with the reason on failure.
             */
            static bool FindLayer( ITileMap *pTileMap, int nLayerId, stLayer &layer, std :: string &strError )  {

                stMapInfo  mapInfo;

                if( !pTileMap -> GetMapInfo( mapInfo ) )  {
                    strError = NO_MAP_LOADED;
                    return false;
                }

                if( !pTileMap -> GetLayer( nLayerId, layer ) )  {
                    strError = "no layer " + std :: to_string( nLayerId ) + " in the loaded map";
                    return false;
                }

                return true;
            }

            /**
             * @brief Fetch the layer named strName of the loaded map, with the reason on failure.
             */
            static bool FindLayer( ITileMap *pTileMap, const std :: string &strName, stLayer &layer, std :: string &strError )  {

                stMapInfo  mapInfo;

                if( !pTileMap -> GetMapInfo( mapInfo ) )  {
                    strError = NO_MAP_LOADED;
                    return false;
                }

                if( !pTileMap -> GetLayer( strName.c_str(), layer ) )  {
                    strError = "no layer named '" + strName + "' in the loaded map";
                    return false;
                }

                return true;
            }

            /**
             * @brief Read tilemap_set_layer's visible/opacity/offsetX/offsetY arguments (from
             * index 2) into layer. The opacity is clamped to 0..255 by sunlight, so any integer
             * is accepted.
             */
            static bool ReadLayerValues( lua_State *pLuaState, stLayer &layer, std :: string &strError )  {

                int  nOpacity = 0;
                int  nOffsetX = 0;
                int  nOffsetY = 0;

                if( !LuaEngineUtil :: ReadBooleanArg( pLuaState, 2, "visible", layer.bVisible, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "opacity", nOpacity, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 4, "offsetX", nOffsetX, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 5, "offsetY", nOffsetY, strError ) )
                    return false;

                layer.nOpacity = nOpacity;
                layer.offset.x = nOffsetX;
                layer.offset.y = nOffsetY;

                return true;
            }

            /** @brief Push a layer's visible, opacity and offset - tilemap_get_layer's four results. */
            static int PushLayer( lua_State *pLuaState, const stLayer &layer )  {

                lua_pushboolean( pLuaState, layer.bVisible );
                lua_pushinteger( pLuaState, layer.nOpacity );
                lua_pushinteger( pLuaState, layer.offset.x );
                lua_pushinteger( pLuaState, layer.offset.y );

                return 4;
            }

            /**
             * @luaname{tilemap_load_map(path, alignment) -> true | nil, message}
             * @luaheading{Error reporting}
             * @luadoc
             * Every tilemap function reports a failure as `nil, "message"`
             * and never raises a Lua error; one with nothing else to return
             * returns `true`. With no map loaded, the queries fail with
             * `"no map is loaded"`; an unknown layer is
             * `"no layer N in the loaded map"`.
             *
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
             * `MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT`. Fails with
             * `"cannot load map '<path>'"` for a map that can't be loaded,
             * and for an unknown alignment.
             * @luaexample
             * tilemap_load_map(BASE_PATH .. "tilemap/corsair/corsair.tmx", MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM)
             */
            int LuaTilemapApi :: LoadMap( lua_State *pLuaState )  {

                std :: string  strError;
                std :: string  strPath;
                int            nAlignment = ITileMap :: MAP_ALIGNMENT_CENTER;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !lua_isnoneornil( pLuaState, 2 ) &&
                    !LuaEngineUtil :: ReadConstantArg( pLuaState, 2, "alignment", s_aMapAlignments,
                                                       sizeof( s_aMapAlignments ) / sizeof( s_aMapAlignments[0] ),
                                                       "MAP_ALIGNMENT_*", nAlignment, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaEngineUtil :: GetTileMap( pLuaState ) -> LoadMap( strPath.c_str(), ( ITileMap :: MapAlignment ) nAlignment ) )
                    return LuaEngineUtil :: PushError( pLuaState, "cannot load map '" + strPath + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{tilemap_unload_map() -> true | nil, message}
             * @luadoc
             * Unload the current map. Fails with `"no map is loaded"`.
             */
            int LuaTilemapApi :: UnloadMap( lua_State *pLuaState )  {

                ITileMap   *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );
                stMapInfo  mapInfo;

                if( !pTileMap -> GetMapInfo( mapInfo ) || !pTileMap -> UnloadMap() )
                    return LuaEngineUtil :: PushError( pLuaState, NO_MAP_LOADED );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{tilemap_get_map_info() -> mapWidth, mapHeight, tileWidth, tileHeight | nil, message}
             * @luadoc
             * Dimensions of the currently loaded map, in tiles
             * (`mapWidth`/`mapHeight`) and pixels-per-tile
             * (`tileWidth`/`tileHeight`). Returns `nil, "no map is loaded"`
             * if no map is loaded.
             * @luaexample
             * local mapW, mapH, tileW, tileH = tilemap_get_map_info()
             */
            int LuaTilemapApi :: GetMapInfo( lua_State *pLuaState )  {

                stMapInfo  mapInfo;

                if( !LuaEngineUtil :: GetTileMap( pLuaState ) -> GetMapInfo( mapInfo ) )
                    return LuaEngineUtil :: PushError( pLuaState, NO_MAP_LOADED );

                lua_pushinteger( pLuaState, mapInfo.mapSize.nWidth );
                lua_pushinteger( pLuaState, mapInfo.mapSize.nHeight );
                lua_pushinteger( pLuaState, mapInfo.tileSize.nWidth );
                lua_pushinteger( pLuaState, mapInfo.tileSize.nHeight );

                return 4;
            }

            /**
             * @luaname{tilemap_get_layer(layerId) -> visible, opacity, offsetX, offsetY | nil, message}
             * @luagroup{get_layer}
             * @luadoc
             * Every sprite lives on a numbered (or named) Tiled layer —
             * layer ids are whatever you assigned them to in the Tiled
             * editor, not something the engine invents (Caravellius
             * keeps its own names for them in
             * `caravellius/src/layerids.lua`).
             *
             * Returns the layer's `visible` flag, its `opacity` and its
             * pixel offset, or `nil, "message"` if there is no such layer. `opacity`
             * is an integer **0..255**, where `255` is fully opaque and
             * `0` fully transparent: the layer's Tiled opacity (0.0–1.0)
             * times 255, rounded, so a layer authored at `0.69` reads
             * `176`.
             * @luaexample
             * local visible, opacity, offX, offY = tilemap_get_layer(2)
             */
            int LuaTilemapApi :: GetLayer( lua_State *pLuaState )  {

                std :: string  strError;
                int            nLayerId = 0;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) ||
                    !FindLayer( LuaEngineUtil :: GetTileMap( pLuaState ), nLayerId, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushLayer( pLuaState, layer );
            }

            /**
             * @luaname{tilemap_get_layer_by_name(layerName) -> visible, opacity, offsetX, offsetY | nil, message}
             * @luagroup{get_layer}
             */
            int LuaTilemapApi :: GetLayerByName( lua_State *pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) ||
                    !FindLayer( LuaEngineUtil :: GetTileMap( pLuaState ), strName, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushLayer( pLuaState, layer );
            }

            /**
             * @luaname{tilemap_set_layer(layerId, visible, opacity, offsetX, offsetY) -> true | nil, message}
             * @luagroup{set_layer}
             * @luadoc
             * Write a layer's `visible` flag, `opacity` (an integer
             * **0..255**, `255` = fully opaque, as `tilemap_get_layer`
             * reads it; a value outside 0..255 is clamped) and pixel
             * offset. Returns `nil, "message"` if there is no such layer;
             * `visible` must be a boolean and the other three integers.
             *
             * It writes **all four** values, so to change one, read the
             * layer first and pass the others back unchanged — writing a
             * layer's own values back changes nothing. Hiding a layer this
             * way hides every sprite added to it too.
             * @luaexample
             * -- fade layer 2 to about half (128 of 255), keeping the rest as it is
             * local visible, _, offX, offY = tilemap_get_layer(2)
             * tilemap_set_layer(2, visible, 128, offX, offY)
             */
            int LuaTilemapApi :: SetLayer( lua_State *pLuaState )  {

                std :: string  strError;
                int            nLayerId = 0;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap  *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                /*
                 * Fetch the existing layer first so it's internal tmx_layer pointer
                 * (opaque to Lua) is preserved when writing it back.
                 */
                if( !FindLayer( pTileMap, nLayerId, layer, strError ) || !ReadLayerValues( pLuaState, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pTileMap -> SetLayer( nLayerId, layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer " + std :: to_string( nLayerId ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{tilemap_set_layer_by_name(layerName, visible, opacity, offsetX, offsetY) -> true | nil, message}
             * @luagroup{set_layer}
             */
            int LuaTilemapApi :: SetLayerByName( lua_State *pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap  *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !FindLayer( pTileMap, strName, layer, strError ) || !ReadLayerValues( pLuaState, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pTileMap -> SetLayer( strName.c_str(), layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer '" + strName + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{tilemap_get_tile(row, col, layerId) -> gid, x, y, width, height | nil, message}
             * @luadoc
             * Look up a single tile by its row/column position on a given
             * layer. `gid` is the tile's Tiled global id (`0`
             * conventionally means "no tile here" in Tiled, same as the
             * raw `.tmx` format). Returns `nil, "message"` if the layer or
             * tile position doesn't exist.
             * @luaexample
             * local gid, x, y, w, h = tilemap_get_tile(0, 0, 1)
             */
            int LuaTilemapApi :: GetTile( lua_State *pLuaState )  {

                std :: string      strError;
                stMatrixPosition   pos;
                int                nLayerId = 0;
                stLayer            layer;
                stTile             tile;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "row", pos.nTileRow, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "col", pos.nTileCol, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap  *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !FindLayer( pTileMap, nLayerId, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pTileMap -> GetTile( pos, layer, tile ) )
                    return LuaEngineUtil :: PushError( pLuaState, "no tile at row " + std :: to_string( pos.nTileRow ) + ", col " +
                                                       std :: to_string( pos.nTileCol ) + " of layer " + std :: to_string( nLayerId ) );

                lua_pushinteger( pLuaState, ( lua_Integer ) tile.nGID );
                lua_pushinteger( pLuaState, tile.dimension.pos.x );
                lua_pushinteger( pLuaState, tile.dimension.pos.y );
                lua_pushinteger( pLuaState, tile.dimension.size.nWidth );
                lua_pushinteger( pLuaState, tile.dimension.size.nHeight );

                return 5;
            }

            /**
             * @luaname{tilemap_to_tile_matrix(worldX, worldY) -> row, col | nil, message}
             * @luadoc
             * Convert a world/screen pixel coordinate to a tile
             * row/column — the inverse of the position math you'd
             * otherwise do by hand against `tilemap_get_map_info`'s tile
             * size. Returns `nil, "message"` if the coordinate is outside the
             * map or no map is loaded.
             * @luaexample
             * local row, col = tilemap_to_tile_matrix(100, 250)
             */
            int LuaTilemapApi :: ToTileMatrix( lua_State *pLuaState )  {

                std :: string                       strError;
                SunLight :: Base :: stCoordinate2D  coord;
                stMatrixPosition                    pos;
                stMapInfo                           mapInfo;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "worldX", coord.x, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "worldY", coord.y, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap  *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !pTileMap -> GetMapInfo( mapInfo ) )
                    return LuaEngineUtil :: PushError( pLuaState, NO_MAP_LOADED );

                if( !pTileMap -> TileMapToTileMatrix( coord, pos ) )
                    return LuaEngineUtil :: PushError( pLuaState, "(" + std :: to_string( coord.x ) + ", " + std :: to_string( coord.y ) + ") is outside the map" );

                lua_pushinteger( pLuaState, pos.nTileRow );
                lua_pushinteger( pLuaState, pos.nTileCol );

                return 2;
            }

            /**
             * @luaname{tilemap_get_object_by_name(name) -> x, y, width, height | nil, message}
             * @luadoc
             * Look up a level-design-authored object — a Tiled
             * `<object>` placed inside any `<objectgroup>` layer, e.g. a
             * trigger/marker — by name, as set in the map editor.
             * Searches every object-group layer in the loaded map.
             * Returns `nil, "message"` if no object with that name exists.
             * @luaexample
             * local x, y, w, h = tilemap_get_object_by_name("SubBossIntervalStart")
             *
             * if x ~= nil then
             *   camera_set_position(x, y)
             * end
             */
            int LuaTilemapApi :: GetObjectByName( lua_State *pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                stObject       object;
                stMapInfo      mapInfo;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "name", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap  *pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !pTileMap -> GetMapInfo( mapInfo ) )
                    return LuaEngineUtil :: PushError( pLuaState, NO_MAP_LOADED );

                if( !pTileMap -> GetObjectByName( strName.c_str(), object ) )
                    return LuaEngineUtil :: PushError( pLuaState, "no object named '" + strName + "' in the loaded map" );

                lua_pushinteger( pLuaState, object.dimension.pos.x );
                lua_pushinteger( pLuaState, object.dimension.pos.y );
                lua_pushinteger( pLuaState, object.dimension.size.nWidth );
                lua_pushinteger( pLuaState, object.dimension.size.nHeight );

                return 4;
            }

            void LuaTilemapApi :: RegisterEnums( lua_State *pLuaState )  {


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
