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
#include "lua/luaworldapi.h"
#include "lua/luaspriteapi.h"
#include "engine/spritepool.h"
#include "lua/luaengineutil.h"
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>

/** The "no map" failure every map query shares. */
#define __NO_MAP_LOADED  "no map is loaded"

/** A layer tint's channels: r, g, b, a. */
#define __RGBA_CHANNELS  4

extern "C"  {
  #include "lauxlib.h"
}



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
                { "MAP_ALIGNMENT_CENTER", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER },
                { "MAP_ALIGNMENT_TOP_RIGHT", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_RIGHT },
                { "MAP_ALIGNMENT_TOP_LEFT", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_TOP_LEFT },
                { "MAP_ALIGNMENT_BOTTOM_RIGHT", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_BOTTOM_RIGHT },
                { "MAP_ALIGNMENT_BOTTOM_LEFT", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_BOTTOM_LEFT },
                { "MAP_ALIGNMENT_CENTER_WIDTH_TOP", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER_WIDTH_TOP },
                { "MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER_WIDTH_BOTTOM },
                { "MAP_ALIGNMENT_CENTER_HEIGHT_LEFT", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER_HEIGHT_LEFT },
                { "MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT", SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER_HEIGHT_RIGHT },
            };

            /**
             * @brief Fetch layer nLayerId of the loaded map, with the reason on failure.
             */
            template<typename TileSource> static bool FindLayer( TileSource* pTileMap, int nLayerId, SunLight :: TileMap :: stLayer& layer, std :: string& strError )  {

                SunLight :: TileMap :: stMapInfo  mapInfo;

                if( !pTileMap -> GetMapInfo( mapInfo ) )  {
                    strError = __NO_MAP_LOADED;
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
            template<typename TileSource> static bool FindLayer( TileSource* pTileMap, const std :: string& strName, SunLight :: TileMap :: stLayer& layer, std :: string& strError )  {

                SunLight :: TileMap :: stMapInfo  mapInfo;

                if( !pTileMap -> GetMapInfo( mapInfo ) )  {
                    strError = __NO_MAP_LOADED;
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
            static bool ReadLayerValues( lua_State* pLuaState, SunLight :: TileMap :: stLayer& layer, std :: string& strError )  {

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
            static int PushLayer( lua_State* pLuaState, const SunLight :: TileMap :: stLayer& layer )  {

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
                int            nAlignment = SunLight :: TileMap :: ITileMap :: MAP_ALIGNMENT_CENTER;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "path", strPath, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !lua_isnoneornil( pLuaState, 2 ) &&
                    !LuaEngineUtil :: ReadConstantArg( pLuaState, 2, "alignment", s_aMapAlignments,
                                                       sizeof( s_aMapAlignments ) / sizeof( s_aMapAlignments[0] ),
                                                       "MAP_ALIGNMENT_*", nAlignment, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !LuaEngineUtil :: GetTileMap( pLuaState ) -> LoadMap( strPath.c_str(), ( SunLight :: TileMap :: ITileMap :: MapAlignment ) nAlignment ) )
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

                SunLight :: TileMap :: ITileMap*   pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );
                SunLight :: TileMap :: stMapInfo   mapInfo;

                if( !pTileMap -> GetMapInfo( mapInfo ) || !pTileMap -> UnloadMap() )
                    return LuaEngineUtil :: PushError( pLuaState, __NO_MAP_LOADED );

                // the default world's sprites are on no layer now (item C7: the sprite pool keeps where each one is)
                static_cast<SpritePool *>( LuaEngineUtil :: GetGlobalPointer( pLuaState, LuaSpriteApi :: SPRITE_POOL_GLOBAL ) ) -> ForgetWorld( 0 );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief The loaded map's size in tiles and its tile size, of a world (the default world when none is given).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{tilemap_get_map_info([world]) -> mapWidth, mapHeight, tileWidth, tileHeight | nil, message}
             * @luadoc
             * Dimensions of the currently loaded map, in tiles
             * (`mapWidth`/`mapHeight`) and pixels-per-tile
             * (`tileWidth`/`tileHeight`). Returns `nil, "no map is loaded"`
             * if no map is loaded. The optional `world` reads another world's
             * map instead of the default world's (its first map, when it has
             * several - `world_get_bounds` gives the whole world's size).
             * @luaexample
             * local mapW, mapH, tileW, tileH = tilemap_get_map_info()
             * local dungeonW, dungeonH = tilemap_get_map_info( dungeon )
             */
            int LuaTilemapApi :: GetMapInfo( lua_State* pLuaState )  {

                std :: string                                     strError;
                SunLight :: TileMap :: stMapInfo                  mapInfo;
                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 1, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pWorld -> GetMapInfo( mapInfo ) )
                    return LuaEngineUtil :: PushError( pLuaState, __NO_MAP_LOADED );

                lua_pushinteger( pLuaState, mapInfo.mapSize.nWidth );
                lua_pushinteger( pLuaState, mapInfo.mapSize.nHeight );
                lua_pushinteger( pLuaState, mapInfo.tileSize.nWidth );
                lua_pushinteger( pLuaState, mapInfo.tileSize.nHeight );

                return 4;
            }

            /**
             * @brief A layer's visible flag, opacity and offset, in a world (the default world when none is given).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{tilemap_get_layer(layerId [, world]) -> visible, opacity, offsetX, offsetY | nil, message}
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
             *
             * The optional `world` reads a layer of another world (in a world of
             * several maps, `layerId` is a world layer id and the layer is the
             * first map's that has it). The layer setters (`tilemap_set_layer`,
             * tint, parallax) take the same optional `world` (sunlight I21): in a
             * world of several maps the change reaches that layer in EVERY map
             * that has it, so a layer looks the same across map edges.
             * @luaexample
             * local visible, opacity, offX, offY = tilemap_get_layer(2)
             * local shown = tilemap_get_layer_by_name( "objects", dungeon )
             */
            int LuaTilemapApi :: GetLayer( lua_State* pLuaState )  {

                std :: string                         strError;
                int                                   nLayerId = 0;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 2, strError );

                if( !pWorld || !FindLayer( pWorld.get(), nLayerId, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushLayer( pLuaState, layer );
            }

            /**
             * @brief A layer's visible flag, opacity and offset by name, in a world (the default world when none is given).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{tilemap_get_layer_by_name(layerName [, world]) -> visible, opacity, offsetX, offsetY | nil, message}
             * @luagroup{get_layer}
             */
            int LuaTilemapApi :: GetLayerByName( lua_State* pLuaState )  {

                std :: string                         strError;
                std :: string                         strName;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 2, strError );

                if( !pWorld || !FindLayer( pWorld.get(), strName, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushLayer( pLuaState, layer );
            }

            /**
             * @luaname{tilemap_set_layer(layerId, visible, opacity, offsetX, offsetY [, world]) -> true | nil, message}
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

                std :: string                         strError;
                int                                   nLayerId = 0;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 6, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: IWorld*  pTileMap = pWorld.get();   // the default world when none is given

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
             * @luaname{tilemap_set_layer_by_name(layerName, visible, opacity, offsetX, offsetY [, world]) -> true | nil, message}
             * @luagroup{set_layer}
             */
            int LuaTilemapApi :: SetLayerByName( lua_State* pLuaState )  {

                std :: string                         strError;
                std :: string                         strName;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 6, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: IWorld*  pTileMap = pWorld.get();   // the default world when none is given

                if( !FindLayer( pTileMap, strName, layer, strError ) || !ReadLayerValues( pLuaState, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pTileMap -> SetLayer( strName.c_str(), layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer '" + strName + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /** @brief Push a layer's tint - tilemap_get_layer_tint's four results (r, g, b, a). */
            static int PushTint( lua_State* pLuaState, const SunLight :: TileMap :: stLayer& layer )  {

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
            static bool ReadTint( lua_State* pLuaState, SunLight :: TileMap :: stLayer& layer, std :: string& strError )  {

                static const char * const  aszChannel[__RGBA_CHANNELS] = { "r", "g", "b", "a" };

                lua_Integer  anChannel[__RGBA_CHANNELS] = { 0, 0, 0, 0 };

                for( int nArg = 0; nArg < __RGBA_CHANNELS; nArg++ )
                    if( !LuaEngineUtil :: ReadIntegerArgInRange( pLuaState, 2 + nArg, aszChannel[nArg], 0, UCHAR_MAX, anChannel[nArg], strError ) )
                        return false;

                layer.tint.nRed   = ( unsigned char ) anChannel[0];
                layer.tint.nGreen = ( unsigned char ) anChannel[1];
                layer.tint.nBlue  = ( unsigned char ) anChannel[2];
                layer.tint.nAlpha = ( unsigned char ) anChannel[3];

                return true;
            }

            /**
             * @luaname{tilemap_get_layer_tint(layerId [, world]) -> r, g, b, a | nil, message}
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
             * draws with it. All four take an optional last `world` (the default
             * world when left out); in a world of several maps a setter tints that
             * layer in every map that has it. Writing back the values read changes nothing, and
             * `tilemap_set_layer` keeps a layer's tint as it is. Every function
             * returns `nil, "message"` for an unknown layer, with no map loaded,
             * or for a channel outside `0`..`255`.
             * @luaexample
             * local r, g, b, a = tilemap_get_layer_tint(3)
             * tilemap_set_layer_tint(3, 255, 160, 160, a)     -- a reddish light, same alpha
             * tilemap_set_layer_tint_by_name("background", 255, 255, 255, 255)   -- no tint
             */
            int LuaTilemapApi :: GetLayerTint( lua_State* pLuaState )  {

                std :: string                         strError;
                int                                   nLayerId = 0;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 2, strError );

                if( !pWorld || !FindLayer( pWorld.get(), nLayerId, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushTint( pLuaState, layer );
            }

            /**
             * @luaname{tilemap_get_layer_tint_by_name(layerName [, world]) -> r, g, b, a | nil, message}
             * @luagroup{layer_tint}
             */
            int LuaTilemapApi :: GetLayerTintByName( lua_State* pLuaState )  {

                std :: string                         strError;
                std :: string                         strName;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 2, strError );

                if( !pWorld || !FindLayer( pWorld.get(), strName, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushTint( pLuaState, layer );
            }

            /**
             * @luaname{tilemap_set_layer_tint(layerId, r, g, b, a [, world]) -> true | nil, message}
             * @luagroup{layer_tint}
             */
            int LuaTilemapApi :: SetLayerTint( lua_State* pLuaState )  {

                std :: string                         strError;
                int                                   nLayerId = 0;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 6, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: IWorld*  pTileMap = pWorld.get();   // the default world when none is given

                // Read first: every other field (visible, opacity, offset, parallax) is written back as it is.
                if( !FindLayer( pTileMap, nLayerId, layer, strError ) || !ReadTint( pLuaState, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pTileMap -> SetLayer( nLayerId, layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer " + std :: to_string( nLayerId ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @luaname{tilemap_set_layer_tint_by_name(layerName, r, g, b, a [, world]) -> true | nil, message}
             * @luagroup{layer_tint}
             */
            int LuaTilemapApi :: SetLayerTintByName( lua_State* pLuaState )  {

                std :: string                         strError;
                std :: string                         strName;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 6, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: IWorld*  pTileMap = pWorld.get();   // the default world when none is given

                if( !FindLayer( pTileMap, strName, layer, strError ) || !ReadTint( pLuaState, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                if( !pTileMap -> SetLayer( strName.c_str(), layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer '" + strName + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Push a layer's parallax factors, each as the shortest decimal that reads back as its
             * float (0.5 reads 0.5, an authored 0.3 reads 0.3).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param layer The layer;
             * @return int number of return data (2);
             */
            static int PushParallax( lua_State* pLuaState, const SunLight :: TileMap :: stLayer& layer )  {

                lua_pushnumber( pLuaState, LuaEngineUtil :: ShortestDecimal( layer.fParallaxX ) );
                lua_pushnumber( pLuaState, LuaEngineUtil :: ShortestDecimal( layer.fParallaxY ) );

                return 2;
            }

            /**
             * @brief Read the fx, fy arguments (2 and 3) into a layer: any finite number that a float can hold
             * (1 = a normal layer, 0 = fixed on the view, negative = the other way).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @param layer The layer to write the factors into;
             * @param strError Receives the reason on failure;
             * @return true if both factors are usable;
             */
            static bool ReadParallax( lua_State* pLuaState, SunLight :: TileMap :: stLayer& layer, std :: string& strError )  {

                static const char * const  aszFactor[2] = { "fx", "fy" };

                double  afFactor[2] = { 0, 0 };

                for( int nArg = 0; nArg < 2; nArg++ )  {
                    if( !LuaEngineUtil :: ReadNumberArg( pLuaState, 2 + nArg, aszFactor[nArg], afFactor[nArg], strError ) )
                        return false;

                    if( !std :: isfinite( ( float ) afFactor[nArg] ) )  {
                        strError = std :: string( "argument '" ) + aszFactor[nArg] + "' must be a finite number";
                        return false;
                    }
                }

                layer.fParallaxX = ( float ) afFactor[0];
                layer.fParallaxY = ( float ) afFactor[1];

                return true;
            }

            /**
             * @brief A layer's parallax factors (sunlight L1: stLayer::fParallaxX/fParallaxY, Tiled's
             * parallaxx/parallaxy).
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{tilemap_get_layer_parallax(layerId [, world]) -> fx, fy | nil, message}
             * @luagroup{layer_parallax}
             * @luaheading{Layer parallax}
             * @luadoc
             * A layer's **parallax factors**, Tiled's `parallaxx`/`parallaxy` layer properties: how fast
             * the layer moves with the camera, per axis. `1` (what a layer without them reads) is a
             * normal layer; `0.5` moves at half the camera's speed, so it looks far away; `0` stays fixed
             * on the view; above `1` moves faster (a foreground); a negative factor moves the other way.
             * A layer is drawn shifted by `(1 - factor) x (view centre + the map's parallax origin)`, as
             * Tiled draws it, in every view about its own centre; a group's factors multiply into the
             * layers inside it.
             *
             * All four take an optional last `world` (the default world when left out); in a world of
             * several maps a setter changes that layer in every map that has it.
             *
             * `tilemap_set_layer_parallax` changes them at runtime (any finite number); the next frame
             * draws with them, and they read back as written. Loading the map again brings back the
             * values it declares. Every function returns `nil, "message"` for an unknown layer, with no
             * map loaded, or for a factor that is not a finite number.
             *
             * **What parallax moves, and what it does not:** a layer's tiles, objects and images; a
             * world-space sprite on the layer (`sprite_set_world_space`), with the layer's own factor (not
             * a parent group's); and what a screen-space sprite on the layer collides with (what is drawn
             * is what collides). It does **not** move a screen-space sprite, the shapes `shape_draw_*`
             * (they always use the view's own camera), or tile picking: `tilemap_to_tile_matrix` and
             * `tilemap_get_tile` work in plain map coordinates, so picking a tile on a parallax layer
             * under a screen point needs the layer's shift added first.
             * @luaexample
             * local fx, fy = tilemap_get_layer_parallax(3)          -- eg. 0.5, 0.5 from the map file
             * tilemap_set_layer_parallax_by_name("sky", 0.1, 0.1)  -- push the sky further away
             * tilemap_set_layer_parallax(2, 0, 0)                  -- a backdrop fixed on the view
             */
            int LuaTilemapApi :: GetLayerParallax( lua_State* pLuaState )  {

                std :: string                         strError;
                int                                   nLayerId = 0;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 2, strError );

                if( !pWorld || !FindLayer( pWorld.get(), nLayerId, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushParallax( pLuaState, layer );
            }

            /**
             * @brief A layer's parallax factors, the layer given by name.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{tilemap_get_layer_parallax_by_name(layerName [, world]) -> fx, fy | nil, message}
             * @luagroup{layer_parallax}
             */
            int LuaTilemapApi :: GetLayerParallaxByName( lua_State* pLuaState )  {

                std :: string                         strError;
                std :: string                         strName;
                SunLight :: TileMap :: stLayer        layer;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 2, strError );

                if( !pWorld || !FindLayer( pWorld.get(), strName, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                return PushParallax( pLuaState, layer );
            }

            /**
             * @brief Change a layer's parallax factors at runtime, through ITileMap::SetLayer: the layer is
             * read first, so every other field (visible, opacity, offset, tint) is written back as it is.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{tilemap_set_layer_parallax(layerId, fx, fy [, world]) -> true | nil, message}
             * @luagroup{layer_parallax}
             */
            int LuaTilemapApi :: SetLayerParallax( lua_State* pLuaState )  {

                std :: string                         strError;
                int                                   nLayerId = 0;
                SunLight :: TileMap :: stLayer        layer;
                SunLight :: TileMap :: stLayer        factors;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "layerId", nLayerId, strError ) || !ReadParallax( pLuaState, factors, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 4, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: IWorld*  pTileMap = pWorld.get();   // the default world when none is given

                if( !FindLayer( pTileMap, nLayerId, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                layer.fParallaxX = factors.fParallaxX;
                layer.fParallaxY = factors.fParallaxY;

                if( !pTileMap -> SetLayer( nLayerId, layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer " + std :: to_string( nLayerId ) );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief Change a layer's parallax factors at runtime, the layer given by name.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{tilemap_set_layer_parallax_by_name(layerName, fx, fy [, world]) -> true | nil, message}
             * @luagroup{layer_parallax}
             */
            int LuaTilemapApi :: SetLayerParallaxByName( lua_State* pLuaState )  {

                std :: string                         strError;
                std :: string                         strName;
                SunLight :: TileMap :: stLayer        layer;
                SunLight :: TileMap :: stLayer        factors;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "layerName", strName, strError ) || !ReadParallax( pLuaState, factors, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                std :: shared_ptr<SunLight :: TileMap :: IWorld>  pWorld = LuaWorldApi :: ReadOptionalWorldArg( pLuaState, 4, strError );

                if( !pWorld )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: IWorld*  pTileMap = pWorld.get();   // the default world when none is given

                if( !FindLayer( pTileMap, strName, layer, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                layer.fParallaxX = factors.fParallaxX;
                layer.fParallaxY = factors.fParallaxY;

                if( !pTileMap -> SetLayer( strName.c_str(), layer ) )
                    return LuaEngineUtil :: PushError( pLuaState, "the engine refused to change layer '" + strName + "'" );

                return LuaEngineUtil :: PushTrue( pLuaState );
            }

            /**
             * @brief The loaded map's parallax origin (Tiled's parallaxoriginx/y), read-only: sunlight reads
             * it from the map and has no setter.
             *
             * @param pLuaState Lua state to be used by engine call;
             * @return int number of return data (if any - required by lua engine)
             *
             * @luaname{tilemap_get_parallax_origin() -> x, y | nil, message}
             * @luagroup{layer_parallax}
             */
            int LuaTilemapApi :: GetParallaxOrigin( lua_State* pLuaState )  {

                SunLight :: TileMap :: stMapInfo  mapInfo;

                if( !LuaEngineUtil :: GetTileMap( pLuaState ) -> GetMapInfo( mapInfo ) || ( mapInfo.pMap == nullptr ) )
                    return LuaEngineUtil :: PushError( pLuaState, __NO_MAP_LOADED );

                lua_pushnumber( pLuaState, mapInfo.pMap -> parallaxoriginx );
                lua_pushnumber( pLuaState, mapInfo.pMap -> parallaxoriginy );

                return 2;
            }

            /**
             * @luaname{tilemap_get_tile(row, col, layerId) -> gid, x, y, width, height | nil, message}
             * @luagroup{tiles}
             * @luaheading{Tiles and map projections}
             * @luadoc
             * Look up a single tile by its row/column position on a given
             * layer. `gid` is the tile's Tiled global id (`0`
             * conventionally means "no tile here" in Tiled, same as the
             * raw `.tmx` format). `x`/`y`/`width`/`height` are the tile's
             * rectangle in **map pixels** (its cell, plus the layer's own
             * offset), whatever the camera or the zoom - the same rectangle a
             * tile collision reports. Returns `nil, "message"` if the layer or
             * tile position doesn't exist.
             *
             * **Map projections:** every function here, and the camera, views and collisions, works on
             * Tiled's orthogonal, isometric, staggered and hexagonal maps alike (`resources/tilemap/isometric/`,
             * `staggered/`, `hexagonal/` and `orthogonal/`, and `samples/projections`). On a non-orthogonal map the
             * rectangle is the tile image's, placed where the projection puts the cell: the cell's own
             * diamond or hexagon is the bottom `tileHeight` pixels of it (`tilemap_get_map_info`), and the
             * cells overlap their neighbours' rectangles. A tile collision tests the tile shapes' bounding
             * boxes, so a sprite near the corner of a diamond can touch an obstacle it does not cover, and
             * on an isometric map one sprite can touch several adjacent obstacles at once.
             * `tilemap_to_tile_matrix` picks by the cell's real outline (sunlight I17; before, on isometric
             * and staggered maps, a cell's centre picked a neighbour), and a point inside the map's pixel
             * rectangle but outside every cell - an isometric map's empty corners - picks no cell.
             * @luaexample
             * local gid, x, y, w, h = tilemap_get_tile(0, 0, 1)
             * local _, _, _, tileHeight = tilemap_get_map_info()
             * local row, col = tilemap_to_tile_matrix(x + w // 2, y + h - tileHeight // 2)   -- the cell's centre: row 0, col 0
             */
            int LuaTilemapApi :: GetTile( lua_State* pLuaState )  {

                std :: string                             strError;
                SunLight :: TileMap :: stMatrixPosition   pos;
                int                                       nLayerId = 0;
                SunLight :: TileMap :: stLayer            layer;
                SunLight :: TileMap :: stTile             tile;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "row", pos.nTileRow, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "col", pos.nTileCol, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

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
             * @luagroup{tiles}
             * @luadoc
             * Convert a **map position** (map pixels, before the camera, the
             * zoom or the viewport's origin) to a tile row/column — the inverse
             * of the position math you'd otherwise do by hand against
             * `tilemap_get_map_info`'s tile size. A world-space sprite's
             * position is a map position as it is; a view-relative sprite's
             * map position is its own plus the camera position
             * (`camera_get_position`). Returns `nil, "message"` if the position
             * is outside the map (or outside every cell, on a non-orthogonal
             * map) or no map is loaded. Up to Scarab v0.1.25 the
             * viewport's origin and the camera were added to the position
             * first, so the answer was shifted (by 10 px with the default
             * viewport) and the last strip of the map counted as outside.
             * @luaexample
             * local row, col = tilemap_to_tile_matrix(100, 250)
             */
            int LuaTilemapApi :: ToTileMatrix( lua_State* pLuaState )  {

                std :: string                                              strError;
                SunLight :: Base :: stCoordinate2D                         coord;
                SunLight :: TileMap :: stMatrixPosition                    pos;
                SunLight :: TileMap :: stMapInfo                           mapInfo;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "worldX", coord.x, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "worldY", coord.y, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !pTileMap -> GetMapInfo( mapInfo ) )
                    return LuaEngineUtil :: PushError( pLuaState, __NO_MAP_LOADED );

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

                std :: string                         strError;
                std :: string                         strName;
                SunLight :: TileMap :: stObject       object;
                SunLight :: TileMap :: stMapInfo      mapInfo;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "name", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !pTileMap -> GetMapInfo( mapInfo ) )
                    return LuaEngineUtil :: PushError( pLuaState, __NO_MAP_LOADED );

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

                std :: string                             strError;
                SunLight :: TileMap :: stMatrixPosition   pos;
                int                                       nLayerId = 0;
                SunLight :: TileMap :: stLayer            layer;
                SunLight :: TileMap :: stMapInfo          mapInfo;
                unsigned int                              nGID = 0;

                if( !LuaEngineUtil :: ReadIntArg( pLuaState, 1, "row", pos.nTileRow, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 2, "col", pos.nTileCol, strError ) ||
                    !LuaEngineUtil :: ReadIntArg( pLuaState, 3, "layerId", nLayerId, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

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

                std :: string                         strError;
                std :: string                         strName;
                SunLight :: TileMap :: stObject       object;
                SunLight :: TileMap :: stMapInfo      mapInfo;
                unsigned int                          nGID = 0;

                if( !LuaEngineUtil :: ReadStringArg( pLuaState, 1, "name", strName, strError ) )
                    return LuaEngineUtil :: PushError( pLuaState, strError );

                SunLight :: TileMap :: ITileMap*  pTileMap = LuaEngineUtil :: GetTileMap( pLuaState );

                if( !pTileMap -> GetMapInfo( mapInfo ) )
                    return LuaEngineUtil :: PushError( pLuaState, __NO_MAP_LOADED );

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
                lua_register( pLuaState, "tilemap_get_layer_parallax", LuaTilemapApi :: GetLayerParallax );
                lua_register( pLuaState, "tilemap_get_layer_parallax_by_name", LuaTilemapApi :: GetLayerParallaxByName );
                lua_register( pLuaState, "tilemap_set_layer_parallax", LuaTilemapApi :: SetLayerParallax );
                lua_register( pLuaState, "tilemap_set_layer_parallax_by_name", LuaTilemapApi :: SetLayerParallaxByName );
                lua_register( pLuaState, "tilemap_get_parallax_origin", LuaTilemapApi :: GetParallaxOrigin );
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
