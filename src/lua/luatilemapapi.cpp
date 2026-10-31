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
#include <climits>
#include <cstdio>
#include <cstdlib>

extern "C"  {
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
            static bool FindLayer( ITileMap* pTileMap, int nLayerId, stLayer& layer, std :: string& strError )  {

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
            static bool FindLayer( ITileMap* pTileMap, const std :: string& strName, stLayer& layer, std :: string& strError )  {

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
             * index 2) into layer. The opacity is a number from 0 (transparent) to 1 (opaque), as
             * Tiled stores it (sunlight I9).
             */
            static bool ReadLayerValues( lua_State* pLuaState, stLayer& layer, std :: string& strError )  {

                double  fOpacity = 0.0;
                int     nOffsetX = 0;
                int     nOffsetY = 0;

                if( !LuaEngineUtil :: ReadBooleanArg( pLuaState, 2, "visible", layer.bVisible, strError ) ||
                    !LuaEngineUtil :: ReadNumberArgInRange( pLuaState, 3, "opacity", 0.0, 1.0, fOpacity, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 4, "offsetX", nOffsetX, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 5, "offsetY", nOffsetY, strError ) )
                    return false;

                layer.fOpacity = ( float ) fOpacity;
                layer.offset.x = nOffsetX;
                layer.offset.y = nOffsetY;

                return true;
            }

            /** @brief Push a layer's visible, opacity and offset - tilemap_get_layer's four results. */
            static int PushLayer( lua_State* pLuaState, const stLayer& layer )  {

                lua_pushboolean( pLuaState, layer.bVisible );
                lua_pushnumber( pLuaState, LuaEngineUtil :: ShortestDecimal( layer.fOpacity ) );
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
             *
             * **Loading over a loaded map** (switching stages) frees the old
             * map - its tile textures, tile animations and resources - and
             * keeps every sprite on its layer, as before (`tilemap_unload_map`
             * is what unloads sprites). The new map is read first: if it can't
             * be loaded, the call fails and the loaded map stays loaded. Up to
             * Scarab v0.1.25 every such load leaked the whole previous map, and
             * a map that failed to parse left no map at all. One map is loaded
             * at a time; every view shows it.
             * @luaexample
             * tilemap_load_map(BASE_PATH .. "tilemap/corsair/corsair.tmx", MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM)
             */
            int LuaTilemapApi :: LoadMap( lua_State* pLuaState )  {

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
             *
             * **Every sprite on a layer is unloaded with it**: its textures are
             * released, it has no active sequence (`sprite_get_active_sequence`
             * returns `-1`), it is on no layer and its collider is taken out, so it
             * never collides while unloaded. Its handle stays valid: to use it
             * after the next `tilemap_load_map`, call `sprite_configure_texture`,
             * `sprite_set_active_sequence` and `sprite_add_to_layer` again (it
             * then collides once per frame, as before). The collision rules
             * (`collision_add_rule`) are kept. Loading a map over a loaded one
             * (`tilemap_load_map` again) does NOT unload sprites.
             */
            int LuaTilemapApi :: UnloadMap( lua_State* pLuaState )  {

                ITileMap*   pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );
                stMapInfo   mapInfo;

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
            int LuaTilemapApi :: GetMapInfo( lua_State* pLuaState )  {

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
             * is a **number from `0` to `1`**, `1` fully opaque and `0` fully
             * transparent - the layer's Tiled opacity exactly as authored, so a
             * layer authored at `0.69` reads `0.69`. (Up to Scarab v0.1.25 it read
             * as an integer 0..255; since Scarab's V3 the integer was exact, but a
             * read-and-write-back still moved an authored `0.5` to `0.502`.)
             * @luaexample
             * local visible, opacity, offX, offY = tilemap_get_layer(2)
             */
            int LuaTilemapApi :: GetLayer( lua_State* pLuaState )  {

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
            int LuaTilemapApi :: GetLayerByName( lua_State* pLuaState )  {

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
             * Write a layer's `visible` flag, `opacity` (a **number from `0`
             * to `1`**, `1` = fully opaque, as `tilemap_get_layer` reads it;
             * a value outside `0`..`1` is an error) and pixel offset. Returns
             * `nil, "message"` if there is no such layer; `visible` must be a
             * boolean, the offsets integers.
             *
             * It writes **all four** values, so to change one, read the
             * layer first and pass the others back unchanged — writing a
             * layer's own values back changes nothing, exactly (its opacity,
             * offset, tint and parallax stay as the map authored them). Hiding
             * a layer this way hides every sprite added to it too.
             * @luaexample
             * -- fade layer 2 to half, keeping the rest as it is
             * local visible, _, offX, offY = tilemap_get_layer(2)
             * tilemap_set_layer(2, visible, 0.5, offX, offY)
             */
            int LuaTilemapApi :: SetLayer( lua_State* pLuaState )  {

                std :: string  strError;
                int            nLayerId = 0;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

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
            int LuaTilemapApi :: SetLayerByName( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !FindLayer( pTileMap, strName, layer, strError ) || !ReadLayerValues( pLuaState, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pTileMap -> SetLayer( strName.c_str(), layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer '" + strName + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /** @brief Push a layer's tint - tilemap_get_layer_tint's four results (r, g, b, a). */
            static int PushTint( lua_State* pLuaState, const stLayer& layer )  {

                lua_pushinteger( pLuaState, layer.tint.nRed );
                lua_pushinteger( pLuaState, layer.tint.nGreen );
                lua_pushinteger( pLuaState, layer.tint.nBlue );
                lua_pushinteger( pLuaState, layer.tint.nAlpha );

                return 4;
            }

            /**
             * @brief Read tilemap_set_layer_tint's r, g, b and a arguments (from index 2), each 0..255,
             * into layer.tint.
             */
            static bool ReadTint( lua_State* pLuaState, stLayer& layer, std :: string& strError )  {

                static const char * const  aszChannel[4] = { "r", "g", "b", "a" };

                lua_Integer  anChannel[4] = { 0, 0, 0, 0 };

                for( int nArg = 0; nArg < 4; nArg++ )
                    if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, 2 + nArg, aszChannel[nArg], 0, UCHAR_MAX, anChannel[nArg], strError ) )
                        return false;

                layer.tint.nRed   = ( unsigned char ) anChannel[0];
                layer.tint.nGreen = ( unsigned char ) anChannel[1];
                layer.tint.nBlue  = ( unsigned char ) anChannel[2];
                layer.tint.nAlpha = ( unsigned char ) anChannel[3];

                return true;
            }

            /**
             * @luaname{tilemap_get_layer_tint(layerId) -> r, g, b, a | nil, message}
             * @luagroup{layer_tint}
             * @luaheading{Layer tint}
             * @luadoc
             * A layer's **tint**: Tiled's `tintcolor` layer property, as four
             * integers `0`..`255`. Its `r`, `g`, `b` multiply into everything
             * textured the layer draws - the tiles of a tile layer, an image
             * layer, and every tile object of an object layer (at its own size,
             * turned or stretched) - so `255, 255, 255` (white, what a layer
             * without a `tintcolor` reads) changes nothing, and `255, 128, 128`
             * reddens it. Shape and text objects are never tinted.
             *
             * `a` is the alpha the map file declares: it is kept and read back,
             * but **ignored for drawing** - the layer's opacity
             * (`tilemap_set_layer`) is the only alpha control.
             *
             * A **group layer**'s tint (and opacity) multiply into every layer
             * inside it, nested groups too, the way Tiled draws them; shapes and
             * text inside take only the group's opacity.
             *
             * `tilemap_set_layer_tint` changes it at runtime; the next frame
             * draws with it. Writing back the values read changes nothing, and
             * `tilemap_set_layer` keeps a layer's tint as it is. Every function
             * returns `nil, "message"` for an unknown layer, with no map loaded,
             * or for a channel outside `0`..`255`.
             * @luaexample
             * local r, g, b, a = tilemap_get_layer_tint(3)
             * tilemap_set_layer_tint(3, 255, 160, 160, a)     -- a reddish light, same alpha
             * tilemap_set_layer_tint_by_name("background", 255, 255, 255, 255)   -- no tint
             */
            int LuaTilemapApi :: GetLayerTint( lua_State* pLuaState )  {

                std :: string  strError;
                int            nLayerId = 0;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) ||
                    !FindLayer( LuaEngineUtil :: GetTileMap( pLuaState ), nLayerId, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushTint( pLuaState, layer );
            }

            /**
             * @luaname{tilemap_get_layer_tint_by_name(layerName) -> r, g, b, a | nil, message}
             * @luagroup{layer_tint}
             */
            int LuaTilemapApi :: GetLayerTintByName( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) ||
                    !FindLayer( LuaEngineUtil :: GetTileMap( pLuaState ), strName, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushTint( pLuaState, layer );
            }

            /**
             * @luaname{tilemap_set_layer_tint(layerId, r, g, b, a) -> true | nil, message}
             * @luagroup{layer_tint}
             */
            int LuaTilemapApi :: SetLayerTint( lua_State* pLuaState )  {

                std :: string  strError;
                int            nLayerId = 0;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                // Read first: every other field (visible, opacity, offset, parallax) is written back as it is.
                if( !FindLayer( pTileMap, nLayerId, layer, strError ) || !ReadTint( pLuaState, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pTileMap -> SetLayer( nLayerId, layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer " + std :: to_string( nLayerId ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{tilemap_set_layer_tint_by_name(layerName, r, g, b, a) -> true | nil, message}
             * @luagroup{layer_tint}
             */
            int LuaTilemapApi :: SetLayerTintByName( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !FindLayer( pTileMap, strName, layer, strError ) || !ReadTint( pLuaState, layer, strError ) )
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
             * raw `.tmx` format). `x`/`y`/`width`/`height` are the tile's
             * rectangle in **map pixels** (its cell, plus the layer's own
             * offset), whatever the camera or the zoom - the same rectangle a
             * tile collision reports. Returns `nil, "message"` if the layer or
             * tile position doesn't exist.
             * @luaexample
             * local gid, x, y, w, h = tilemap_get_tile(0, 0, 1)
             */
            int LuaTilemapApi :: GetTile( lua_State* pLuaState )  {

                std :: string      strError;
                stMatrixPosition   pos;
                int                nLayerId = 0;
                stLayer            layer;
                stTile             tile;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "row", pos.nTileRow, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "col", pos.nTileCol, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

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
             * @luaname{tilemap_to_tile_matrix(mapX, mapY) -> row, col | nil, message}
             * @luadoc
             * Convert a **map position** (map pixels, before the camera, the
             * zoom or the viewport's origin) to a tile row/column — the inverse
             * of the position math you'd otherwise do by hand against
             * `tilemap_get_map_info`'s tile size. A world-space sprite's
             * position is a map position as it is; a view-relative sprite's
             * map position is its own plus the camera position
             * (`camera_get_position`). Returns `nil, "message"` if the position
             * is outside the map or no map is loaded. Up to Scarab v0.1.25 the
             * viewport's origin and the camera were added to the position
             * first, so the answer was shifted (by 10 px with the default
             * viewport) and the last strip of the map counted as outside.
             * @luaexample
             * local row, col = tilemap_to_tile_matrix(100, 250)
             */
            int LuaTilemapApi :: ToTileMatrix( lua_State* pLuaState )  {

                std :: string                       strError;
                SunLight :: Base :: stCoordinate2D  coord;
                stMatrixPosition                    pos;
                stMapInfo                           mapInfo;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "worldX", coord.x, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "worldY", coord.y, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

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
            int LuaTilemapApi :: GetObjectByName( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                stObject       object;
                stMapInfo      mapInfo;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "name", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

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

            /**
             * @luaname{tilemap_get_tile_frame(row, col, layerId) -> gid | nil, message}
             * @luagroup{tile_frame}
             * @luaheading{Animated tiles}
             * @luadoc
             * The gid a cell of a tile layer **draws right now**: for a tile
             * with a Tiled animation, the gid of the frame its animation shows
             * at this moment; for a static tile, the tile's own gid
             * (`tilemap_get_tile`'s). Flip bits are removed.
             * `tilemap_get_object_tile_frame` is the same for a tile object,
             * found by name as `tilemap_get_object_by_name` finds it.
             *
             * A Tiled tile animation runs **by time**, as Tiled shows it: the
             * frame on screen is the time since `tilemap_load_map`, modulo the
             * animation's cycle (the sum of its frame durations), walked
             * through the durations - frame k is shown from the sum of the
             * durations before it, for its own duration, in every cycle. So
             * every cell and tile object showing the same tile is in step, and
             * a tile that is not drawn (scrolled off screen, on a hidden layer,
             * masked out of a view) does not pause: it shows the frame for the
             * time now. An animation whose durations are all `0` shows its
             * first frame. Up to Scarab v0.1.25 an animation advanced only
             * while drawn, from where it stopped, its first frame was shown
             * twice each cycle, and a duration that was not a whole number of
             * frames was held until the next frame.
             *
             * Asking only reads: it never changes what is drawn. Failures are
             * `nil, "message"`: no map loaded, no such layer, a layer that is
             * not a tile layer, a cell off the map, an empty cell; for an
             * object, no object of that name or one that is not a tile object.
             * A hidden layer still answers.
             * @luaexample
             * -- the torch at row 4, col 7 of layer 1: is its flame frame up?
             * local FLAME_UP_GID = 12
             * local flameUp = tilemap_get_tile_frame(4, 7, 1) == FLAME_UP_GID
             * local doorGid = tilemap_get_object_tile_frame("door")
             */
            int LuaTilemapApi :: GetTileFrame( lua_State* pLuaState )  {

                std :: string      strError;
                stMatrixPosition   pos;
                int                nLayerId = 0;
                stLayer            layer;
                stMapInfo          mapInfo;
                unsigned int       nGID = 0;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "row", pos.nTileRow, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "col", pos.nTileCol, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !FindLayer( pTileMap, nLayerId, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: string  strCell = "row " + std :: to_string( pos.nTileRow ) + ", col " + std :: to_string( pos.nTileCol );

                // sunlight's GetTileFrame answers only false for every refusal: the reasons it can't tell
                // apart are checked here first, so the one left when it still says false is an empty cell.
                if( ( layer.pLayer == nullptr ) || ( layer.pLayer -> type != L_LAYER ) )
                    return LuaEngineUtil :: PushError( pLuaState, "layer " + std :: to_string( nLayerId ) + " is not a tile layer" );

                pTileMap -> GetMapInfo( mapInfo );

                if( ( pos.nTileRow < 0 ) || ( pos.nTileRow >= mapInfo.mapSize.nHeight ) ||
                    ( pos.nTileCol < 0 ) || ( pos.nTileCol >= mapInfo.mapSize.nWidth ) )
                    return LuaEngineUtil :: PushError( pLuaState, strCell + " is off the map (" + std :: to_string( mapInfo.mapSize.nHeight ) +
                                                       " rows, " + std :: to_string( mapInfo.mapSize.nWidth ) + " cols)" );

                if( !pTileMap -> GetTileFrame( nLayerId, pos, nGID ) )
                    return LuaEngineUtil :: PushError( pLuaState, "no tile at " + strCell + " of layer " + std :: to_string( nLayerId ) + " (the cell is empty)" );

                lua_pushinteger( pLuaState, ( lua_Integer ) nGID );

                return 1;
            }

            /**
             * @luaname{tilemap_get_object_tile_frame(name) -> gid | nil, message}
             * @luagroup{tile_frame}
             */
            int LuaTilemapApi :: GetObjectTileFrame( lua_State* pLuaState )  {

                std :: string  strError;
                std :: string  strName;
                stObject       object;
                stMapInfo      mapInfo;
                unsigned int   nGID = 0;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "name", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !pTileMap -> GetMapInfo( mapInfo ) )
                    return LuaEngineUtil :: PushError( pLuaState, NO_MAP_LOADED );

                if( !pTileMap -> GetObjectByName( strName.c_str(), object ) )
                    return LuaEngineUtil :: PushError( pLuaState, "no object named '" + strName + "' in the loaded map" );

                if( !pTileMap -> GetObjectTileFrame( strName.c_str(), nGID ) )
                    return LuaEngineUtil :: PushError( pLuaState, "object '" + strName + "' is not a tile object" );

                lua_pushinteger( pLuaState, ( lua_Integer ) nGID );

                return 1;
            }

            /**
             * @brief Register this module's constant groups as Lua globals (see each group's @luaconstants).
             * @param pLuaState Lua state to be used by engine call;
             */
            void LuaTilemapApi :: RegisterEnums( lua_State* pLuaState )  {


                LuaEngineUtil :: RegisterConstants( pLuaState, s_aMapAlignments, sizeof( s_aMapAlignments ) / sizeof( s_aMapAlignments[0] ) );
            }

            /**
             * @brief Register the tile map management Lua-callable functions and enums.
             */
            void LuaTilemapApi :: Register( lua_State* pLuaState )  {

                lua_register( pLuaState, "tilemap_load_map", LuaTilemapApi :: LoadMap );
                lua_register( pLuaState, "tilemap_unload_map", LuaTilemapApi :: UnloadMap );
                lua_register( pLuaState, "tilemap_get_map_info", LuaTilemapApi :: GetMapInfo );
                lua_register( pLuaState, "tilemap_get_layer", LuaTilemapApi :: GetLayer );
                lua_register( pLuaState, "tilemap_get_layer_by_name", LuaTilemapApi :: GetLayerByName );
                lua_register( pLuaState, "tilemap_set_layer", LuaTilemapApi :: SetLayer );
                lua_register( pLuaState, "tilemap_set_layer_by_name", LuaTilemapApi :: SetLayerByName );
                lua_register( pLuaState, "tilemap_get_layer_tint", LuaTilemapApi :: GetLayerTint );
                lua_register( pLuaState, "tilemap_get_layer_tint_by_name", LuaTilemapApi :: GetLayerTintByName );
                lua_register( pLuaState, "tilemap_set_layer_tint", LuaTilemapApi :: SetLayerTint );
                lua_register( pLuaState, "tilemap_set_layer_tint_by_name", LuaTilemapApi :: SetLayerTintByName );
                lua_register( pLuaState, "tilemap_get_tile", LuaTilemapApi :: GetTile );
                lua_register( pLuaState, "tilemap_to_tile_matrix", LuaTilemapApi :: ToTileMatrix );
                lua_register( pLuaState, "tilemap_get_object_by_name", LuaTilemapApi :: GetObjectByName );
                lua_register( pLuaState, "tilemap_get_tile_frame", LuaTilemapApi :: GetTileFrame );
                lua_register( pLuaState, "tilemap_get_object_tile_frame", LuaTilemapApi :: GetObjectTileFrame );

                RegisterEnums( pLuaState );
            }
        }
    }
}
